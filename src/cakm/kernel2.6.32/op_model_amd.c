/*
 * @file op_model_amd.c
 *
 * athlon / K7 / K8 / 
 * Family 10h/11h/12h/14h/15h 
 * model-specific MSR operations
 *
 * @remark Copyright 2002-2009 OProfile authors
 * @remark Read the file COPYING
 *
 * @author John Levon
 * @author Philippe Elie
 * @author Graydon Hoare
 * @author Robert Richter <robert.richter@amd.com>
 * @author Barry Kasindorf <barry.kasindorf@amd.com>
 * @author Jason Yeh <jason.yeh@amd.com>
 * @author Suravee Suthikulpanit <suravee.suthikulpanit@amd.com>
 */

#include "oprofile.h"
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/percpu.h>
#include <linux/fs.h>
#include <linux/version.h>

#include <asm/ptrace.h>
#include <asm/msr.h>
#include <asm/nmi.h>
#include <asm/mutex.h>

#include "op_x86_model.h"
#include "op_counter.h"
#include "../cakm_ver.h"

#if USE_INTERNAL_ERRATA
#include "../internal/errata.h"
#else
#include "../errata.h"
#endif


/* NOTE: Expected to be in msr-index.h in the future */
#ifndef MSR_FAMILY15H_EVNTSEL0
#define MSR_FAMILY15H_EVNTSEL0		0xc0010200
#endif
#ifndef MSR_FAMILY15H_PERFCTR0
#define MSR_FAMILY15H_PERFCTR0		0xc0010201
#endif

#define MSR_AMD64_IBSBPTGTRIP		0xc001103b

#define NUM_COUNTERS 4
#define NUM_COUNTERS_FAMILY15H 6
#ifdef CONFIG_OPROFILE_EVENT_MULTIPLEX
#define NUM_VIRT_COUNTERS 32
static unsigned long reset_value[NUM_VIRT_COUNTERS];
#else
#define NUM_VIRT_COUNTERS op_amd_num_perfctr
static unsigned long reset_value[NUM_COUNTERS_FAMILY15H];
#endif

#define OP_EVENT_MASK			0x0FFF
#define OP_CTR_OVERFLOW			(1ULL<<31)

#define MSR_AMD_EVENTSEL_RESERVED	((0xFFFFFCF0ULL<<32)|(1ULL<<21))


#ifdef CONFIG_OPROFILE_IBS

/* IbsFetchCtl bits/masks */
#define IBS_FETCH_RAND_EN		(1ULL<<57)
#define IBS_FETCH_VAL			(1ULL<<49)
#define IBS_FETCH_ENABLE		(1ULL<<48)
#define IBS_FETCH_CNT_MASK		0xFFFF0000ULL

/*IbsOpCtl bits */
#define IBS_OP_CNT_CTL			(1ULL<<19)
#define IBS_OP_VAL			(1ULL<<18)
#define IBS_OP_ENABLE			(1ULL<<17)

#define IBS_OP_CUR_CNT_EXT_SET(x,y) \
{\
	x &= ~((u64)0x7F << 52);\
	x |= ((y & (u64)0x7F00000) << 32);\
}

#define IBS_OP_CUR_CNT_SET(x,y) \
{\
	x &= ~((u64)0xFFFFF << 32);\
	x |= (y & (u64)0xFFFFF) << 32;\
}
	

#define IBS_OP_MAX_CNT_EXT_SET(x,y) \
{\
	x &= ~((u64)0x7F << 20);\
	x |= (y  & (u64)0x7F00000);\
}

#define IBS_OP_MAX_CNT_SET(x,y) \
{\
	x &= ~((u64)0xFFFF); \
	x |= ((y >> 4) & (u64)0xFFFF);\
}

#define IBS_FETCH_SIZE			6
#define IBS_OP_SIZE			12

#define CPUID_AMD_AUTHENTIC		0x80000000
#define CPUID_IBS_IDENTIFIERS		(CPUID_AMD_AUTHENTIC | 0x1B)

/* Check IBS Read Write Op Counter Supported: CPUID Fn8000_001B_EAX[3] */
#define CPUID_IBS_RW_OP_CNTR(x) 	((x & (0x1 << 3))? 1: 0)

/* Check IBS Op Counting Mode Supported: CPUID Fn8000_001B_EAX[4] */
#define CPUID_IBS_OP_COUNTING_MODE(x) 	((x & (0x1 << 4))? 1: 0)

