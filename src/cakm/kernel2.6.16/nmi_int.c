/**
 * @file nmi_int.c
 *
 * @remark Copyright 2002 OProfile authors
 * @remark Read the file COPYING
 *
 * @author John Levon <levon@movementarian.org>
 */

#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/smp.h>
#include "oprofile.h"
#include <linux/sysdev.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>
#include <linux/percpu.h>
#include <asm/nmi.h>
#include <asm/msr.h>
#include <asm/apic.h>
#include <linux/version.h>

#include "op_counter.h"
#include "op_x86_model.h"
#include "oprofile_stats.h"
#include "../cakm_ver.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16) // RHEL4
#define pm_message_t u32
#endif

/* Check IBS Op Counting Mode Supported: CPUID Fn8000_001B_EAX[4] */
#define CPUID_IBS_OP_COUNTING_MODE(x) 	((x & (0x1 << 4))? 1: 0)

#ifndef for_each_possible_cpu
#define for_each_possible_cpu(i)	for (i = 0; i < NR_CPUS; ++i)
#endif

static struct op_x86_model_spec const * model;
static DEFINE_PER_CPU(struct op_msrs, cpu_msrs);
static DEFINE_PER_CPU(unsigned long, saved_lvtpc);

static int nmi_start(void);
static void nmi_stop(void);
static void nmi_cpu_start(void * dummy);
static void nmi_cpu_stop(void * dummy);

/* 0 == registered but off, 1 == registered and on */
static int nmi_enabled = 0;
int ibs_allowed = 0;	/* AMD Family 10h+ */
u32 cpuid_ibs_id;
extern unsigned long driver_version;	/* driver version in oprof.c */
extern void op_amd_set_num_perfctr(void);

unsigned long mux_norm = 1; /* Default MUX normalization value */

#ifdef CONFIG_PM

static int nmi_suspend(struct sys_device *dev, pm_message_t state)
{
	/* Only one CPU left, just stop that one */
	if (nmi_enabled == 1)
		nmi_cpu_stop(NULL);
	return 0;
}


static int nmi_resume(struct sys_device *dev)
{
	if (nmi_enabled == 1)
		nmi_cpu_start(NULL);
	return 0;
}


static struct sysdev_class oprofile_sysclass = {
	set_kset_name("oprofile"),
	.resume		= nmi_resume,
	.suspend	= nmi_suspend,
};


static struct sys_device device_oprofile = {
	.id	= 0,
	.cls	= &oprofile_sysclass,
};


static int __init init_driverfs(void)
{
	int error;
	error = sysdev_class_register(&oprofile_sysclass);
	if (!error)
		error = sysdev_register(&device_oprofile);
	return error;
}


static void exit_driverfs(void)
{
	sysdev_unregister(&device_oprofile);
	sysdev_class_unregister(&oprofile_sysclass);
}

#else
#define init_driverfs() do { } while (0)
#define exit_driverfs() do { } while (0)
#endif /* CONFIG_PM */

static void nmi_cpu_switch(void *dummy)
{
	int cpu = smp_processor_id();
	struct op_msrs *msrs = &per_cpu(cpu_msrs, cpu);
	model->switch_ctrs(msrs);
}

static int nmi_switch_event(void)
{
	/* Check CPU 0 should be sufficient */
	int cpu = smp_processor_id();
	struct op_msrs *msrs = &per_cpu(cpu_msrs, cpu);

	if (model->check_multiplexing(msrs) < 0)
		return -EINVAL; 

	on_each_cpu(nmi_cpu_switch, NULL, 0, 1);
	return 0;
}


static int nmi_callback(struct pt_regs * regs, int cpu)
{
	return model->check_ctrs(regs, &per_cpu(cpu_msrs, cpu));
}


static void nmi_cpu_save_registers(struct op_msrs * msrs)
{
	unsigned int const nr_ctrs = model->num_counters;
	unsigned int const nr_ctrls = model->num_controls;
	struct op_msr * counters = msrs->counters;
	struct op_msr * controls = msrs->controls;
	unsigned int i;

	for (i = 0; i < nr_ctrs; ++i) {
		if (counters[i].addr) {
			rdmsr(counters[i].addr,
				counters[i].saved.low,
				counters[i].saved.high);
		}
	}

	for (i = 0; i < nr_ctrls; ++i) {
		if (controls[i].addr) {
			rdmsr(controls[i].addr,
				controls[i].saved.low,
				controls[i].saved.high);
		}
	}
}


static void nmi_save_registers(void * dummy)
{
	int cpu = smp_processor_id();
	struct op_msrs * msrs = &per_cpu(cpu_msrs, cpu);
	nmi_cpu_save_registers(msrs);
}


