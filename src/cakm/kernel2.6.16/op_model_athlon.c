/**
 * @file op_model_athlon.h
 *
 * athlon / K7 / K8 / 
 * Family 10h/11h/12h/14h/15h 
 * model-specific MSR operations
 *
 * @remark Copyright 2002 OProfile authors
 * @remark Read the file COPYING
 *
 * @author John Levon
 * @author Philippe Elie
 * @author Graydon Hoare
 * @author Barry Kasindorf
*/

#include "oprofile.h"
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/percpu.h>

#include <asm/ptrace.h>
#include <asm/msr.h>

#include "op_x86_model.h"
#include "op_counter.h"

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

#define NUM_COUNTERS 32
#define NUM_HARDWARE_COUNTERS 4
#define NUM_HARDWARE_COUNTERS_FAMILY15H 6

#define CTR_IS_RESERVED(msrs,c) (msrs->counters[(c)].addr ? 1 : 0)
#define CTR_READ(l,h,msrs,c) do {rdmsr(msrs->counters[(c)].addr, (l), (h));} while (0)
#define CTR_WRITE_LOW(l,msrs,c) do {wrmsr(msrs->counters[(c)].addr, -(unsigned int)(l), -1);} while (0)
#define CTR_WRITE_LONG(l,h,msrs,c) do {wrmsr(msrs->counters[(c)].addr, (unsigned int)(l), (unsigned int)(h));} while (0)
#define CTR_OVERFLOWED(n) (!((n) & (1U<<31)))
/* Counter is only 48 bits */
#define CTR_OVERFLOWED_HIGH(n) (!((n) & (1U<<15)))

#define CTRL_IS_RESERVED(msrs,c) (msrs->controls[(c)].addr ? 1 : 0)
#define CTRL_READ(l,h,msrs,c) do {rdmsr(msrs->controls[(c)].addr, (l), (h));} while (0)
#define CTRL_WRITE(l,h,msrs,c) do {wrmsr(msrs->controls[(c)].addr, (l), (h));} while (0)
#define CTRL_SET_ACTIVE(n) (n |= (1<<22))
#define CTRL_SET_INACTIVE(n) (n &= ~(1<<22))
#define CTRL_CLEAR_LO(x) (x &= (1<<21))
#define CTRL_CLEAR_HI(x) (x &= 0xfffffcf0)
#define CTRL_SET_ENABLE(val) (val |= 1<<20)
#define CTRL_SET_USR(val,u) (val |= ((u & 1) << 16))
#define CTRL_SET_KERN(val,k) (val |= ((k & 1) << 17))
#define CTRL_SET_UM(val, m) (val |= (m << 8))
#define CTRL_SET_EVENT_LOW(val, e) (val |= (e & 0xff))
#define CTRL_SET_EVENT_HIGH(val, e) (val |= ((e >> 8) & 0xf))
#define CTRL_SET_HOST_ONLY(val, h) (val |= ((h & 1) << 9))
#define CTRL_SET_GUEST_ONLY(val, h) (val |= ((h & 1) << 8))

/* Definition of Family10h IBS register Addresses */
#define MSR_AMD64_IBSFETCHCTL		0xc0011030
#define MSR_AMD64_IBSFETCHLINAD		0xc0011031
#define MSR_AMD64_IBSFETCHPHYSAD	0xc0011032
#define MSR_AMD64_IBSOPCTL		0xc0011033
#define MSR_AMD64_IBSOPRIP		0xc0011034
#define MSR_AMD64_IBSOPDATA		0xc0011035
#define MSR_AMD64_IBSOPDATA2		0xc0011036
#define MSR_AMD64_IBSOPDATA3		0xc0011037
#define MSR_AMD64_IBSDCLINAD		0xc0011038
#define MSR_AMD64_IBSDCPHYSAD		0xc0011039
#define MSR_AMD64_IBSCTL		0xc001103a