/* Check IBS Branch Target Address Reporting Supported: CPUID Fn8000_001B_EAX[5] */
#define CPUID_IBS_BR_TRGT_ADDR(x) 	((x & (0x1 << 5))? 1: 0)

/* Check IBS Op extended cur/max count: CPUID Fn8000_001B_EAX[6] */
#define CPUID_IBS_OP_CNT_EXT(x) 	((x & (0x1 << 6))? 1: 0)

/* Check IBS Invalid IBS RIP detection: CPUID Fn8000_001B_EAX[7] */
#define CPUID_IBS_RIP_INVALID_CHK(x) 	((x & (0x1 << 7))? 1: 0)

#define IS_OPRIP_INVALID(x)		((x & ((u64)0x1 << 38))? 1: 0)

extern struct mutex start_mutex;
extern unsigned long oprofile_started;

static int has_ibs;
static u32 cpuid_ibs_id;
static unsigned int ibs_op_max_cnt;

/* Keep track the list of available perf counters */
static unsigned int cntr_mask = 0;

struct op_ibs_config {
	unsigned long op_enabled;
	unsigned long fetch_enabled;
	unsigned long max_cnt_fetch;
	unsigned long max_cnt_op;
	unsigned long rand_en;
	unsigned long dispatched_ops;
	unsigned long branch_target;
};

static struct op_ibs_config ibs_config;

#endif

#ifdef CONFIG_OPROFILE_EVENT_MULTIPLEX

static unsigned int op_amd_num_perfctr;

static inline unsigned int op_amd_msr_evntsel(unsigned int i)
{
	if ( boot_cpu_data.x86 == 0x15)
		return MSR_FAMILY15H_EVNTSEL0 + (i*2);
	else
		return MSR_K7_EVNTSEL0 + i;
}


static inline unsigned int op_amd_msr_perfctr(unsigned int i)
{
	if ( boot_cpu_data.x86 == 0x15)
		return MSR_FAMILY15H_PERFCTR0 + (i*2);
	else
		return MSR_K7_PERFCTR0 + i;
}


static inline void op_amd_set_num_perfctr(void)
{
	if ( boot_cpu_data.x86 == 0x15)
		op_amd_num_perfctr = NUM_COUNTERS_FAMILY15H;
	else
		op_amd_num_perfctr = NUM_COUNTERS;
}


static void op_mux_fill_in_addresses(struct op_msrs * const msrs)
{
	int i;

	for (i = 0; i < NUM_VIRT_COUNTERS; i++) {
		int hw_counter = op_x86_virt_to_phys(i);

		/* NOTE: Suravee: This is a hack */
		if ((cntr_mask & (1 << i)) && reserve_perfctr_nmi(op_amd_msr_perfctr(i)))
			msrs->multiplex[i].addr = op_amd_msr_perfctr(hw_counter);
		else
			msrs->multiplex[i].addr = 0;
	}
}

static void op_mux_switch_ctrl(struct op_x86_model_spec const *model,
			       struct op_msrs const * const msrs)
{
	u64 val;
	int i;

	/* enable active counters */
	for (i = 0; i < op_amd_num_perfctr; ++i) {
		int virt = op_x86_phys_to_virt(i);
		if (!counter_config[virt].enabled)
			continue;
		rdmsrl(msrs->controls[i].addr, val);
		val &= model->reserved;
		val |= op_x86_get_ctrl(model, &counter_config[virt]);
		wrmsrl(msrs->controls[i].addr, val);
	}
}

#else

static inline void op_mux_fill_in_addresses(struct op_msrs * const msrs) { }

#endif


/* SURAVEE: For Family15h rev A, we need to make sure that we can 
 *          disable the counter before allowing it to be enabled.
 */
static void detect_ctr(void * dummy)
{
	unsigned int cntr = *((unsigned int *)dummy);
	unsigned int enable_mask = 1 << 22;
	unsigned int low = 0; 
	unsigned int low_old = 0;
	unsigned int high = 0;

	rdmsr(op_amd_msr_evntsel(cntr) , low, high);

	if (!(low & enable_mask)) {
		// Conter is disabled, so it is available.
		cntr_mask |= (1 << cntr);
		return;
	}

	low_old = low;
		
	// Try to disable MSR
	wrmsr(op_amd_msr_evntsel(0) + cntr, (low & ~enable_mask), high);

	// Read back the MSR to check
	rdmsr(op_amd_msr_evntsel(cntr), low, high);

	if (!(low & enable_mask)) {
		// At this point we know we can write value.
		// Therefore, the counter is available.	
		cntr_mask |= (1 << cntr);

		// Write back the original value	
		wrmsr(op_amd_msr_evntsel(cntr), low_old, high);
	}
}


