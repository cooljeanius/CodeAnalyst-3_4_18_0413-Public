/**
 * @file cpu_buffer.h
 *
 * @remark Copyright 2002 OProfile authors
 * @remark Read the file COPYING
 *
 * @author John Levon <levon@movementarian.org>
 */

#ifndef OPROFILE_CPU_BUFFER_H
#define OPROFILE_CPU_BUFFER_H

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/cache.h>
 
struct task_struct;
 
int alloc_cpu_buffers(void);
void free_cpu_buffers(void);

void start_cpu_work(void);
void end_cpu_work(void);

/* CPU buffer is composed of such entries (which are
 * also used for context switch notes)
 */
struct op_sample {
	unsigned long eip;
	unsigned long event;
};
 
struct oprofile_cpu_buffer {
	volatile unsigned long head_pos;
	volatile unsigned long tail_pos;
	unsigned long buffer_size;
	struct task_struct * last_task;
	int last_cpu_mode;
	int tracing;
	struct op_sample * buffer;
	unsigned long sample_received;
	unsigned long sample_lost_overflow;
	unsigned long backtrace_aborted;
#ifdef CONFIG_CA_CSS
	unsigned long ca_css_interval;
#endif
	int cpu;
	struct work_struct work;
} ____cacheline_aligned;

extern struct oprofile_cpu_buffer cpu_buffer[];

void cpu_buffer_reset(struct oprofile_cpu_buffer * cpu_buf);

/* transient events for the CPU buffer -> event buffer */
#define CPU_MODE_USER           0
#define CPU_MODE_KERNEL         1
#define CPU_MODE_XEN            2
#define CPU_TRACE_BEGIN         3
#define CPU_DOMAIN_SWITCH       4
#define IBS_FETCH_BEGIN		5
#define IBS_OP_BEGIN		6


#endif /* OPROFILE_CPU_BUFFER_H */