/* high dword IbsFetchCtl[bit 49] */
#define IBS_FETCH_VALID_BIT		0x00020000
/* high dword IbsFetchCtl[bit 52] */
#define IBS_FETCH_PHY_ADDR_VALID_BIT 	0x00100000
#define IBS_FETCH_CTL_HIGH_MASK		0xFFFFFFFF
/* high dword IbsFetchCtl[bit 48] */
#define IBS_FETCH_ENABLE		0x00010000
#define IBS_FETCH_CTL_CNT_MASK 		0x00000000FFFF0000
#define IBS_FETCH_CTL_MAX_CNT_MASK 	0x000000000000FFFF

/*IbsOpCtl masks/bits */
#define IBS_OP_DISPATCH_CNT		(1ULL<<19)	/* bit 19 */
#define IBS_OP_VALID_BIT		(1ULL<<18)	/* bit 18 */
#define IBS_OP_ENABLE			(1ULL<<17)	/* bit 17 */

/*IbsOpData masks */
#define IBS_OP_DATA_BRANCH_MASK	   0x3F00000000		/* IbsOpData[32:37] */
#define IBS_OP_DATA_HIGH_MASK	   0x0000FFFF00000000	/* IbsOpData[32:47] */
#define IBS_OP_DATA_LOW_MASK	   0x00000000FFFFFFFF	/*IbsOpData[0:31] */

/*IbsOpData2 masks */
#define IBS_OP_DATA2_MASK	   0x000000000000002F

/*IbsOpData3 masks */
#define IBS_OP_DATA3_LS_MASK	   0x0000000003

#define IBS_OP_DATA3_PHY_ADDR_VALID_BIT 0x0000000000040000
#define IBS_OP_DATA3_LIN_ADDR_VALID_BIT 0x0000000000020000
#define IBS_CTL_LVT_OFFSET_VALID_BIT	0x100
/* AMD ext internal APIC Local Vectors */
#define APIC_IELVT			0x500
/* number of APIC Entries for ieLVT */
#define NUM_APIC_IELVT			4

/*PCI Extended Configuration Constants */
/* Northbridge Configuration Register */
#define NB_CFG_MSR            		0xC001001F
/* Bit 46, EnableCf8ExtCfg: enable CF8 extended configuration cycles */
#define ENABLE_CF8_EXT_CFG_MASK		0x4000
/* MSR to set the IBS control register APIC LVT offset */
#define IBS_LVT_OFFSET_PCI		0x1CC

/* IBS rev [bit 10] 1 = IBS Rev B */
#define IBS_REV_MASK	    		0x400

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

#define IS_OPRIP_INVALID_HIGH(x)		((x & ((u64)0x1 << 6))? 1: 0)

static unsigned long long reset_value[NUM_COUNTERS] = {0};
static DEFINE_PER_CPU(int, switch_index);
extern int ibs_allowed;		/* AMD Family 10h+ */
static int Extended_PCI_Enabled = 0;
extern u32 cpuid_ibs_id;
static unsigned int ibs_op_max_cnt;
extern unsigned long mux_norm;

/* Last enabled perf counter */
static unsigned int mux_last_cntr = 0;

/* Keep track the list of available perf counters */
static unsigned int cntr_mask = 0;


/**
 * Add an AMD IBS  sample. This may be called from any context. Pass
 * smp_processor_id() as cpu. Passes IBS registers as a unsigned int[8]
 */
void oprofile_add_ibs_op_sample(struct pt_regs * const regs,
				unsigned int * const ibs_op);

void oprofile_add_ibs_fetch_sample(struct pt_regs * const regs,
				unsigned int * const ibs_fetch);

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


void op_amd_set_num_perfctr(void)
{
	if ( boot_cpu_data.x86 == 0x15)
		op_amd_num_perfctr = NUM_HARDWARE_COUNTERS_FAMILY15H;
	else
		op_amd_num_perfctr = NUM_HARDWARE_COUNTERS;
}


/* SURAVEE: For Family15h rev A, we need to make sure that we can 
 *          disable the counter before allowing it to be enabled.
 */