static unsigned int op_amd_detect_ctrs(void)
{
	unsigned int i;
	cntr_mask = 0;

	for (i = 0 ; i < op_amd_num_perfctr; i++) {
		if (!reserve_evntsel_nmi(op_amd_msr_evntsel(i))) {
			printk (KERN_DEBUG "oprofile: Warning! Coudn't reserve counter %u.\n", i);
			continue;
		}
	
		on_each_cpu(detect_ctr, &i, 1);
		
		release_evntsel_nmi(op_amd_msr_evntsel(i));
	}

	// Replicate the mask to all the bits used for multiplexing
	for (i = op_amd_num_perfctr ; i < NUM_VIRT_COUNTERS; i = i + op_amd_num_perfctr) {
		cntr_mask |= (cntr_mask << op_amd_num_perfctr);
	}

	return cntr_mask;
}


/* functions for op_amd_spec */
static void op_amd_fill_in_addresses(struct op_msrs * const msrs)
{
	int i;

	for (i = 0; i < op_amd_num_perfctr; i++) {
		if ((cntr_mask & (1 << i)) && reserve_perfctr_nmi(op_amd_msr_perfctr(i)))
			msrs->counters[i].addr = op_amd_msr_perfctr(i);
		else
			msrs->counters[i].addr = 0;
	}

	for (i = 0; i < op_amd_num_perfctr; i++) {
		if ((cntr_mask & (1 << i)) && reserve_evntsel_nmi(op_amd_msr_evntsel(i)))
			msrs->controls[i].addr = op_amd_msr_evntsel(i);
		else
			msrs->controls[i].addr = 0;
	}

	op_mux_fill_in_addresses(msrs);
}

static void op_amd_setup_ctrs(struct op_x86_model_spec const *model,
			      struct op_msrs const * const msrs)
{
	u64 val;
	int i;

	/* setup reset_value */
	for (i = 0; i < NUM_VIRT_COUNTERS; ++i) {
		if (counter_config[i].enabled)
			reset_value[i] = counter_config[i].count;
		else
			reset_value[i] = 0;
	}

	/* clear all counters */
	for (i = 0; i < op_amd_num_perfctr; ++i) {
		if (unlikely(!msrs->controls[i].addr))
			continue;
		rdmsrl(msrs->controls[i].addr, val);
		val &= model->reserved;
		wrmsrl(msrs->controls[i].addr, val);
	}

	/* avoid a false detection of ctr overflows in NMI handler */
	for (i = 0; i < op_amd_num_perfctr; ++i) {
		if (unlikely(!msrs->counters[i].addr))
			continue;
		wrmsrl(msrs->counters[i].addr, -1LL);
	}

	/* enable active counters */
	for (i = 0; i < op_amd_num_perfctr; ++i) {
		int virt = op_x86_phys_to_virt(i);
		if (!counter_config[virt].enabled)
			continue;
		if (!msrs->counters[i].addr)
			continue;

		/* setup counter registers */
		wrmsrl(msrs->counters[i].addr, -(u64)reset_value[virt]);

		/* setup control registers */
		rdmsrl(msrs->controls[i].addr, val);
		val &= model->reserved;
		val |= op_x86_get_ctrl(model, &counter_config[virt]);
		wrmsrl(msrs->controls[i].addr, val);
	}
}

#ifdef CONFIG_OPROFILE_IBS
/*
 * 16-bit Linear Feedback Shift Register (LFSR)
 *
 *                       16   14   13    11
 * Feedback polynomial = X  + X  + X  +  X  + 1 
 */  
static unsigned int lfsr_random(void)
{
	static unsigned int lfsr_value = 0xF00D;
	unsigned int bit;

	// Compute next bit to shift in
	bit = ((lfsr_value >> 0) ^
	       (lfsr_value >> 2) ^
	       (lfsr_value >> 3) ^
	       (lfsr_value >> 5)) & 0x0001;

	// Advance to next register value
	return ( lfsr_value = (lfsr_value >> 1) | (bit << 15) ); 
}

#define LFSR_MASK			0xFFF

