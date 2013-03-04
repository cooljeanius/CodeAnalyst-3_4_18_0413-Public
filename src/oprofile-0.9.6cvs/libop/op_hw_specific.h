/* 
 * @file architecture specific interfaces
 * @remark Copyright 2008 Intel Corporation
 * @remark Read the file COPYING
 * @author Andi Kleen
 */

#if defined(__i386__) || defined(__x86_64__) 

/* Assume we run on the same host as the profilee */

#define num_to_mask(x) ((1U << (x)) - 1)

#if defined(__i386__) && defined(__PIC__)
/* %ebx may be the PIC register.  */
	#define __cpuid(level, a, b, c, d)			\
	  __asm__ ("xchgl\t%%ebx, %1\n\t"			\
		   "cpuid\n\t"					\
		   "xchgl\t%%ebx, %1\n\t"			\
		   : "=a" (a), "=r" (b), "=c" (c), "=d" (d)	\
		   : "0" (level))
#else
	#define __cpuid(level, a, b, c, d)			\
	  __asm__ ("cpuid\n\t"					\
		   : "=a" (a), "=b" (b), "=c" (c), "=d" (d)	\
		   : "0" (level))
#endif


static inline int cpuid_vendor(char *vnd)
{
	union {
		struct {
			unsigned b,d,c;
		};
		char v[12];
	} v;
	unsigned eax;
//	asm("cpuid" : "=a" (eax), "=b" (v.b), "=c" (v.c), "=d" (v.d) : "0" (0));
	__cpuid(0, eax, v.b, v.c, v.d);
	return !strncmp(v.v, vnd, 12);
}

/* Work around Nehalem spec update AAJ79: CPUID incorrectly indicates
   unhalted reference cycle architectural event is supported. We assume
   steppings after C0 report correct data in CPUID. */
static inline void workaround_nehalem_aaj79(unsigned *_ebx)
{
	union {
		unsigned eax;
		struct {
			unsigned stepping : 4;
			unsigned model : 4;
			unsigned family : 4;
			unsigned type : 2;
			unsigned res : 2;
			unsigned ext_model : 4;
			unsigned ext_family : 8;
			unsigned res2 : 4;
		};
	} v;
	unsigned model;
	unsigned ebx, ecx, edx;

	if (!cpuid_vendor("GenuineIntel"))
		return;
//	asm("cpuid" : "=a" (v.eax) : "0" (1) : "ecx","ebx","edx");
	__cpuid(1, v.eax, ebx, ecx, edx); 
	model = (v.ext_model << 4) + v.model;
	if (v.family != 6 || model != 26 || v.stepping > 4)
		return;
	*_ebx |= (1 << 2);	/* disable unsupported event */
}

static inline unsigned arch_get_filter(op_cpu cpu_type)
{
	if (cpu_type == CPU_ARCH_PERFMON) { 
		unsigned eax, ebx, ecx, edx;
//		asm("cpuid" : "=a" (eax), "=b" (ebx) : "0" (0xa) : "ecx","edx");
		__cpuid(0xa, eax, ebx, ecx, edx); 
		workaround_nehalem_aaj79(&ebx);
		return ebx & num_to_mask(eax >> 24);
	}
	return -1U;
}

static inline int arch_num_counters(op_cpu cpu_type) 
{
	if (cpu_type == CPU_ARCH_PERFMON) {
		unsigned eax, ebx, ecx, edx;
//		asm("cpuid" : "=a" (eax) : "0" (0xa) : "ebx","ecx","edx");
		__cpuid(0xa, eax, ebx, ecx, edx); 
		return (eax >> 8) & 0xff;
	} 
	return -1;
}

static inline unsigned arch_get_counter_mask(void)
{
	unsigned eax, ebx, ecx, edx;
//	asm("cpuid" : "=a" (eax) : "0" (0xa) : "ebx","ecx","edx");
	__cpuid(0xa, eax, ebx, ecx, edx); 
	return num_to_mask((eax >> 8) & 0xff);	
}

#else

static inline unsigned arch_get_filter(op_cpu cpu_type)
{
	/* Do something with passed arg to shut up the compiler warning */
	if (cpu_type != CPU_NO_GOOD)
		return 0;
	return 0;
}

static inline int arch_num_counters(op_cpu cpu_type) 
{
	/* Do something with passed arg to shut up the compiler warning */
	if (cpu_type != CPU_NO_GOOD)
		return -1;
	return -1;
}

static inline unsigned arch_get_counter_mask(void)
{
	return 0;
}

#endif