static void _athlon_detect_ctrs(void * dummy)
{
	int i;
	unsigned int enable_mask = 1 << 22;
	
	cntr_mask = 0;

	for (i = 0 ; i < op_amd_num_perfctr ; i++) {
		unsigned int low = 0; 
		unsigned int low_old = 0;
		unsigned int high = 0;
		
		// Read the MSR
		rdmsr(op_amd_msr_perfctr(i), low, high);

		if (!(low & enable_mask)) {
			// Conter is disabled, so it is available.
			cntr_mask |= (1 << i);
			continue;
		}
	
		low_old = low;
			
		// Try to disable MSR
		wrmsr(op_amd_msr_perfctr(i), (low & ~enable_mask), high);

		// Read back the MSR to check
		rdmsr(op_amd_msr_perfctr(i), low, high);

		if (!(low & enable_mask)) {
			// At this point we know we can write value.
			// Therefore, the counter is available.	
			cntr_mask |= (1 << i);

			// Write back the original value	
			wrmsr(op_amd_msr_perfctr(i), low_old, high);
		}
	}

	// Replicate the mask to all the bits used for multiplexing
	for (i = op_amd_num_perfctr ; i < NUM_COUNTERS; i = i + op_amd_num_perfctr) {
		cntr_mask |= (cntr_mask << op_amd_num_perfctr);
	}
}

static unsigned int athlon_detect_ctrs(void)
{
	on_each_cpu(_athlon_detect_ctrs, NULL, 0, 1);
	return cntr_mask;
}

static void athlon_fill_in_addresses(struct op_msrs * const msrs)
{
	int i;

	for (i = 0; i < NUM_COUNTERS; i++) {
		int hw_counter = i % op_amd_num_perfctr;
		if (cntr_mask & (1 << i))
			msrs->counters[i].addr = op_amd_msr_perfctr(hw_counter);
		else
			msrs->counters[i].addr = 0;
	}

	for (i = 0; i < NUM_COUNTERS; i++) {
		int hw_control = i % op_amd_num_perfctr;
		if (cntr_mask & (1 << i))
			msrs->controls[i].addr = op_amd_msr_evntsel(hw_control);
		else
			msrs->controls[i].addr = 0;
	}
}


static void athlon_setup_ctrs(struct op_msrs const * const msrs)
{
	unsigned int low, high;
	unsigned int tmp_low, tmp_high;
	unsigned long long tmp;
	int i;
	struct op_msr *counters = msrs->counters;

	__get_cpu_var(switch_index) = 0;

	for (i = 0; i < NUM_COUNTERS; ++i) {
		if (counter_config[i].enabled){
			mux_last_cntr = i;
			reset_value[i] = -(counter_config[i].count);
			counters[i].multiplex.low  = reset_value[i];
			counters[i].multiplex.high = reset_value[i] >> 32;
		}else
			reset_value[i] = 0;
	}

	/* clear all counters */
	for (i = 0 ; i < op_amd_num_perfctr; ++i) {
		if (unlikely(!CTRL_IS_RESERVED(msrs,i)))
			continue;
		CTRL_READ(low, high, msrs, i);
		CTRL_CLEAR_LO(low);
		CTRL_CLEAR_HI(high);
		CTRL_WRITE(low, high, msrs, i);
	}

	/* avoid a false detection of ctr overflows in NMI handler */
	for (i = 0; i < op_amd_num_perfctr; ++i) {
		if (unlikely(!CTR_IS_RESERVED(msrs,i)))
			continue;
		CTR_WRITE_LOW(1, msrs, i);
	}

	/* enable active counters */
	for (i = 0; i < op_amd_num_perfctr; ++i) {
		if ((counter_config[i].enabled) && (CTR_IS_RESERVED(msrs,i))) {
			tmp = reset_value[i];
			tmp_low = tmp;
			tmp_high = (tmp >> 32);
			CTR_WRITE_LONG(tmp_low, tmp_high,msrs,i);
			CTRL_READ(low, high, msrs, i);
			CTRL_CLEAR_LO(low);
			CTRL_CLEAR_HI(high);
			CTRL_SET_ENABLE(low);
			CTRL_SET_USR(low, counter_config[i].user);
			CTRL_SET_KERN(low, counter_config[i].kernel);
			CTRL_SET_UM(low, counter_config[i].unit_mask);
			CTRL_SET_EVENT_LOW(low, counter_config[i].event);
			CTRL_SET_EVENT_HIGH(high, counter_config[i].event);
			CTRL_SET_HOST_ONLY(high, 0);
			CTRL_SET_GUEST_ONLY(high, 0);
			CTRL_WRITE(low, high, msrs, i);
		}
	}

	mux_norm = (mux_last_cntr / op_amd_num_perfctr) + 1;
}


