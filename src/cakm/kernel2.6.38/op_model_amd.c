/*
 * @file op_model_amd.c
 * athlon / K7 / K8 / Family 10h model-specific MSR operations
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

//TODO:
// - check BTA
// - check Family15h MUX

#include "oprofile.h"
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/percpu.h>

#include <asm/ptrace.h>
#include <asm/msr.h>
#include <asm/nmi.h>
#include <asm/apic.h>
#include <asm/processor.h>
#include <asm/cpufeature.h>

#include "op_x86_model.h"
#include "op_counter.h"
#include "../cakm_ver.h"

#if USE_INTERNAL_ERRATA
#include "../internal/errata.h"
#else
#include "../errata.h"
#endif

#define NUM_COUNTERS		4
#define NUM_COUNTERS_F15H	6
#ifdef CONFIG_OPROFILE_EVENT_MULTIPLEX
#define NUM_VIRT_COUNTERS	32
#else
#define NUM_VIRT_COUNTERS	0
#endif

#define OP_EVENT_MASK			0x0FFF
#define OP_CTR_OVERFLOW			(1ULL<<31)

#define MSR_AMD_EVENTSEL_RESERVED	((0xFFFFFCF0ULL<<32)|(1ULL<<21))

static int num_counters;
static unsigned long reset_value[OP_MAX_COUNTER];

#define IBS_FETCH_SIZE			6
#define IBS_OP_SIZE			12

static u32 ibs_caps;

struct ibs_config {
	unsigned long op_enabled;
	unsigned long fetch_enabled;
	unsigned long max_cnt_fetch;
	unsigned long max_cnt_op;
	unsigned long rand_en;
	unsigned long dispatched_ops;
	unsigned long branch_target;
};

struct ibs_state {
	u64		ibs_op_ctl;
	int		branch_target;
	unsigned long	sample_size;
};

static struct ibs_config ibs_config;
static struct ibs_state ibs_state;

/*
 * IBS cpuid feature detection
 */

#define IBS_CPUID_FEATURES		0x8000001b

/*
 * Same bit mask as for IBS cpuid feature flags (Fn8000_001B_EAX), but
 * bit 0 is used to indicate the existence of IBS.
 */
#define IBS_CAPS_AVAIL			(1U<<0)
#define IBS_CAPS_FETCHSAM		(1U<<1)
#define IBS_CAPS_OPSAM			(1U<<2)
#define IBS_CAPS_RDWROPCNT		(1U<<3)
#define IBS_CAPS_OPCNT			(1U<<4)
#define IBS_CAPS_BRNTRGT		(1U<<5)
#define IBS_CAPS_OPCNTEXT		(1U<<6)

#ifdef CA_IBS_RIPINVALIDCHK
#define IBS_CAPS_RIPINVALIDCHK		(1U<<7)
#endif

#define IBS_CAPS_DEFAULT		(IBS_CAPS_AVAIL		\
					 | IBS_CAPS_FETCHSAM	\
					 | IBS_CAPS_OPSAM)

/*
 * IBS APIC setup
 */
#define IBSCTL				0x1cc
#define IBSCTL_LVT_OFFSET_VALID		(1ULL<<8)
#define IBSCTL_LVT_OFFSET_MASK		0x0F

/*
 * IBS randomization macros
 */
#define IBS_RANDOM_BITS			12
#define IBS_RANDOM_MASK			((1ULL << IBS_RANDOM_BITS) - 1)
#define IBS_RANDOM_MAXCNT_OFFSET	(1ULL << (IBS_RANDOM_BITS - 5))

static u32 get_ibs_caps(void)
{
	u32 ibs_caps;
	unsigned int max_level;

	if (!boot_cpu_has(X86_FEATURE_IBS))
		return 0;

	/* check IBS cpuid feature flags */
	max_level = cpuid_eax(0x80000000);
	if (max_level < IBS_CPUID_FEATURES)
		return IBS_CAPS_DEFAULT;

	ibs_caps = cpuid_eax(IBS_CPUID_FEATURES);
	if (!(ibs_caps & IBS_CAPS_AVAIL))
		/* cpuid flags not valid */
		return IBS_CAPS_DEFAULT;

	return ibs_caps;
}

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

	/* Compute next bit to shift in */
	bit = ((lfsr_value >> 0) ^
	       (lfsr_value >> 2) ^
	       (lfsr_value >> 3) ^
	       (lfsr_value >> 5)) & 0x0001;

	/* Advance to next register value */
	lfsr_value = (lfsr_value >> 1) | (bit << 15);

	return lfsr_value;
}