static void free_msrs(void)
{
	int i;
	for_each_possible_cpu(i) {
		kfree(per_cpu(cpu_msrs, i).counters);
		per_cpu(cpu_msrs, i).counters = NULL;
		kfree(per_cpu(cpu_msrs, i).controls);
		per_cpu(cpu_msrs, i).controls = NULL;
	}
}


static int allocate_msrs(void)
{
	int success = 1;
	size_t controls_size = sizeof(struct op_msr) * model->num_controls;
	size_t counters_size = sizeof(struct op_msr) * model->num_counters;

	int i;
	for_each_possible_cpu(i) {
		per_cpu(cpu_msrs, i).counters = kmalloc(counters_size,
								GFP_KERNEL);
		if (!per_cpu(cpu_msrs, i).counters) {
			success = 0;
			break;
		}
		per_cpu(cpu_msrs, i).controls = kmalloc(controls_size,
								GFP_KERNEL);
		if (!per_cpu(cpu_msrs, i).controls) {
			success = 0;
			break;
		}
	}

	if (!success)
		free_msrs();

	return success;
}


static void nmi_cpu_setup(void * dummy)
{
	int cpu = smp_processor_id();
	struct op_msrs * msrs = &per_cpu(cpu_msrs, cpu);
	spin_lock(&oprofilefs_lock);
	model->setup_ctrs(msrs);
	spin_unlock(&oprofilefs_lock);
	per_cpu(saved_lvtpc, cpu) = apic_read(APIC_LVTPC);
	apic_write(APIC_LVTPC, APIC_DM_NMI);
}


static int nmi_setup(void)
{
	int cpu;

	if (!allocate_msrs())
		return -ENOMEM;

	/* We walk a thin line between law and rape here.
	 * We need to be careful to install our NMI handler
	 * without actually triggering any NMIs as this will
	 * break the core code horrifically.
	 */
	if (reserve_lapic_nmi() < 0) {
		free_msrs();
		return -EBUSY;
	}

	/* We need to serialize save and setup for HT because the subset
	 * of msrs are distinct for save and setup operations
	 */

	/* Assume saved/restored counters are the same on all CPUs */
	model->fill_in_addresses(&per_cpu(cpu_msrs, 0));
	for_each_possible_cpu(cpu) {
		if (cpu != 0) {
			memcpy(per_cpu(cpu_msrs, cpu).counters,
				per_cpu(cpu_msrs, 0).counters,
				sizeof(struct op_msr) * model->num_counters);

			memcpy(per_cpu(cpu_msrs, cpu).controls,
				per_cpu(cpu_msrs, 0).controls,
				sizeof(struct op_msr) * model->num_controls);
		}

	}

	/*setup AMD Family10h IBS irq if needed */
	if (ibs_allowed) {
		setup_ibs_nmi();
	}

	on_each_cpu(nmi_save_registers, NULL, 0, 1);
	on_each_cpu(nmi_cpu_setup, NULL, 0, 1);
	set_nmi_callback(nmi_callback);
	nmi_enabled = 1;
	return 0;
}


static void nmi_restore_registers(struct op_msrs * msrs)
{
	unsigned int const nr_ctrs = model->num_counters;
	unsigned int const nr_ctrls = model->num_controls;
	struct op_msr * counters = msrs->counters;
	struct op_msr * controls = msrs->controls;
	unsigned int i;

	for (i = 0; i < nr_ctrls; ++i) {
		if (controls[i].addr) {
			wrmsr(controls[i].addr,
				controls[i].saved.low,
				controls[i].saved.high);
		}
	}

	for (i = 0; i < nr_ctrs; ++i) {
		if (counters[i].addr) {
			wrmsr(counters[i].addr,
				counters[i].saved.low,
				counters[i].saved.high);
		}
	}
}


static void nmi_cpu_shutdown(void * dummy)
{
	unsigned int v;
	int cpu = smp_processor_id();
	struct op_msrs *msrs = &__get_cpu_var(cpu_msrs);

	/* restoring APIC_LVTPC can trigger an apic error because the delivery
	 * mode and vector nr combination can be illegal. That's by design: on
	 * power on apic lvt contain a zero vector nr which are legal only for
	 * NMI delivery mode. So inhibit apic err before restoring lvtpc
	 */
	v = apic_read(APIC_LVTERR);
	apic_write(APIC_LVTERR, v | APIC_LVT_MASKED);
	apic_write(APIC_LVTPC, per_cpu(saved_lvtpc, cpu));
	apic_write(APIC_LVTERR, v);
	nmi_restore_registers(msrs);
}


