/**
 * @file oprof.c
 *
 * @remark Copyright 2002 OProfile authors
 * @remark Read the file COPYING
 *
 * @author John Levon <levon@movementarian.org>
 *
 * Modified by Aravind Menon for Xen
 * These modifications are:
 * Copyright (C) 2005 Hewlett-Packard Co.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/workqueue.h>
#include <linux/time.h>
#include <asm/semaphore.h>
#include <linux/version.h>

#include "oprofile.h"
#include "oprof.h"
#include "event_buffer.h"
#include "cpu_buffer.h"
#include "buffer_sync.h"
#include "oprofile_stats.h"

struct oprofile_operations oprofile_ops;

static void switch_worker(void *work);
static DECLARE_WORK(switch_work, switch_worker, NULL);
static unsigned long is_setup;

#ifdef CONFIG_CA_CSS

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
#include <linux/mutex.h>
DEFINE_MUTEX(start_mutex);
#else
DECLARE_MUTEX(start_sem);
#endif

#else // CONFIG_CA_CSS

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
#include <linux/mutex.h>
static DEFINE_MUTEX(start_mutex);
#else
static DECLARE_MUTEX(start_sem);
#endif

#endif // CONFIG_CA_CSS

static unsigned int switch_work_pending = 0;

unsigned long timeout_jiffies;
unsigned long driver_version = 0x1000;	/*Update version here */
unsigned long oprofile_started;
unsigned long backtrace_depth;


/* timer
   0 - use performance monitoring hardware if available
   1 - use the timer int mechanism regardless
 */
static int timer = 0;

#ifdef CONFIG_XEN
int oprofile_set_active(int active_domains[], unsigned int adomains)
{
	int err;

	if (!oprofile_ops.set_active)
		return -EINVAL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_lock(&start_mutex);
#else
	down(&start_sem);
#endif
	err = oprofile_ops.set_active(active_domains, adomains);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_unlock(&start_mutex);
#else
	up (&start_sem);
#endif
	return err;
}

int oprofile_set_passive(int passive_domains[], unsigned int pdomains)
{
	int err;

	if (!oprofile_ops.set_passive)
		return -EINVAL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_lock(&start_mutex);
#else
	down(&start_sem);
#endif
	err = oprofile_ops.set_passive(passive_domains, pdomains);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_unlock(&start_mutex);
#else
	up (&start_sem);
#endif
	return err;
}
#endif

int oprofile_setup(void)
{
	int err;
 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_lock(&start_mutex);
#else
	down(&start_sem);
#endif

	if ((err = alloc_cpu_buffers()))
		goto out;

	if ((err = alloc_event_buffer()))
		goto out1;
 
	if (oprofile_ops.setup && (err = oprofile_ops.setup()))
		goto out2;
 
	/* Note even though this starts part of the
	 * profiling overhead, it's necessary to prevent
	 * us missing task deaths and eventually oopsing
	 * when trying to process the event buffer.
	 */
	if ((err = sync_start()))
		goto out3;

	is_setup = 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_unlock(&start_mutex);
#else
	up (&start_sem);
#endif
	return 0;
 
out3:
	if (oprofile_ops.shutdown)
		oprofile_ops.shutdown();
out2:
	free_event_buffer();
out1:
	free_cpu_buffers();
out:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_unlock(&start_mutex);
#else
	up (&start_sem);
#endif
	return err;
}


static void start_switch_worker(void)
{
	schedule_delayed_work(&switch_work, timeout_jiffies);
}

static void switch_worker(void *work)
{
	if (oprofile_ops.switch_events() < 0) {
		switch_work_pending = 0;
	} else  {
		atomic_inc(&oprofile_stats.multiplex_counter);
		start_switch_worker();
		switch_work_pending = 1;
	}
}


/* Actually start profiling (echo 1>/dev/oprofile/enable) */
int oprofile_start(void)
{
	int err = -EINVAL;
 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_lock(&start_mutex);
#else
	down(&start_sem);
#endif
 
	if (!is_setup)
		goto out;

	err = 0; 
 
	if (oprofile_started)
		goto out;
 
	oprofile_reset_stats();

	if ((err = oprofile_ops.start()))
		goto out;

	if (oprofile_ops.switch_events)
		start_switch_worker();

	oprofile_started = 1;
out:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_unlock(&start_mutex);
#else
	up (&start_sem);
#endif
	return err;
}

 
/* echo 0>/dev/oprofile/enable */
void oprofile_stop(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_lock(&start_mutex);
#else
	down(&start_sem);
#endif
	if (!oprofile_started)
		goto out;

	oprofile_ops.stop();
	oprofile_started = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16) //SLES10, RHEL5
	if (switch_work_pending) {
		cancel_rearming_delayed_work(&switch_work);
	}
#else //RHEL4
	cancel_delayed_work(&switch_work);
#endif
	/* wake up the daemon to read what remains */
	wake_up_buffer_waiter();

out:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_unlock(&start_mutex);
#else
	up (&start_sem);
#endif
}


void oprofile_shutdown(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_lock(&start_mutex);
#else
	down(&start_sem);
#endif
	sync_stop();
	if (oprofile_ops.shutdown)
		oprofile_ops.shutdown();
	is_setup = 0;
	free_event_buffer();
	free_cpu_buffers();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_unlock(&start_mutex);
#else
	up (&start_sem);
#endif
}


/* User inputs in ms, converts to jiffies */
int oprofile_set_timeout(unsigned long val_msec)
{
	int err = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_lock(&start_mutex);
#else
	down(&start_sem);
#endif

	if (oprofile_started) {
		err = -EBUSY;
		goto out;
	}

	if (!oprofile_ops.switch_events) {
		err = -EINVAL;
		goto out;
	}

	timeout_jiffies = msecs_to_jiffies(val_msec);
	if (timeout_jiffies == MAX_JIFFY_OFFSET)
		timeout_jiffies = msecs_to_jiffies(1);

out:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_unlock(&start_mutex);
#else
	up (&start_sem);
#endif
	return err;

}


int oprofile_set_backtrace(unsigned long val)
{
	int err = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_lock(&start_mutex);
#else
	down(&start_sem);
#endif

	if (oprofile_started) {
		err = -EBUSY;
		goto out;
	}

	if (!oprofile_ops.backtrace) {
		err = -EINVAL;
		goto out;
	}

	backtrace_depth = val;

out:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	mutex_unlock(&start_mutex);
#else
	up (&start_sem);
#endif
	return err;
}


static void __init oprofile_switch_timer_init(void)
{
	timeout_jiffies = msecs_to_jiffies(1);
}


static int __init oprofile_init(void)
{
	int err;

	oprofile_switch_timer_init();
	err = oprofile_arch_init(&oprofile_ops);

	if (err < 0 || timer) {
		printk(KERN_INFO "oprofile: using timer interrupt.\n");
		oprofile_timer_init(&oprofile_ops);
	}

	err = oprofilefs_register();
	if (err)
		oprofile_arch_exit();

	return err;
}


static void __exit oprofile_exit(void)
{
	oprofilefs_unregister();
	oprofile_arch_exit();
}

 
module_init(oprofile_init);
module_exit(oprofile_exit);

module_param_named(timer, timer, int, 0644);
MODULE_PARM_DESC(timer, "force use of timer interrupt");
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Levon <levon@movementarian.org>");
MODULE_DESCRIPTION("OProfile system profiler");