/*
 * IBS software randomization
 *
 * The IBS periodic op counter is randomized in software. The lower 12
 * bits of the 20 bit counter are randomized. IbsOpCurCnt is
 * initialized with a 12 bit random value.
 */
static inline u64 op_amd_randomize_ibs_op(u64 val)
{
	unsigned int random = lfsr_random();

	if (!(ibs_caps & IBS_CAPS_RDWROPCNT))
		/*
		 * Work around if the hw can not write to IbsOpCurCnt
		 *
		 * Randomize the lower 8 bits of the 16 bit
		 * IbsOpMaxCnt [15:0] value in the range of -128 to
		 * +127 by adding/subtracting an offset to the
		 * maximum count (IbsOpMaxCnt).
		 *
		 * To avoid over or underflows and protect upper bits
		 * starting at bit 16, the initial value for
		 * IbsOpMaxCnt must fit in the range from 0x0081 to
		 * 0xff80.
		 */
		val += (s8)(random >> 4);
	else
		val |= (u64)(random & IBS_RANDOM_MASK) << 32;

	return val;
}

#ifdef CONFIG_CA_CSS
extern void ca_css_check_interval_and_add(struct pt_regs * const regs);
#endif

static inline void
op_amd_handle_ibs(struct pt_regs * const regs,
		  struct op_msrs const * const msrs)
{
	u64 val, ctl;
	struct op_entry entry;

	if (!ibs_caps)
		return;

	if (ibs_config.fetch_enabled) {
		rdmsrl(MSR_AMD64_IBSFETCHCTL, ctl);
		UBTS231710_workaround(&ctl);
		if (ctl & IBS_FETCH_VAL) {
			rdmsrl(MSR_AMD64_IBSFETCHLINAD, val);

			// Force linear address in non-canonical form.
			// (Also workaround for UBTS:228876)
			val &= ~(0xFFFFULL << 48);

			oprofile_write_reserve(&entry, regs, val,
					       IBS_FETCH_CODE, IBS_FETCH_SIZE);
			oprofile_add_data64(&entry, val);
			oprofile_add_data64(&entry, ctl);
			rdmsrl(MSR_AMD64_IBSFETCHPHYSAD, val);
			oprofile_add_data64(&entry, val);
			oprofile_write_commit(&entry);

			/* reenable the IRQ */
			ctl &= ~(IBS_FETCH_VAL | IBS_FETCH_CNT);
			ctl |= IBS_FETCH_ENABLE;
			
			UBTS232156_workaround(&ctl);
			UBTS231710_workaround(&ctl);
			
			wrmsrl(MSR_AMD64_IBSFETCHCTL, ctl);
		}
	}