static void apply_max_cnt_compensation(void)
{
	if (0 == CPUID_IBS_RW_OP_CNTR(cpuid_ibs_id)) 
		return;

	/* Note:
	 * Prevent overflow when enabling IBS-Op randomization
	 */
	if (!CPUID_IBS_OP_CNT_EXT(cpuid_ibs_id)) {
		if (ibs_op_max_cnt > 0xFF800) {
			ibs_op_max_cnt = 0xFF800;
		}
	} else {
		if (ibs_op_max_cnt > 0x7FFF800) {
			ibs_op_max_cnt = 0x7FFF800;
		}
	}

	/* NOTE:
	 * The randomization code modifies the value of current count
	 * register which counts up  from the initial value loaded by
	 * the driver.  Without compensation, the effective sampling
	 * period is smaller than the requested sampling period and
	 * oversampling will result. We add in the mean of the
	 * pseudo-random values (0x7FF in this case in order to
	 * offset the reduction.
	 */
	ibs_op_max_cnt += (LFSR_MASK >> 1);
}

static u64 apply_lfsr_random(u64 ctl) 
{
	unsigned int ran_cur;

	if (0 == CPUID_IBS_RW_OP_CNTR(cpuid_ibs_id))
		return ctl;

	/* Randomize the lower 12 bits of vaIbsOpCurCnt [51:32] */
	ran_cur = lfsr_random() & LFSR_MASK;

	IBS_OP_CUR_CNT_SET(ctl, ran_cur);

	if (CPUID_IBS_OP_CNT_EXT(cpuid_ibs_id))
		IBS_OP_CUR_CNT_EXT_SET(ctl, ran_cur);
	
	return ctl;
}


#ifdef CONFIG_CA_CSS
extern void ca_css_check_interval_and_add(struct pt_regs * const regs);
#endif

static inline void
op_amd_handle_ibs(struct pt_regs * const regs,
		  struct op_msrs const * const msrs)
{
	u64 val, ctl;
	u32 ext_msr_size;
	struct op_entry entry;

	if (!has_ibs)
		return;

	if (ibs_config.fetch_enabled) {
		rdmsrl(MSR_AMD64_IBSFETCHCTL, ctl);
		UBTS231710_workaround(&ctl);
		if (ctl & IBS_FETCH_VAL) {

			rdmsrl(MSR_AMD64_IBSFETCHLINAD, val);

			/* Force linear address in non-canonical form.
			 * (Also workaround for UBTS:228876)
			 */
			val &= ~(0xFFFFULL << 48);

			oprofile_write_reserve(&entry, regs, val,
					       IBS_FETCH_CODE, IBS_FETCH_SIZE);
			oprofile_add_data64(&entry, val);
			oprofile_add_data64(&entry, ctl);
			rdmsrl(MSR_AMD64_IBSFETCHPHYSAD, val);
			oprofile_add_data64(&entry, val);

			oprofile_write_commit(&entry);

			/* reenable the IRQ */
			ctl &= ~(IBS_FETCH_VAL | IBS_FETCH_CNT_MASK);
			ctl |= IBS_FETCH_ENABLE;
			UBTS232156_workaround(&ctl);
			UBTS231710_workaround(&ctl);
			wrmsrl(MSR_AMD64_IBSFETCHCTL, ctl);
		}
	}

	if (ibs_config.op_enabled) {
		ext_msr_size = 0;
		rdmsrl(MSR_AMD64_IBSOPCTL, ctl);
		if (ctl & IBS_OP_VAL) {
			
			/* Check if IbsOpRip is valid */
			rdmsrl(MSR_AMD64_IBSOPDATA, val);
			if (CPUID_IBS_RIP_INVALID_CHK(cpuid_ibs_id) && IS_OPRIP_INVALID(val))
				goto next;

			/* Check if IBS branch target is enabled*/
			if (ibs_config.branch_target)
				ext_msr_size ++;

			rdmsrl(MSR_AMD64_IBSOPRIP, val);
			oprofile_write_reserve(&entry, regs, val,
					       IBS_OP_CODE, IBS_OP_SIZE + ext_msr_size);
			oprofile_add_data64(&entry, val);
			rdmsrl(MSR_AMD64_IBSOPDATA, val);
			oprofile_add_data64(&entry, val);
			rdmsrl(MSR_AMD64_IBSOPDATA2, val);
			oprofile_add_data64(&entry, val);
			rdmsrl(MSR_AMD64_IBSOPDATA3, val);
			oprofile_add_data64(&entry, val);
			rdmsrl(MSR_AMD64_IBSDCLINAD, val);
			oprofile_add_data64(&entry, val);
			rdmsrl(MSR_AMD64_IBSDCPHYSAD, val);
			oprofile_add_data64(&entry, val);

			/* Add branch target MSR*/
			if (ibs_config.branch_target) {
				rdmsrl(MSR_AMD64_IBSBPTGTRIP, val);
				oprofile_add_data(&entry, (unsigned long) val);
			}

			oprofile_write_commit(&entry);
next:
			/* reenable the IRQ */
			ctl &= ~IBS_OP_VAL & 0xFFFFFFFF;
			ctl |= IBS_OP_ENABLE;
			
			ctl = apply_lfsr_random(ctl);

			// Note: (From Paul) 
			// The erratum ERBT-1068 requires SW to clear 
			//IbsOpData3 in some BD and TN processors
			wrmsrl(MSR_AMD64_IBSOPDATA3, 0);

			wrmsrl(MSR_AMD64_IBSOPCTL, ctl);
			rdmsrl(MSR_AMD64_IBSOPRIP, val);  // UBTS 299030
		}
	}
}