/*
 * Quick check to see if multiplexing is necessary.
 * The check should be efficient since counters are used
 * in ordre.
 */
static int athlon_check_multiplexing(struct op_msrs const * const msrs)
{
	return mux_last_cntr >= op_amd_num_perfctr  ? 0 : -EINVAL;
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

	// Compute next bit to shift in
	bit = ((lfsr_value >> 0) ^
	       (lfsr_value >> 2) ^
	       (lfsr_value >> 3) ^
	       (lfsr_value >> 5)) & 0x0001;

	// Advance to next register value
	return ( lfsr_value = (lfsr_value >> 1) | (bit << 15) ); 
}


#define LFSR_MASK                       0xFFF

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

static void apply_lfsr_random(u32 * low, u32 * high )
{
        unsigned int ran_cur;
        u64 ctl = *high;
        ctl = (ctl << 32) | *low;

        if (0 == CPUID_IBS_RW_OP_CNTR(cpuid_ibs_id))
                return;

        /* Randomize the lower 12 bits of vaIbsOpCurCnt [51:32] */
        ran_cur = lfsr_random() & LFSR_MASK;

        // Set
        IBS_OP_CUR_CNT_SET(ctl, ran_cur);

	if (CPUID_IBS_OP_CNT_EXT(cpuid_ibs_id))
                IBS_OP_CUR_CNT_EXT_SET(ctl, ran_cur);

        *low  = (ctl & 0xFFFFFFFF);
        *high = ((ctl >> 32)& 0xFFFFFFFF);
}


static int athlon_check_ctrs(struct pt_regs * const regs,
			     struct op_msrs const * const msrs)
{
	unsigned int low, high;
	int i;
	struct ibs_fetch_sample ibs_fetch;
	struct ibs_op_sample ibs_op;

	for (i = 0 ; i < op_amd_num_perfctr ; ++i) {
		int offset = i + __get_cpu_var(switch_index);
		if (!reset_value[offset])
			continue;
		CTR_READ(low, high, msrs, i);
		if (CTR_OVERFLOWED_HIGH(high))
		{
			low = reset_value[offset];
			high = reset_value[offset] >> 32;
			oprofile_add_sample(regs, offset);
			CTR_WRITE_LONG(low, high,msrs,i);
		}
	}

	/*If AMD and IBS is available */
	if (ibs_allowed && ibs_config.FETCH_enabled) {
		low = 0;
		high = 0;
		rdmsr(MSR_AMD64_IBSFETCHCTL, low, high);
		UBTS231710_workaround_short(&high);
		if (high & IBS_FETCH_VALID_BIT) {
			ibs_fetch.ibs_fetch_ctl_high = high;
			ibs_fetch.ibs_fetch_ctl_low = low;
			rdmsr(MSR_AMD64_IBSFETCHLINAD, low, high);

			/* Force linear address in non-canonical form.
			 * (Also workaround for UBTS:228876)
			 */
			high &= ~(0xFFFF << 16);

			ibs_fetch.ibs_fetch_lin_addr_high = high;
			ibs_fetch.ibs_fetch_lin_addr_low = low;
			rdmsr(MSR_AMD64_IBSFETCHPHYSAD, low, high);
			ibs_fetch.ibs_fetch_phys_addr_high = high;
			ibs_fetch.ibs_fetch_phys_addr_low = low;

			oprofile_add_ibs_fetch_sample(regs,
						 (unsigned int *)&ibs_fetch);

			/*reenable the IRQ */
			rdmsr(MSR_AMD64_IBSFETCHCTL, low, high);
			high &= ~(IBS_FETCH_VALID_BIT);
			high |= IBS_FETCH_ENABLE;
			low &= IBS_FETCH_CTL_MAX_CNT_MASK;
			UBTS232156_workaround_short(&low);
			UBTS231710_workaround_short(&high);
			wrmsr(MSR_AMD64_IBSFETCHCTL, low, high);
		}
	}