	if (ibs_config.op_enabled) {
		rdmsrl(MSR_AMD64_IBSOPCTL, ctl);
		if (ctl & IBS_OP_VAL) {
#ifdef CA_IBS_RIPINVALIDCHK
#define IS_OPRIP_INVALID(x)		((x & ((u64)0x1 << 38))? 1: 0)
			/* Check if IbsOpRip is valid */
			rdmsrl(MSR_AMD64_IBSOPDATA, val);
			if (ibs_caps & IBS_CAPS_RIPINVALIDCHK 
			&& IS_OPRIP_INVALID(val))
				goto next;
#endif
			rdmsrl(MSR_AMD64_IBSOPRIP, val);
			oprofile_write_reserve(&entry, regs, val, IBS_OP_CODE,
					       ibs_state.sample_size);
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
			if (ibs_state.branch_target) {
				rdmsrl(MSR_AMD64_IBSBRTARGET, val);
				oprofile_add_data(&entry, (unsigned long)val);
			}
			oprofile_write_commit(&entry);
#ifdef CA_IBS_RIPINVALIDCHK
next:
#endif

			/* reenable the IRQ */
			ctl = op_amd_randomize_ibs_op(ibs_state.ibs_op_ctl);

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

	if (!ibs_caps)
		return;

	memset(&ibs_state, 0, sizeof(ibs_state));

	/*
	 * Note: Since the max count settings may out of range we
	 * write back the actual used values so that userland can read
	 * it.
	 */

	val = 0; // Suravee added
	if (ibs_config.fetch_enabled) {
		val = ibs_config.max_cnt_fetch >> 4;
		val = min(val, IBS_FETCH_MAX_CNT);
		ibs_config.max_cnt_fetch = val << 4;
		val |= ibs_config.rand_en ? IBS_FETCH_RAND_EN : 0;
		val |= IBS_FETCH_ENABLE;
		wrmsrl(MSR_AMD64_IBSFETCHCTL, val);
	}

	val = 0; // Suravee added
	if (ibs_config.op_enabled) {
		// Workaround UBTS 227027 Enable LBR
		// Should this workaround save the previous LBR state?
#define MSR_AMD64_DBG_CTL 0x000001D9
		wrmsrl(MSR_AMD64_DBG_CTL, 0x01);

		val = ibs_config.max_cnt_op >> 4;
		if (!(ibs_caps & IBS_CAPS_RDWROPCNT)) {
			/*
			 * IbsOpCurCnt not supported.  See
			 * op_amd_randomize_ibs_op() for details.
			 */
			val = clamp(val, 0x0081ULL, 0xFF80ULL);
			ibs_config.max_cnt_op = val << 4;
		} else {
			/*
			 * The start value is randomized with a
			 * positive offset, we need to compensate it
			 * with the half of the randomized range. Also
			 * avoid underflows.
			 */
			val += IBS_RANDOM_MAXCNT_OFFSET;
			if (ibs_caps & IBS_CAPS_OPCNTEXT)
				val = min(val, IBS_OP_MAX_CNT_EXT);
			else
				val = min(val, IBS_OP_MAX_CNT);
			ibs_config.max_cnt_op =
				(val - IBS_RANDOM_MAXCNT_OFFSET) << 4;
		}
		val = ((val & ~IBS_OP_MAX_CNT) << 4) | (val & IBS_OP_MAX_CNT);
		val |= ibs_config.dispatched_ops ? IBS_OP_CNT_CTL : 0;
		val |= IBS_OP_ENABLE;
		ibs_state.ibs_op_ctl = val;
		ibs_state.sample_size = IBS_OP_SIZE;
		if (ibs_config.branch_target) {
			ibs_state.branch_target = 1;
			ibs_state.sample_size++;
		}
		val = op_amd_randomize_ibs_op(ibs_state.ibs_op_ctl);
		wrmsrl(MSR_AMD64_IBSOPCTL, val);
		rdmsrl(MSR_AMD64_IBSOPRIP, val); // Workaround UBTS 299030
	}
}

static void op_amd_stop_ibs(void)
{
	if (!ibs_caps)
		return;

	if (ibs_config.fetch_enabled)
		/* clear max count and enable */
		wrmsrl(MSR_AMD64_IBSFETCHCTL, 0);

	if (ibs_config.op_enabled) {
		/* clear max count and enable */
		wrmsrl(MSR_AMD64_IBSOPCTL, 0);
		// Workaround UBTS 227027 Enable LBR
		// Should this workaround restore the previous LBR state?
		wrmsrl(MSR_AMD64_DBG_CTL, 0x00);
	}
	
}

static inline int eilvt_is_available(int offset)
{
	/* check if we may assign a vector */
	return !setup_APIC_eilvt(offset, 0, APIC_EILVT_MSG_NMI, 1);
}

static inline int ibs_eilvt_valid(void)
{
	int offset;
	u64 val;

	rdmsrl(MSR_AMD64_IBSCTL, val);
	offset = val & IBSCTL_LVT_OFFSET_MASK;

	if (!(val & IBSCTL_LVT_OFFSET_VALID)) {
		pr_err(FW_BUG "cpu %d, invalid IBS interrupt offset %d (MSR%08X=0x%016llx)\n",
		       smp_processor_id(), offset, MSR_AMD64_IBSCTL, val);
		return 0;
	}

	if (!eilvt_is_available(offset)) {
		pr_err(FW_BUG "cpu %d, IBS interrupt offset %d not available (MSR%08X=0x%016llx)\n",
		       smp_processor_id(), offset, MSR_AMD64_IBSCTL, val);
		return 0;
	}

	return 1;
}

static inline int get_ibs_offset(void)
{
	u64 val;

	rdmsrl(MSR_AMD64_IBSCTL, val);
	if (!(val & IBSCTL_LVT_OFFSET_VALID))
		return -EINVAL;

	return val & IBSCTL_LVT_OFFSET_MASK;
}

static void setup_APIC_ibs(void)
{
	int offset;

	offset = get_ibs_offset();
	if (offset < 0)
		goto failed;

	if (!setup_APIC_eilvt(offset, 0, APIC_EILVT_MSG_NMI, 0))
		return;
failed:
	pr_warn("oprofile: IBS APIC setup failed on cpu #%d\n",
		smp_processor_id());
}

static void clear_APIC_ibs(void)
{
	int offset;

	offset = get_ibs_offset();
	if (offset >= 0)
		setup_APIC_eilvt(offset, 0, APIC_EILVT_MSG_FIX, 1);
}

#ifdef CONFIG_OPROFILE_EVENT_MULTIPLEX

static void op_mux_switch_ctrl(struct op_x86_model_spec const *model,
			       struct op_msrs const * const msrs)
{
	u64 val;
	int i;

	/* enable active counters */
	for (i = 0; i < num_counters; ++i) {
		int virt = op_x86_phys_to_virt(i);
		if (!reset_value[virt])
			continue;
		rdmsrl(msrs->controls[i].addr, val);
		val &= model->reserved;
		val |= op_x86_get_ctrl(model, &counter_config[virt]);
		wrmsrl(msrs->controls[i].addr, val);
	}
}

#endif

/* functions for op_amd_spec */

static void op_amd_shutdown(struct op_msrs const * const msrs)
{
	int i;

	for (i = 0; i < num_counters; ++i) {
		if (!msrs->counters[i].addr)
			continue;
		release_perfctr_nmi(MSR_K7_PERFCTR0 + i);
		release_evntsel_nmi(MSR_K7_EVNTSEL0 + i);
	}
}

static int op_amd_fill_in_addresses(struct op_msrs * const msrs)
{
	int i;

	for (i = 0; i < num_counters; i++) {
		if (!reserve_perfctr_nmi(MSR_K7_PERFCTR0 + i))
			goto fail;
		if (!reserve_evntsel_nmi(MSR_K7_EVNTSEL0 + i)) {
			release_perfctr_nmi(MSR_K7_PERFCTR0 + i);
			goto fail;
		}
		/* both registers must be reserved */
		if (num_counters == NUM_COUNTERS_F15H) {
			msrs->counters[i].addr = MSR_F15H_PERF_CTR + (i << 1);
			msrs->controls[i].addr = MSR_F15H_PERF_CTL + (i << 1);
		} else {
			msrs->controls[i].addr = MSR_K7_EVNTSEL0 + i;
			msrs->counters[i].addr = MSR_K7_PERFCTR0 + i;
		}
		continue;
	fail:
		if (!counter_config[i].enabled)
			continue;
		op_x86_warn_reserved(i);
		op_amd_shutdown(msrs);
		return -EBUSY;
	}

	return 0;
}

static void op_amd_setup_ctrs(struct op_x86_model_spec const *model,
			      struct op_msrs const * const msrs)
{
	u64 val;
	int i;

	/* setup reset_value */
	for (i = 0; i < OP_MAX_COUNTER; ++i) {
		if (counter_config[i].enabled
		    && msrs->counters[op_x86_virt_to_phys(i)].addr)
			reset_value[i] = counter_config[i].count;
		else
			reset_value[i] = 0;
	}

	/* clear all counters */
	for (i = 0; i < num_counters; ++i) {
		if (!msrs->controls[i].addr)
			continue;
		rdmsrl(msrs->controls[i].addr, val);
		if (val & ARCH_PERFMON_EVENTSEL_ENABLE)
			op_x86_warn_in_use(i);
		val &= model->reserved;
		wrmsrl(msrs->controls[i].addr, val);
		/*
		 * avoid a false detection of ctr overflows in NMI
		 * handler
		 */
		wrmsrl(msrs->counters[i].addr, -1LL);
	}

	/* enable active counters */
	for (i = 0; i < num_counters; ++i) {
		int virt = op_x86_phys_to_virt(i);
		if (!reset_value[virt])
			continue;

		/* setup counter registers */
		wrmsrl(msrs->counters[i].addr, -(u64)reset_value[virt]);

		/* setup control registers */
		rdmsrl(msrs->controls[i].addr, val);
		val &= model->reserved;
		val |= op_x86_get_ctrl(model, &counter_config[virt]);
		wrmsrl(msrs->controls[i].addr, val);
	}

	if (ibs_caps)
		setup_APIC_ibs();
}

static void op_amd_cpu_shutdown(void)
{
	if (ibs_caps)
		clear_APIC_ibs();
}

static int op_amd_check_ctrs(struct pt_regs * const regs,
			     struct op_msrs const * const msrs)
{
	u64 val;
	int i;

	for (i = 0; i < num_counters; ++i) {
		int virt = op_x86_phys_to_virt(i);

		if (!reset_value[virt]) {
			continue;
		}
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

#ifdef CA_MUX
static void op_amd_start_pmc(struct op_msrs const * const msrs)
#else
static void op_amd_start(struct op_msrs const * const msrs)
#endif
{
	u64 val;
	int i;

	for (i = 0; i < num_counters; ++i) {
		if (!reset_value[op_x86_phys_to_virt(i)])
			continue;
		rdmsrl(msrs->controls[i].addr, val);
		val |= ARCH_PERFMON_EVENTSEL_ENABLE;
		wrmsrl(msrs->controls[i].addr, val);
	}
#ifdef CA_MUX
// NOTHING HERE
#else
	op_amd_start_ibs();
#endif
}

#ifdef CA_MUX
static void op_amd_stop_pmc(struct op_msrs const * const msrs)
#else
static void op_amd_stop(struct op_msrs const * const msrs)
#endif
{
	u64 val;
	int i;

	/*
	 * Subtle: stop on all counters to avoid race with setting our
	 * pm callback
	 */
	for (i = 0; i < num_counters; ++i) {
		if (!reset_value[op_x86_phys_to_virt(i)])
			continue;
		rdmsrl(msrs->controls[i].addr, val);
		val &= ~ARCH_PERFMON_EVENTSEL_ENABLE;
		wrmsrl(msrs->controls[i].addr, val);
	}
#ifdef CA_MUX
}

static void op_amd_start(struct op_msrs const * const msrs)
{
	op_amd_start_pmc(msrs);
	op_amd_start_ibs();
}

static void op_amd_stop(struct op_msrs const * const msrs)
{
	op_amd_stop_pmc(msrs);
#endif
	op_amd_stop_ibs();
}

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

static int setup_ibs_ctl(int ibs_eilvt_off)
{
	struct pci_dev *cpu_cfg;
	int nodes;
	int pci_device_id_misc;
	u32 value = 0;

	nodes = 0;
	cpu_cfg = NULL;
	pci_device_id_misc = get_pci_device_id_misc();

	if (pci_device_id_misc == 0) {
		printk(KERN_DEBUG "Failed to get PCI device id to configure IBS\n");
		return -ENODEV;
	}
	
	do {

		cpu_cfg = pci_get_device(PCI_VENDOR_ID_AMD,
					 pci_device_id_misc, 
					 cpu_cfg);
		if (!cpu_cfg)
			break;
		++nodes;
		pci_write_config_dword(cpu_cfg, IBSCTL, ibs_eilvt_off
				       | IBSCTL_LVT_OFFSET_VALID);
		pci_read_config_dword(cpu_cfg, IBSCTL, &value);
		if (value != (ibs_eilvt_off | IBSCTL_LVT_OFFSET_VALID)) {
			pci_dev_put(cpu_cfg);
			printk(KERN_DEBUG "Failed to setup IBS LVT offset, "
			       "IBSCTL = 0x%08x\n", value);
			return -EINVAL;
		}
	} while (1);

	if (!nodes) {
		printk(KERN_DEBUG "No CPU node configured for IBS\n");
		return -ENODEV;
	}

	return 0;
}

static int force_ibs_eilvt_setup(void)
{
	int i;
	int ret;

	/* find the next free available EILVT entry */
	for (i = 1; i < 4; i++) {
		if (!eilvt_is_available(i))
			continue;
		ret = setup_ibs_ctl(i);
		if (ret)
			return ret;
		pr_err(FW_BUG "using offset %d for IBS interrupts\n", i);
		return 0;
	}

	printk(KERN_DEBUG "No EILVT entry available\n");

	return -EBUSY;
}

static int __init_ibs_nmi(void)
{
	int ret;

	if (ibs_eilvt_valid())
		return 0;

	ret = force_ibs_eilvt_setup();
	if (ret)
		return ret;

	if (!ibs_eilvt_valid())
		return -EFAULT;

	pr_err(FW_BUG "workaround enabled for IBS LVT offset\n");

	return 0;
}

/*
 * check and reserve APIC extended interrupt LVT offset for IBS if
 * available
 *
 * init_ibs() preforms implicitly cpu-local operations, so pin this
 * thread to its current CPU
 */

static void init_ibs(void)
{
	preempt_disable();

	ibs_caps = get_ibs_caps();
	if (!ibs_caps)
		goto out;

	if (__init_ibs_nmi() < 0)
		ibs_caps = 0;
	else
		printk(KERN_INFO "oprofile: AMD IBS detected (0x%08x)\n", ibs_caps);

out:
	preempt_enable();
}

static int (*create_arch_files)(struct super_block *sb, struct dentry *root);

static int setup_ibs_files(struct super_block *sb, struct dentry *root)
{
	struct dentry *dir;
	int ret = 0;

	/* architecture specific files */
	if (create_arch_files)
		ret = create_arch_files(sb, root);

	if (ret)
		return ret;

	if (!ibs_caps)
		return ret;

	/* model specific files */

	/* setup some reasonable defaults */
	memset(&ibs_config, 0, sizeof(ibs_config));
	ibs_config.max_cnt_fetch = 250000;
	ibs_config.max_cnt_op = 250000;

	if (ibs_caps & IBS_CAPS_FETCHSAM) {
		dir = oprofilefs_mkdir(sb, root, "ibs_fetch");
		oprofilefs_create_ulong(sb, dir, "enable",
					&ibs_config.fetch_enabled);
		oprofilefs_create_ulong(sb, dir, "max_count",
					&ibs_config.max_cnt_fetch);
		oprofilefs_create_ulong(sb, dir, "rand_enable",
					&ibs_config.rand_en);
	}

	if (ibs_caps & IBS_CAPS_OPSAM) {
		dir = oprofilefs_mkdir(sb, root, "ibs_op");
		oprofilefs_create_ulong(sb, dir, "enable",
					&ibs_config.op_enabled);
		oprofilefs_create_ulong(sb, dir, "max_count",
					&ibs_config.max_cnt_op);
		if (ibs_caps & IBS_CAPS_OPCNT)
			oprofilefs_create_ulong(sb, dir, "dispatched_ops",
						&ibs_config.dispatched_ops);
		if (ibs_caps & IBS_CAPS_BRNTRGT)
			oprofilefs_create_ulong(sb, dir, "branch_target",
						&ibs_config.branch_target);
	}

	return 0;
}

struct op_x86_model_spec op_amd_spec;

static int op_amd_init(struct oprofile_operations *ops)
{
	printk(KERN_DEBUG "oprofile: CAKM version %u.%u.%u\n", 
			CAKM_MAJOR, CAKM_MINOR, CAKM_MICRO);

	init_ibs();
	create_arch_files = ops->create_files;
	ops->create_files = setup_ibs_files;

	if (boot_cpu_data.x86 == 0x15) {
		num_counters = NUM_COUNTERS_F15H;
	} else {
		num_counters = NUM_COUNTERS;
	}

	op_amd_spec.num_counters = num_counters;
	op_amd_spec.num_controls = num_counters;
	op_amd_spec.num_virt_counters = max(num_counters, NUM_VIRT_COUNTERS);

	return 0;
}

struct op_x86_model_spec op_amd_spec = {
	/* num_counters/num_controls filled in at runtime */
	.reserved		= MSR_AMD_EVENTSEL_RESERVED,
	.event_mask		= OP_EVENT_MASK,
	.init			= op_amd_init,
	.fill_in_addresses	= &op_amd_fill_in_addresses,
	.setup_ctrs		= &op_amd_setup_ctrs,
	.cpu_down		= &op_amd_cpu_shutdown,
	.check_ctrs		= &op_amd_check_ctrs,
	.start			= &op_amd_start,
	.stop			= &op_amd_stop,
	.shutdown		= &op_amd_shutdown,
#ifdef CONFIG_OPROFILE_EVENT_MULTIPLEX
	.switch_ctrl		= &op_mux_switch_ctrl,
#ifdef CA_MUX
	.switch_start		= &op_amd_start_pmc,
	.switch_stop		= &op_amd_stop_pmc,
#endif
#endif
};