static inline void op_amd_start_ibs(void)
{
	u64 val;

	if (has_ibs && ibs_config.fetch_enabled) {
		val = (ibs_config.max_cnt_fetch >> 4) & 0xFFFF;
		val |= ibs_config.rand_en ? IBS_FETCH_RAND_EN : 0;
		val |= IBS_FETCH_ENABLE;
		wrmsrl(MSR_AMD64_IBSFETCHCTL, val);
	}

	val = 0;
	if (has_ibs && ibs_config.op_enabled) {

		// Workaround UBTS 227027 Enable LBR
		// Should this workaround save the previous LBR state?
#define MSR_AMD64_DBG_CTL 0x000001D9
		wrmsrl(MSR_AMD64_DBG_CTL, 0x01);

		ibs_op_max_cnt = ibs_config.max_cnt_op;

		apply_max_cnt_compensation();

		IBS_OP_MAX_CNT_SET(val, ibs_op_max_cnt);
		
		if (CPUID_IBS_OP_CNT_EXT(cpuid_ibs_id))
			IBS_OP_MAX_CNT_EXT_SET(val, ibs_op_max_cnt);

		if (CPUID_IBS_OP_COUNTING_MODE(cpuid_ibs_id)
		&&  ibs_config.dispatched_ops)
			val |= IBS_OP_CNT_CTL;

		val |= IBS_OP_ENABLE;
		wrmsrl(MSR_AMD64_IBSOPCTL, val);
		rdmsrl(MSR_AMD64_IBSOPRIP, val); // Workaround UBTS 299030
	}
}

static void op_amd_stop_ibs(void)
{
	if (has_ibs && ibs_config.fetch_enabled)
		/* clear max count and enable */
		wrmsrl(MSR_AMD64_IBSFETCHCTL, 0);

	if (has_ibs && ibs_config.op_enabled)
		/* clear max count and enable */
		wrmsrl(MSR_AMD64_IBSOPCTL, 0);
		// Workaround UBTS 227027 Enable LBR
		// Should this workaround restore the previous LBR state?
		wrmsrl(MSR_AMD64_DBG_CTL, 0x00);
}

#else

static inline void op_amd_handle_ibs(struct pt_regs * const regs,
				    struct op_msrs const * const msrs) { }
static inline void op_amd_start_ibs(void) { }
static inline void op_amd_stop_ibs(void) { }

#endif

static int op_amd_check_ctrs(struct pt_regs * const regs,
			     struct op_msrs const * const msrs)
{
	u64 val;
	int i;

	for (i = 0; i < op_amd_num_perfctr; ++i) {
		int virt = op_x86_phys_to_virt(i);
		if (!reset_value[virt])
			continue;
		rdmsrl(msrs->counters[i].addr, val);
		/* bit is clear if overflowed: */
		if (val & OP_CTR_OVERFLOW)
			continue;
		oprofile_add_sample(regs, virt);
		wrmsrl(msrs->counters[i].addr, -(u64)reset_value[virt]);
	}

	op_amd_handle_ibs(regs, msrs);

#ifdef CONFIG_CA_CSS
	ca_css_check_interval_and_add(regs); 
#endif
	/* See op_model_ppro.c */
	return 1;
}

static void op_amd_start_pmc(struct op_msrs const * const msrs)
{
	u64 val;
	int i;

	for (i = 0; i < op_amd_num_perfctr; ++i) {
		if (!reset_value[op_x86_phys_to_virt(i)])
			continue;
		rdmsrl(msrs->controls[i].addr, val);
		val |= ARCH_PERFMON_EVENTSEL0_ENABLE;
		wrmsrl(msrs->controls[i].addr, val);
	}
}