	if (ibs_allowed && ibs_config.OP_enabled) {
		// Clear value
		low = 0;
		high = 0;

		rdmsr(MSR_AMD64_IBSOPCTL, low, high);
		if (low & IBS_OP_VALID_BIT) {
			
			/* Check if IbsOpRip is valid */
			rdmsr(MSR_AMD64_IBSOPDATA, low, high);
			if (CPUID_IBS_RIP_INVALID_CHK(cpuid_ibs_id) 
			    && IS_OPRIP_INVALID_HIGH(high))
				goto next;

			rdmsr(MSR_AMD64_IBSOPRIP, low, high);
			ibs_op.ibs_op_rip_low = low;
			ibs_op.ibs_op_rip_high = high;
			rdmsr(MSR_AMD64_IBSOPDATA, low, high);
			ibs_op.ibs_op_data1_low = low;
			ibs_op.ibs_op_data1_high = high;
			rdmsr(MSR_AMD64_IBSOPDATA2, low, high);
			ibs_op.ibs_op_data2_low = low;
			ibs_op.ibs_op_data2_high = high;
			rdmsr(MSR_AMD64_IBSOPDATA3, low, high);
			ibs_op.ibs_op_data3_low = low;
			ibs_op.ibs_op_data3_high = high;
			rdmsr(MSR_AMD64_IBSDCLINAD, low, high);
			ibs_op.ibs_dc_linear_low = low;
			ibs_op.ibs_dc_linear_high = high;
			rdmsr(MSR_AMD64_IBSDCPHYSAD, low, high);
			ibs_op.ibs_dc_phys_low = low;
			ibs_op.ibs_dc_phys_high = high;

			oprofile_add_ibs_op_sample(regs,
						 (unsigned int *)&ibs_op);
next:
			/* reenable the IRQ */
			rdmsr(MSR_AMD64_IBSOPCTL, low, high);
			low &= ~(IBS_OP_VALID_BIT);
			low |= IBS_OP_ENABLE;

			apply_lfsr_random(&low, &high);
			
			// Note: (From Paul) 
			// The erratum ERBT-1068 requires SW to clear 
			//IbsOpData3 in some BD and TN processors
			wrmsr(MSR_AMD64_IBSOPDATA3, 0, 0);


			wrmsr(MSR_AMD64_IBSOPCTL, low, high);
			rdmsr(MSR_AMD64_IBSOPRIP, low, high); // Workaround UBTS 299030
		}
	}

	/* See op_model_ppro.c */
	return 1;
}

 
static void athlon_start_pmc(struct op_msrs const * const msrs)
{
	unsigned int low = 0, high = 0;
	int i;
	for (i = 0 ; i < op_amd_num_perfctr ; ++i) {
		if (reset_value[i]) {
			CTRL_READ(low, high, msrs, i);
			CTRL_SET_ACTIVE(low);
			CTRL_WRITE(low, high, msrs, i);
		}
	}
}


static void athlon_start(struct op_msrs const * const msrs)
{
	unsigned int low = 0, high = 0;

	athlon_start_pmc(msrs);

	if (ibs_allowed && ibs_config.FETCH_enabled) {
		low = 0, 
		high = 0;
		low = (ibs_config.max_cnt_fetch >> 4) & 0xFFFF;
		high =  ((ibs_config.rand_en & 0x1) << 25)  + IBS_FETCH_ENABLE;
		wrmsr(MSR_AMD64_IBSFETCHCTL, low, high);
	}

	if (ibs_allowed && ibs_config.OP_enabled ) {
		low = 0, 
		high = 0;

		// Workaround UBTS 227027 Enable LBR
		// Should this workaround save the previous LBR state?
#define MSR_AMD64_DBG_CTL 0x000001D9
		wrmsr(MSR_AMD64_DBG_CTL, 0x01, 0x0);
		
		ibs_op_max_cnt = ibs_config.max_cnt_op;

		apply_max_cnt_compensation();

		IBS_OP_MAX_CNT_SET(low, ibs_op_max_cnt);
		low |= IBS_OP_ENABLE;
		
		if (CPUID_IBS_OP_CNT_EXT(cpuid_ibs_id))
			IBS_OP_MAX_CNT_EXT_SET(low, ibs_op_max_cnt);
		
		if (CPUID_IBS_OP_COUNTING_MODE(cpuid_ibs_id)
		&& ibs_config.dispatched_ops)
			low |= IBS_OP_DISPATCH_CNT;

		high = 0;
		wrmsr(MSR_AMD64_IBSOPCTL, low, high);
		rdmsr(MSR_AMD64_IBSOPRIP, low, high); // Workaround UBTS 299030
	}
}