static void nmi_shutdown(void)
{
	struct op_msrs *msrs;

	nmi_enabled = 0;
	on_each_cpu(nmi_cpu_shutdown, NULL, 0, 1);
	unset_nmi_callback();
	release_lapic_nmi();
	msrs = &get_cpu_var(cpu_msrs);
	model->shutdown(msrs);
	free_msrs();
	put_cpu_var(cpu_msrs);

	/*clear AMD Family 10h IBS irq if needed */
	if (ibs_allowed)
		clear_ibs_nmi();
}


static void nmi_cpu_start(void * dummy)
{
	struct op_msrs const * msrs = &__get_cpu_var(cpu_msrs);
	model->start(msrs);
}


static int nmi_start(void)
{
	on_each_cpu(nmi_cpu_start, NULL, 0, 1);
	return 0;
}


static void nmi_cpu_stop(void * dummy)
{
	struct op_msrs const * msrs = &__get_cpu_var(cpu_msrs);
	model->stop(msrs);
}


static void nmi_stop(void)
{
	on_each_cpu(nmi_cpu_stop, NULL, 0, 1);
}


struct op_counter_config counter_config[OP_MAX_COUNTER];
struct op_ibs_config ibs_config;

static int nmi_detect_ctrs(unsigned int * avail_cntr_mask)
{
	*avail_cntr_mask = 0;

	/* SURAVEE: We need to reserve the local APIC nmi
	 *          before messing around with the perf counter.
	 */	
	if (reserve_lapic_nmi() < 0) 
		return -EBUSY;

	*avail_cntr_mask = model->detect_ctrs();


	/* SURAVEE: Restore the local APIC nmi.  Here the Watchdog
	 *          nmi might be re-enabled.
	 */	
	release_lapic_nmi();
	return 0;

}

static int nmi_create_files(struct super_block * sb, struct dentry * root)
{
	unsigned int i;
	struct dentry *dir;
	unsigned int avail_cntr_mask;

	if (nmi_detect_ctrs(&avail_cntr_mask) < 0)
		return -EBUSY;

	for (i = 0; i < model->num_counters; ++i) {

		char buf[4];

		/* Do not export the interface if the hardware is not available. */
		if (!(avail_cntr_mask & (1 << i)))
			continue;

		snprintf(buf,  sizeof(buf), "%d", i);
		dir = oprofilefs_mkdir(sb, root, buf);
		oprofilefs_create_ulong(sb, dir, "enabled",
					&counter_config[i].enabled);
		oprofilefs_create_ulong(sb, dir, "event",
					&counter_config[i].event);
		oprofilefs_create_ulonglong(sb, dir, "count",
					&counter_config[i].count);
		oprofilefs_create_ulong(sb, dir, "unit_mask",
					&counter_config[i].unit_mask);
		oprofilefs_create_ulong(sb, dir, "kernel",
					&counter_config[i].kernel);
		oprofilefs_create_ulong(sb, dir, "user",
					&counter_config[i].user);
	}

	/* Setup AMD Family 10h IBS control if needed */
	if (ibs_allowed) {
		char buf[12];

		/* setup some reasonable defaults */
		ibs_config.max_cnt_fetch = 250000;
		ibs_config.FETCH_enabled = 0;
		ibs_config.max_cnt_op = 250000;
		ibs_config.OP_enabled = 0;
		ibs_config.dispatched_ops = 1;
		ibs_config.rand_en = 1;

		oprofilefs_create_ulong(sb,root, "version",
					&driver_version);

		snprintf(buf,  sizeof(buf), "ibs_fetch");
		dir = oprofilefs_mkdir(sb, root, buf);
		oprofilefs_create_ulong(sb, dir, "rand_enable",
					&ibs_config.rand_en);
		oprofilefs_create_ulong(sb, dir, "enable",
					&ibs_config.FETCH_enabled);
		oprofilefs_create_ulong(sb, dir, "max_count",
					&ibs_config.max_cnt_fetch);
		snprintf(buf,  sizeof(buf), "ibs_op");
		dir = oprofilefs_mkdir(sb, root, buf);
		oprofilefs_create_ulong(sb, dir, "enable",
					&ibs_config.OP_enabled);
		oprofilefs_create_ulong(sb, dir, "max_count",
					&ibs_config.max_cnt_op);

		if (CPUID_IBS_OP_COUNTING_MODE(cpuid_ibs_id))
			oprofilefs_create_ulong(sb, dir, "dispatched_ops",
						&ibs_config.dispatched_ops);
	}

	return 0;
}

static int p4force;
module_param(p4force, int, 0);