static void op_amd_stop_pmc(struct op_msrs const * const msrs)
{
	u64 val;
	int i;

	/*
	 * Subtle: stop on all counters to avoid race with setting our
	 * pm callback
	 */
	for (i = 0; i < op_amd_num_perfctr; ++i) {
		if (!reset_value[op_x86_phys_to_virt(i)])
			continue;
		rdmsrl(msrs->controls[i].addr, val);
		val &= ~ARCH_PERFMON_EVENTSEL0_ENABLE;
		wrmsrl(msrs->controls[i].addr, val);
	}
}

static void op_amd_start(struct op_msrs const * const msrs)
{
	op_amd_start_pmc(msrs);
	op_amd_start_ibs();
}

static void op_amd_stop(struct op_msrs const * const msrs)
{
	op_amd_stop_pmc(msrs);
	op_amd_stop_ibs();
}

static void op_amd_shutdown(struct op_msrs const * const msrs)
{
	int i;

	for (i = 0; i < op_amd_num_perfctr; ++i) {
		if (msrs->counters[i].addr)
			release_perfctr_nmi(op_amd_msr_perfctr(i));
	}
	for (i = 0; i < op_amd_num_perfctr; ++i) {
		if (msrs->controls[i].addr)
			release_evntsel_nmi(op_amd_msr_evntsel(i));
	}
}

#ifdef CONFIG_OPROFILE_IBS

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
#define APIC_EILVT_LVTOFF_MCE 0
#define APIC_EILVT_LVTOFF_IBS 1

static void _setup_APIC_eilvt(u8 lvt_off, u8 vector, u8 msg_type, u8 mask)
{
	unsigned long reg = (lvt_off << 4) + APIC_EILVTn(0);
	unsigned int  v   = (mask << 16) | (msg_type << 8) | vector;

	apic_write(reg, v);
}

u8 setup_APIC_eilvt_ibs(u8 vector, u8 msg_type, u8 mask)
{
	_setup_APIC_eilvt(APIC_EILVT_LVTOFF_IBS, vector, msg_type, mask);
	return APIC_EILVT_LVTOFF_IBS;
}
#endif

static inline void apic_init_ibs_nmi_per_cpu(void *arg)
{
	setup_APIC_eilvt_ibs(0, APIC_EILVT_MSG_NMI, 0);
}