static void athlon_stop_pmc(struct op_msrs const * const msrs)
{
	unsigned int low,high;
	int i;

	/* Subtle: stop on all counters to avoid race with
	 * setting our pm callback */
	for (i = 0 ; i < op_amd_num_perfctr ; ++i) {
		if (!reset_value[i + per_cpu(switch_index, smp_processor_id())])
			continue;

		CTRL_READ(low, high, msrs, i);
		CTRL_SET_INACTIVE(low);
		CTRL_WRITE(low, high, msrs, i);
	}

}


static void athlon_stop(struct op_msrs const * const msrs)
{
	unsigned int low,high;

	athlon_stop_pmc(msrs);

	if (ibs_allowed && ibs_config.FETCH_enabled) {
		low = 0;		/* clear max count and enable */
		high = 0;
		wrmsr(MSR_AMD64_IBSFETCHCTL, low, high);
	}

	if (ibs_allowed && ibs_config.OP_enabled) {
		low = 0;		/* clear max count and enable */
		high = 0;
		wrmsr(MSR_AMD64_IBSOPCTL, low, high);
		
		// Workaround UBTS 227027 Enable LBR
		// Should this workaround restore the previous LBR state?
		wrmsr(MSR_AMD64_DBG_CTL, 0x00, 0x00);
	}
}


static void athlon_switch_ctrs(struct op_msrs const * const msrs)
{
	unsigned int low, high;
	struct op_msr *counters = msrs->counters;
	
	int i, s = per_cpu(switch_index, smp_processor_id());

	athlon_stop_pmc(msrs);

	/* save the current hw counts */
	for (i = 0; i < op_amd_num_perfctr; ++i) {
		int offset = i + s;
		if (!reset_value[offset])
			continue;
		CTR_READ(low, high, msrs, i);
		counters[offset].multiplex.high = high;
		counters[offset].multiplex.low  = low;
	}

	/* move to next eventset */
	s += op_amd_num_perfctr; 

	if ((s >= NUM_COUNTERS) || s > mux_last_cntr ) {
		per_cpu(switch_index, smp_processor_id()) = 0;
		s = 0;
	} else
		per_cpu(switch_index, smp_processor_id()) = s;

	/* enable next active counters */
	for (i = 0; i < op_amd_num_perfctr; ++i) {
		int offset = i + s;
		if ((counter_config[offset].enabled) && (CTR_IS_RESERVED(msrs,i))) {
			low = counters[offset].multiplex.low;
			high= counters[offset].multiplex.high;
			CTR_WRITE_LONG(low, high,msrs,i);
			CTRL_READ(low, high, msrs, i);
			CTRL_CLEAR_LO(low);
			CTRL_CLEAR_HI(high);
			CTRL_SET_ENABLE(low);
			CTRL_SET_USR(low, counter_config[offset].user);
			CTRL_SET_KERN(low, counter_config[offset].kernel);
			CTRL_SET_UM(low, counter_config[offset].unit_mask);
			CTRL_SET_EVENT_LOW(low, counter_config[offset].event);
			CTRL_SET_EVENT_HIGH(high, counter_config[offset].event);
			CTRL_SET_HOST_ONLY(high, 0);
			CTRL_SET_GUEST_ONLY(high, 0);
			CTRL_WRITE(low, high, msrs, i);
		} 
	}
	athlon_start_pmc(msrs);
}


static void athlon_shutdown(struct op_msrs const * const msrs)
{
}

/*
 *	Enable AMD extended PCI config space thru IO
 *	save previous state
 */