static int __init p4_init(char ** cpu_type)
{
	__u8 cpu_model = boot_cpu_data.x86_model;

	if (!p4force && (cpu_model > 6 || cpu_model == 5))
		return 0;

#ifndef CONFIG_SMP
	*cpu_type = "i386/p4";
	model = &op_p4_spec;
	return 1;
#else
	switch (smp_num_siblings) {
	case 1:
		*cpu_type = "i386/p4";
		model = &op_p4_spec;
		return 1;

	case 2:
		*cpu_type = "i386/p4-ht";
		model = &op_p4_ht2_spec;
		return 1;
	}
#endif

	printk(KERN_INFO "oprofile: P4 HyperThreading detected with > 2 threads\n");
	printk(KERN_INFO "oprofile: Reverting to timer mode.\n");
	return 0;
}


static int __init ppro_init(char ** cpu_type)
{
	__u8 cpu_model = boot_cpu_data.x86_model;

	switch (cpu_model) {
	case 0 ... 2:
		*cpu_type = "i386/ppro";
		break;
	case 3 ... 5:
		*cpu_type = "i386/pii";
		break;
	case 6 ... 8:
		*cpu_type = "i386/piii";
		break;
	case 9:
		*cpu_type = "i386/p6_mobile";
		break;
	case 10 ... 13:
		*cpu_type = "i386/p6";
		break;
	case 14:
		*cpu_type = "i386/core";
		break;
	case 15: case 23:
		*cpu_type = "i386/core_2";
		break;
	case 26:
		*cpu_type = "i386/core_2";
		break;
	default:
		/* Unknown */
		return 0;
	}

	model = &op_ppro_spec;
	return 1;
}

/* in order to get driverfs right */
static int using_nmi;

int __init op_nmi_init(struct oprofile_operations *ops)
{
	__u8 vendor = boot_cpu_data.x86_vendor;
	__u8 family = boot_cpu_data.x86;
	__u8 cpu_model = boot_cpu_data.x86_model;
	char *cpu_type = "";
	uint32_t eax, ebx, ecx, edx;

	if (!cpu_has_apic)
		return -ENODEV;

	switch (vendor) {
		case X86_VENDOR_AMD:
			
			printk(KERN_DEBUG "oprofile: CAKM version %u.%u.%u\n", 
						CAKM_MAJOR, CAKM_MINOR, CAKM_MICRO);

			/* Needs to be at least an Athlon (or hammer in 32bit mode) */

			switch (family) {
			default:
				return -ENODEV;
			case 6:
				model = &op_athlon_spec;
				cpu_type = "i386/athlon";
				break;
			case 0xf:
				model = &op_athlon_spec;
				/* Actually it could be i386/hammer too, but give
				   user space an consistent name. */
				cpu_type = "x86-64/hammer";
				break;
			case 0x10:
				model = &op_athlon_spec;
				cpu_type = "x86-64/family10";
				break;
			case 0x11:
				model = &op_athlon_spec;
				cpu_type = "x86-64/family11h";
				break;
			case 0x12:
				model = &op_athlon_spec;
				cpu_type = "x86-64/family12h";
				break;
			case 0x14:
				model = &op_athlon_spec;
				cpu_type = "x86-64/family14h";
				break;
			case 0x15:
				model = &op_amd_family15h_spec;
				if (cpu_model < 0x10)
					cpu_type = "x86-64/family15h";
				else if (cpu_model < 0x20)
					cpu_type = "x86-64/family15h_1xh";
				break;
			}
			/* see if IBS is available */
			if (family >= 0x10) {
				cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
				if (ecx & 0x400) {
					/* This CPU has IBS capability */
					ibs_allowed = 1;

					/* 
					 * Check Instruction Based Sampling Identifiers 
					 * (CPUID Fn8000_001B) availability (RevC and later)
					 */
					if (cpuid_eax(0x80000000) >= 0x8000001B)
						cpuid_ibs_id = cpuid_eax(0x8000001B);

				}
			}

			/* Setup number of counters */
			op_amd_set_num_perfctr();

			break;

		case X86_VENDOR_INTEL:
			switch (family) {
				/* Pentium IV */
				case 0xf:
					if (!p4_init(&cpu_type))
						return -ENODEV;
					break;

				/* A P6-class processor */
				case 6:
					if (!ppro_init(&cpu_type))
						return -ENODEV;
					break;

				default:
					return -ENODEV;
			}
			break;

		default:
			return -ENODEV;
	}

	init_driverfs();
	using_nmi = 1;
	ops->create_files = nmi_create_files;
	ops->setup = nmi_setup;
	ops->shutdown = nmi_shutdown;
	ops->start = nmi_start;
	ops->stop = nmi_stop;
	ops->cpu_type = cpu_type;
	ops->switch_events = nmi_switch_event;
	printk(KERN_INFO "oprofile: using NMI interrupt.\n");
	return 0;
}


void op_nmi_exit(void)
{
	if (using_nmi)
		exit_driverfs();
}