static inline void apic_clear_ibs_nmi_per_cpu(void *arg)
{
	setup_APIC_eilvt_ibs(0, APIC_EILVT_MSG_FIX, 1);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
struct pci_dev *pci_get_domain_bus_and_slot(int domain, unsigned int bus,
                                            unsigned int devfn)
{
	struct pci_dev *dev = NULL;

	for_each_pci_dev(dev) {
		if (pci_domain_nr(dev->bus) == domain &&
			(dev->bus->number == bus && dev->devfn == devfn))
		return dev;
	}
	return NULL;
}
#endif

static int get_pci_device_id_misc(void)
{
	int dev_id = 0;
	struct pci_dev * dev;

	// Use domain 0, bus 0, device 18, function 3
	if ((dev = pci_get_domain_bus_and_slot(0,0,0xc3)) != NULL) {
		if (PCI_VENDOR_ID_AMD == dev->vendor)
			dev_id = dev->device;
		pci_dev_put(dev);
	}
	
	return dev_id;
}

#define AMD_PCI_MISC_CTRL_MAX	8
static struct pci_dev * pci_dev_misc[AMD_PCI_MISC_CTRL_MAX];
static unsigned int pci_dev_misc_cnt;
static u32 ibs_eilvt;

#define IBSCTL_LVTOFFSETVAL		(1 << 8)
#define IBSCTL				0x1cc

/*
 * NOTE: [Suravee] (7/22/2010)
 * We need to validate the IBS Ctrl MSRs on each core since
 * some cores might not be initialized. (ON RevA)
 * The workaround is to have that particular core reconfigure
 * the IBS Control Register through the PCI configuration space.
 */
static void ibs_ctrl_msr_validate(void * arg)
{
	u64 msr_val = 0;
	int i;
	int retry = 1;

	do {	
		/* Read IBS Ctrl MSR to verify IBS configuration*/
		rdmsrl (MSR_AMD64_IBSCTL, msr_val);
		if (msr_val == ibs_eilvt)
			break;

		if (!retry) {
			printk (KERN_DEBUG "oprofile: Error! CPU %d failed to configure IBS Control Register.\n", smp_processor_id());
		}

		printk (KERN_DEBUG "oprofile: Warning! CPU %d attempting to configure IBS Control Register.\n", smp_processor_id());

		for (i = 0 ; i < pci_dev_misc_cnt ; i++) {
			/* Setup IBS Ctrl via PCI config space */
			pci_write_config_dword(pci_dev_misc[i], IBSCTL, ibs_eilvt);
		}
		retry = 0;
	} while (1);
}


static int init_ibs_nmi(void)
{
	struct pci_dev * cpu_cfg;
	u32 value;
	int pci_device_id_misc;

	pci_device_id_misc = get_pci_device_id_misc();
	if (0 == pci_device_id_misc) {
		printk(KERN_DEBUG "oprofile: Error! Failed to get PCI device ID.");
		return 1;
	}

	ibs_eilvt = setup_APIC_eilvt_ibs(0, APIC_EILVT_MSG_FIX, 1) | IBSCTL_LVTOFFSETVAL;

	cpu_cfg = NULL;
	pci_dev_misc_cnt = 0;
	do {
		cpu_cfg = pci_get_device(PCI_VENDOR_ID_AMD,
					 pci_device_id_misc,
					 cpu_cfg);
		if (!cpu_cfg)
			break;

		pci_write_config_dword(cpu_cfg, IBSCTL, ibs_eilvt);
		pci_read_config_dword(cpu_cfg, IBSCTL, &value);
		if (value != ibs_eilvt) {
			pci_dev_put(cpu_cfg);
			printk(KERN_DEBUG "oprofile: Error! Failed to setup IBS LVT offset, "
				"IBSCTL = 0x%08x", value);
			return 1;
		}
		
		pci_dev_misc[pci_dev_misc_cnt++] = cpu_cfg;
	
	} while (1);

	if (!pci_dev_misc_cnt) {
		printk(KERN_DEBUG "No CPU node configured for IBS");
		return 1;
	} 

	/* per CPU IBS Control MSR validation */
	on_each_cpu (ibs_ctrl_msr_validate, NULL, 1);
	
	/* per CPU setup */
	on_each_cpu(apic_init_ibs_nmi_per_cpu, NULL, 1);

	return 0;
}

/* uninitialize the APIC for the IBS interrupts if needed */
static void clear_ibs_nmi(void)
{
	if (has_ibs)
		on_each_cpu(apic_clear_ibs_nmi_per_cpu, NULL, 1);
}

/* initialize the APIC for the IBS interrupts if available */
static void init_ibs(void)
{
	has_ibs = boot_cpu_has(X86_FEATURE_IBS);

	if (!has_ibs)
		return;

	printk(KERN_INFO "oprofile: AMD IBS detected\n");

	/* 
	 * Check Instruction Based Sampling Identifiers 
	 * (CPUID Fn8000_001B) availability (RevC and later)
	 */
	if (cpuid_eax(CPUID_AMD_AUTHENTIC) >= CPUID_IBS_IDENTIFIERS)
		cpuid_ibs_id = cpuid_eax(CPUID_IBS_IDENTIFIERS);

	if (init_ibs_nmi()) {
		has_ibs = 0;
		return;
	}

}

static void ibs_exit(void)
{
	if (!has_ibs)
		return;

	clear_ibs_nmi();
}

static int (*create_arch_files)(struct super_block *sb, struct dentry *root);

int op_amd_set_ibs_bta_flag(unsigned long val)
{
	int err = 0;

	mutex_lock(&start_mutex);

	if (oprofile_started) {
		err = -EBUSY;
		goto out;
	}

	ibs_config.branch_target = val;

out:
	mutex_unlock(&start_mutex);
	return err;
}

static ssize_t ibs_bta_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	return oprofilefs_ulong_to_user(ibs_config.branch_target, buf, count, offset);
}

static ssize_t ibs_bta_write(struct file *file, char const __user *buf, size_t count, loff_t *offset)
{
	unsigned long val;
	int retval;

	if (*offset)
		return -EINVAL;

	retval = oprofilefs_ulong_from_user(&val, buf, count);
	if (retval)
		return retval;

	retval = op_amd_set_ibs_bta_flag(val);

	if (retval)
		return retval;
	return count;
}

static const struct file_operations ibs_bta_fops = {
	.read		= ibs_bta_read,
	.write		= ibs_bta_write
};