static void
Enable_Extended_PCI_Config(void)
{
	unsigned int low, high;
	rdmsr(NB_CFG_MSR, low, high);
	Extended_PCI_Enabled = high  & ENABLE_CF8_EXT_CFG_MASK;
	high |= ENABLE_CF8_EXT_CFG_MASK;
	wrmsr(NB_CFG_MSR, low, high);
}

/*
 *	Disable AMD extended PCI config space thru IO
 *	restore to previous state
 */
static void
Disable_Extended_PCI_Config(void)
{
	unsigned int low, high;
	rdmsr(NB_CFG_MSR, low, high);
	high &= ~ENABLE_CF8_EXT_CFG_MASK;
	high |= Extended_PCI_Enabled;
	wrmsr(NB_CFG_MSR, low, high);
}

/*
 * Modified to use AMD extended PCI config space thru IO
 * these 2 I/Os should be atomic but there is no easy way to do that.
 * Should use the MMio version, will when it is fixed
 */

static void
PCI_Extended_Write(struct pci_dev *dev, unsigned int offset,
						 unsigned long val)
{
	outl(0x80000000 | (((offset >> 8)  & 0x0f) << 24) |
		((dev->bus->number & 0xff) << 16) | ((dev->devfn | 3) << 8)
		 | (offset & 0x0fc), 0x0cf8);

	outl(val, 0xcfc);
}

static inline void APIC_init_per_cpu(void *arg)
{
	 unsigned long i =  *(unsigned long *)arg;

	apic_write(APIC_IELVT + (i << 4), APIC_DM_NMI);
}

static inline void APIC_clear_per_cpu(void *arg)
{
	 unsigned long i =  *(unsigned long *)arg;

	apic_write(APIC_IELVT + (i << 4), APIC_LVT_MASKED);
}

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
static unsigned long ibs_eilvt_offset;

/*
 * NOTE: [Suravee] (7/22/2010)
 * We need to validate the IBS Ctrl MSRs on each core since
 * some cores might not be initialized. (ON RevA)
 * The workaround is to have that particular core reconfigure
 * the IBS Control Register through the PCI configuration space.
 */
static void ibs_ctrl_msr_validate(void * arg)
{
	unsigned long low, high;
	int i;
	int retry = 1;

	do {	
		/* Read IBS Ctrl MSR to verify IBS configuration*/
		rdmsr (MSR_AMD64_IBSCTL, low, high);
		if (low == (ibs_eilvt_offset | IBS_CTL_LVT_OFFSET_VALID_BIT))
			break;

		if (!retry) {
			printk (KERN_DEBUG "oprofile: Error! CPU %d failed to configure IBS Control Register.\n", smp_processor_id());
		}

		printk (KERN_DEBUG "oprofile: Warning! CPU %d attempting to configure IBS Control Register.\n", smp_processor_id());

		for (i = 0 ; i < pci_dev_misc_cnt ; i++) {
			/* Setup IBS Ctrl via PCI config space */
			PCI_Extended_Write(pci_dev_misc[i], IBS_LVT_OFFSET_PCI,
				(ibs_eilvt_offset | IBS_CTL_LVT_OFFSET_VALID_BIT) );
		}
		retry = 0;
	} while (1);

	//DEBUG
	//printk (KERN_DEBUG "oprofile: CPU:%d: IBS Control MSR value = %lx\n", smp_processor_id(), low);
}


/*
 * initialize the APIC for the IBS interrupts
 * if needed on AMD Family10h rev B0 and later
 */
void setup_ibs_nmi(void)
{
	struct pci_dev *gh_device = NULL;
	//u32 low, high;

	unsigned long i;
	unsigned long	apicLVT;
	int pci_device_id_misc = 0;

	for (i = 0; i < NUM_APIC_IELVT; i++) {
		/* get ieLVT contents */
		apicLVT = apic_read(APIC_IELVT + (i << 4));
		if ( (apicLVT & APIC_LVT_MASKED) != 0 ) {
			/* This slot is disabled, so we can use it */
			ibs_eilvt_offset = i;
			break;
		}
	}

	Enable_Extended_PCI_Config();

	pci_device_id_misc = get_pci_device_id_misc();
	if (0 == pci_device_id_misc) {
		printk(KERN_DEBUG "Failed to get PCI device id.");
		return;
	}

	pci_dev_misc_cnt = 0;
	/**** Be sure to run loop until NULL is returned to
	decrement reference count on any pci_dev structures returned ****/
	while ( (gh_device = pci_get_device(PCI_VENDOR_ID_AMD,
		 pci_device_id_misc, gh_device)) != NULL )
	{
		/* This code may change if we can find a proper
		 * way to get at the PCI extended config space */
		PCI_Extended_Write(gh_device, IBS_LVT_OFFSET_PCI,
				(ibs_eilvt_offset | IBS_CTL_LVT_OFFSET_VALID_BIT) );

		pci_dev_misc[pci_dev_misc_cnt++] = gh_device;	
	}

	/* per CPU IBS Control MSR validation */
	on_each_cpu (ibs_ctrl_msr_validate, NULL, 1, 1);

	Disable_Extended_PCI_Config();
	on_each_cpu(APIC_init_per_cpu, (void *)&ibs_eilvt_offset, 1, 1);
}

/*
 * unitialize the APIC for the IBS interrupts if needed on AMD Family10h
 * rev B0 and later */
void clear_ibs_nmi(void)
{
	unsigned long low, high;
	unsigned long i;
	int pci_device_id_misc = 0;
	int j;

	/*see if the IBS control register is already set */
	rdmsr(MSR_AMD64_IBSCTL, low, high);
	if ( 0 == (low & IBS_CTL_LVT_OFFSET_VALID_BIT))
	/*nothing to do it is already cleared
	 *(assume on all CPUS if any is done)
	 */
		return;

	i = low & 0x3;	//get LVT vector number

	on_each_cpu(APIC_clear_per_cpu, (void *)&i, 0, 1);
	
	Enable_Extended_PCI_Config();

	pci_device_id_misc = get_pci_device_id_misc();
	if (0 == pci_device_id_misc) {
		printk(KERN_DEBUG "Failed to get PCI device id.");
		return;
	}
	on_each_cpu(APIC_clear_per_cpu, (void *)&i, 1, 1);
	/**** Be sure to run loop until NULL is returned
	 * to decrement reference count on any pci_dev structures returned */
	for (j = 0 ; j < pci_dev_misc_cnt ; j++)
	{
		/* free the LVT entry */
		PCI_Extended_Write(pci_dev_misc[j], IBS_LVT_OFFSET_PCI, ( 0 ));
	}
	Disable_Extended_PCI_Config();
}


struct op_x86_model_spec const op_athlon_spec = {
	.num_counters = NUM_COUNTERS,
	.num_controls = NUM_COUNTERS,
	.num_hardware_counters = NUM_HARDWARE_COUNTERS,
	.num_hardware_controls = NUM_HARDWARE_COUNTERS,
	.fill_in_addresses = &athlon_fill_in_addresses,
	.setup_ctrs = &athlon_setup_ctrs,
	.check_ctrs = &athlon_check_ctrs,
	.start = &athlon_start,
	.stop = &athlon_stop,
	.shutdown = &athlon_shutdown,
	.switch_ctrs = &athlon_switch_ctrs,
	.check_multiplexing = &athlon_check_multiplexing,
	.detect_ctrs = &athlon_detect_ctrs,
};


struct op_x86_model_spec const op_amd_family15h_spec = {
	.num_counters = NUM_COUNTERS,
	.num_controls = NUM_COUNTERS,
	.num_hardware_counters = NUM_HARDWARE_COUNTERS_FAMILY15H,
	.num_hardware_controls = NUM_HARDWARE_COUNTERS_FAMILY15H,
	.fill_in_addresses = &athlon_fill_in_addresses,
	.setup_ctrs = &athlon_setup_ctrs,
	.check_ctrs = &athlon_check_ctrs,
	.start = &athlon_start,
	.stop = &athlon_stop,
	.shutdown = &athlon_shutdown,
	.switch_ctrs = &athlon_switch_ctrs,
	.check_multiplexing = &athlon_check_multiplexing,
	.detect_ctrs = &athlon_detect_ctrs,
};