static int setup_ibs_files(struct super_block *sb, struct dentry *root)
{
	struct dentry *dir;
	int ret = 0;

	/* architecture specific files */
	if (create_arch_files)
		ret = create_arch_files(sb, root);

	if (ret)
		return ret;

	if (!has_ibs)
		return ret;

	/* model specific files */

	/* setup some reasonable defaults */
	ibs_config.max_cnt_fetch = 250000;
	ibs_config.fetch_enabled = 0;
	ibs_config.max_cnt_op = 250000;
	ibs_config.op_enabled = 0;
	ibs_config.dispatched_ops = 1;
	ibs_config.branch_target = 0;

	dir = oprofilefs_mkdir(sb, root, "ibs_fetch");
	oprofilefs_create_ulong(sb, dir, "enable",
				&ibs_config.fetch_enabled);
	oprofilefs_create_ulong(sb, dir, "max_count",
				&ibs_config.max_cnt_fetch);
	oprofilefs_create_ulong(sb, dir, "rand_enable",
				&ibs_config.rand_en);

	dir = oprofilefs_mkdir(sb, root, "ibs_op");
	oprofilefs_create_ulong(sb, dir, "enable",
				&ibs_config.op_enabled);
	oprofilefs_create_ulong(sb, dir, "max_count",
				&ibs_config.max_cnt_op);
	
	if (CPUID_IBS_OP_COUNTING_MODE(cpuid_ibs_id))
		oprofilefs_create_ulong(sb, dir, "dispatched_ops",
					&ibs_config.dispatched_ops);

	if (CPUID_IBS_BR_TRGT_ADDR(cpuid_ibs_id))
		oprofilefs_create_file(sb, dir, "branch_target",
					&ibs_bta_fops);
					

	return 0;
}

static int op_amd_init(struct oprofile_operations *ops)
{
	printk(KERN_DEBUG "oprofile: CAKM version %u.%u.%u\n", 
			CAKM_MAJOR, CAKM_MINOR, CAKM_MICRO);
	op_amd_set_num_perfctr();
	init_ibs();
	create_arch_files = ops->create_files;
	ops->create_files = setup_ibs_files;
	return 0;
}

static void op_amd_exit(void)
{
	ibs_exit();
}

#else

/* no IBS support */

static int op_amd_init(struct oprofile_operations *ops)
{
	op_amd_set_num_perfctr();
	return 0;
}

static void op_amd_exit(void) {}

#endif /* CONFIG_OPROFILE_IBS */

struct op_x86_model_spec op_amd_spec = {
	.num_counters		= NUM_COUNTERS,
	.num_controls		= NUM_COUNTERS,
	.num_virt_counters	= NUM_VIRT_COUNTERS,
	.reserved		= MSR_AMD_EVENTSEL_RESERVED,
	.event_mask		= OP_EVENT_MASK,
	.init			= op_amd_init,
	.exit			= op_amd_exit,
	.fill_in_addresses	= &op_amd_fill_in_addresses,
	.setup_ctrs		= &op_amd_setup_ctrs,
	.check_ctrs		= &op_amd_check_ctrs,
	.start			= &op_amd_start,
	.stop			= &op_amd_stop,
	.shutdown		= &op_amd_shutdown,
#ifdef CONFIG_OPROFILE_EVENT_MULTIPLEX
	.switch_ctrl		= &op_mux_switch_ctrl,
	.switch_start		= &op_amd_start_pmc,
	.switch_stop		= &op_amd_stop_pmc,
#endif
	.detect_ctrs		= &op_amd_detect_ctrs,

};

struct op_x86_model_spec op_amd_family15h_spec = {
	.num_counters		= NUM_COUNTERS_FAMILY15H,
	.num_controls		= NUM_COUNTERS_FAMILY15H,
	.num_virt_counters	= NUM_VIRT_COUNTERS,
	.reserved		= MSR_AMD_EVENTSEL_RESERVED,
	.event_mask		= OP_EVENT_MASK,
	.init			= op_amd_init,
	.exit			= op_amd_exit,
	.fill_in_addresses	= &op_amd_fill_in_addresses,
	.setup_ctrs		= &op_amd_setup_ctrs,
	.check_ctrs		= &op_amd_check_ctrs,
	.start			= &op_amd_start,
	.stop			= &op_amd_stop,
	.shutdown		= &op_amd_shutdown,
#ifdef CONFIG_OPROFILE_EVENT_MULTIPLEX
	.switch_ctrl		= &op_mux_switch_ctrl,
	.switch_start		= &op_amd_start_pmc,
	.switch_stop		= &op_amd_stop_pmc,
#endif
	.detect_ctrs		= &op_amd_detect_ctrs,

};
