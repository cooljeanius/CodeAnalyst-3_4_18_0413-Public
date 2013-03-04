#if defined (CONFIG_CA_CSS)

#include <linux/version.h>
#include <linux/mm.h>
#include <linux/mm.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/dcookies.h>
#include <linux/profile.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/highmem.h>

#include <asm/uaccess.h>

#include <linux/dcookies.h>
#include <linux/fs.h>

#include "oprofile.h"
#include "op_ca_css.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0) 
static unsigned long
copy_from_user_nmi(void *to, const void __user *from, unsigned long n);
#endif

extern unsigned long oprofile_started;

static int get_address_range(unsigned long addr, struct mm_struct *mm, 
			unsigned long * start, unsigned long * end )
{
	struct vm_area_struct * vma = NULL;
	if (!start || !end)
		return 0;

	/* Lookup the memory mapping for the address range */
	for (vma = find_vma(mm, addr); vma != NULL; vma = vma->vm_next) {
		if (addr < vma->vm_start || addr >= vma->vm_end)
			continue;
		
		*start = vma->vm_start;
		*end   = vma->vm_end;
		break;
	}
	
	//printk ("DEBUG:addr %lx, start %lx, end %lx, size = %d\n", 
	//		addr, *start, *end, (*end)-(*start));	

	return (end - start);
}


static unsigned long try_match_retAddr_with_dcookie(unsigned long ra, struct mm_struct *mm) 
{
	unsigned long cookie = 0;
	struct vm_area_struct * vma = NULL;
	struct path * path = NULL;

	/* Lookup dcookie from return address */
	for (vma = find_vma(mm, ra); vma != NULL; vma = vma->vm_next) {
		if (ra < vma->vm_start || ra >= vma->vm_end)
			continue;

		// If not map to file, we don't care 
		if (!vma->vm_file)
			continue;
		
		//if (!(vma->vm_flags & VM_EXECUTABLE))
		//	continue;

		// This is same as fast_get_dcookie
		path = &(vma->vm_file->f_path);
		// DCACHE_COOCKE only available from 2.6.29 kernel forward
		if (path->dentry->d_flags & DCACHE_COOKIE)
			cookie = (unsigned long) path->dentry;
		else
			get_dcookie(path, &cookie);
		break;
	}
			
	//if (cookie) printk("OK: -------- Cookie found for ra = %lx\n", ra);
	
	return cookie;
}


#if CONFIG_VERIFY_NEARCALL
/*
 * Call Instructions
 *
 * Mnemonic Opcode Description
 * CALL rel16off 		E8 iw 
 * (5: op + ModRM + SIB + 16-bit)
 * 	Near call with the target specified by a 16-bit relative displacement.
 *
 * CALL rel32off 		E8 id 
 * (7: op + ModRM + SIB + 32-bit))
 * 	Near call with the target specified by a 32-bit relative displacement.
 * 
 * CALL reg/mem16 		FF /2 
 * (2: op + ModRM) 
 * (5: op + ModRM + SIB + 16-bit)
 * 	Near call with the target specified by reg/mem16.
 * 
 * CALL reg/mem32 		FF /2 
 * (2: op + ModRM)
 * (7: op + ModRM + SIB + 32-bit)
 * 	Near call with the target specified by reg/mem32. 
 * 	(There is no prefix for encoding this in 64-bit mode.)
 * 
 * CALL reg/mem64 		FF /2 
 * (2 : op + ModRM)
 * (11: op + ModRM + SIB + 64-bit)
 * 	Near call with the target specified by reg/mem64.
 * 
 * - reg/mem16	Word (16-bit) operand in a GPR register or memory.
 * - reg/mem32	Doubleword (32-bit) operand in a GPR register or memory.
 * - reg/mem64	Quadword (64-bit) operand in a GPR register or memory.
 * - rel16off	Signed 16-bit offset relative to the instruction pointer.
 * - rel32off	Signed 32-bit offset relative to the instruction pointer.
 *
 * - ib, iw, id, iq—Specifies an immediate-operand value. The opcode determines 
 *   whether the value is signed or unsigned. The value following the opcode, 
 *   ModRM, or SIB byte is either one byte (ib), two bytes (iw), or four bytes 
 *   (id). Word and doubleword values start with the low-order byte.
 *
 * - /digit—Indicates that the ModRM byte specifies only one register or 
 *   memory (r/m) operand. The digit is specified by the ModRM reg field and 
 *   is used as an instruction-opcode extension. Valid digit values range from 0 to 7.
 */
static int is_near_call_retaddr(unsigned long ra)
{
	unsigned char * byte = (unsigned char *) ra;
	unsigned int opcode = 0; 
	unsigned int ModRM  = 0; 
	unsigned char tmp [11] = {0};
	
	if (0 == copy_from_user_nmi (tmp, byte-11, 11))
		return -1;

	/* 11-byte */	
	opcode = tmp[0]; 
	ModRM  = tmp[1];
 	/* CALL mem64	FF /2  */
	if (opcode == 0xFF && (ModRM & 0x38) == 2)
		goto out;

	/* 7-byte */	
	opcode = tmp[4];
	ModRM  = tmp[5];
 	/* CALL mem32	FF /2  */
	if (opcode == 0xFF && (ModRM & 0x38) == 2)
		goto out;

 	/* CALL rel32off 	E8 id */
	if (opcode == 0xE8)
		goto out;
	
	/* 5-byte */	
	opcode = tmp[6];
	ModRM  = tmp[7];
 	/* CALL mem16	FF /2  */
	if (opcode == 0xFF && (ModRM & 0x38) == 2)
		goto out;
	
	/* CALL rel16off 	E8 iw  */
	if (opcode == 0xE8)
		goto out;

	/* 2-byte */	
	opcode = tmp[9];
	ModRM  = tmp[10];
	 /* CALL reg 	FF /2 */
	if (opcode == 0xFF && (ModRM & 0x38) == 2)
		goto out;

	return -1;
out:
	return 0;
}
#endif
 
#if CONFIG_VERIFY_FARCALL
/*
 * Far Call Instructions
 * 
 * CALL FAR pntr16:16 		9A cd 
 * (5: Op + 16-bit + 16-bit)
 * 	Far call direct, with the target specified by a far pointer contained 
 * 	in the instruction. (INVALID IN 64-BIT MODE.)
 * 
 * CALL FAR pntr16:32 		9A cp 
 * (7: Op + 16-bit + 32-bit)
 * 	Far call direct, with the target specified by a far pointer contained 
 * 	in the instruction. (INVALID IN 64-BIT MODE.)
 * 
 * CALL FAR mem16:16 		FF /3 
 * (6: Op + ModRM + 16-bit + 16-bit)
 * 	Far call indirect, with the target specified by a far pointer in memory.
 * 
 * CALL FAR mem16:32 		FF /3 
 * (8: Op + ModRM + 32-bit + 16-bit)
 * 	Far call indirect, with the target specified by a far pointer in memory.
 *
 *
 * - pntr16:16	Far pointer with 16-bit selector and 16-bit offset.
 * - pntr16:32	Far pointer with 16-bit selector and 32-bit offset.
 * - mem16:16	Two sequential word (16-bit) operands in memory.
 * - mem16:32	A doubleword (32-bit) operand followed by a word (16-bit) operand in memory.
 *
 * - cb, cw, cd, cp—Specifies a code-offset value and possibly a new code-segment 
 *   register value. The value following the opcode is either one byte (cb), 
 *   two bytes (cw), four bytes (cd), or six bytes (cp).
 */
//TODO: Verify this
static int is_far_call_retaddr(unsigned long ra)
{
	unsigned char * byte = (unsigned char *) ra;
	unsigned int opcode = 0; 
	unsigned int ModRM  = 0; 
	unsigned char tmp [8] = {0};
	
	if (0 == copy_from_user_nmi (tmp, byte-8, 8))
		return -1;

	/* 8-byte */	
	opcode = tmp[0];
	ModRM  = tmp[1];
 	/* CALL FAR mem16:32 	FF /3 */
	if (opcode == 0xFF && (ModRM & 0x38) == 3)
		goto out;

	/* 7-byte */	
	opcode = tmp[1];
 	/* CALL FAR pntr16:32 	9A cp  */
	if (opcode == 0x9A)
		goto out;
	
	/* 6-byte */	
	opcode = tmp[2];
	ModRM  = tmp[3];
 	/* CALL FAR mem16:16 	FF /3 */
	if (opcode == 0xFF && (ModRM & 0x38) == 3)
		goto out;

	/* 5-byte */	
	opcode = tmp[3];
	ModRM  = tmp[4];
 	/* CALL FAR pntr16:16 	9A cd */
	if (opcode == 0x9A)
		goto out;

	return -1;
out:
	return 0;
}
#endif

static int is_call_retaddr(unsigned long ra)
{
	int ret = 0;
#if CONFIG_VERIFY_NEARCALL
	if (0 != is_near_call_retaddr(ra))
#endif
#if CONFIG_VERIFY_FARCALL
		ret = is_far_call_retaddr(ra);
#else
		ret = -1;
#endif
	return ret;
}

static void dump_user_ca_css(struct pt_regs *regs, unsigned int depth, 
			struct task_struct * task)
{
	struct mm_struct *mm = NULL;
	unsigned long ra = 0;		/* Return address */
	unsigned long sa = 0;		/* Current stack address */
	unsigned long sa_top = 0;	/* Top of the stack address range */
	unsigned long sa_bot = 0;	/* Bottom of the stack address range */ 
	unsigned int d = depth;		/* Current depth */
	unsigned int data_size = sizeof(unsigned long);

	if (!depth)
		return;

	/* NOTE: [Suravee]
	 * Sometimes cannot use get_task_mm since it calls "might_sleep".
	 * This triggers the "Scheduling while atomic" kernel
	 * debug message.
	 * mm = task->mm; //mm = get_task_mm(task);
	 */
	mm = get_task_mm(task);
	if (!mm)
		return; 

	down_read(&mm->mmap_sem);

	sa = (unsigned long) regs->sp;

#ifndef __i386__
	if (ca_css_bitness) 
		data_size = sizeof(unsigned int);
#endif

	/* Get bottom of the stack */
	if (mm->start_stack != 0)
		sa_bot = mm->start_stack;
	else {
		/* Get current stack address range */
		if (get_address_range(sa, mm, &sa_top, &sa_bot) == 0)
			return;	
	}
		
	/* Stack Unwinding Logic:
	 * We start from the top of the stack (lower addr) and unwind each entry
	 * based on the data size (bitness) and try to look for the
	 * return address by checking it against dcookie value.
	 */
//	printk("DEBUG: sa = %lx, sa_bot = %lx, sa_top = %lx, data_size = %u\n", 
//		sa, sa_bot, sa_top, data_size);
	for (; sa < sa_bot; sa += data_size) {

		if (0 == copy_from_user_nmi (&ra, (unsigned long*)sa, data_size))
			break; 

		/* Rule out non-return-address value */
		if (ra < 20) /* Why 20? */
			continue;

		if (try_match_retAddr_with_dcookie(ra,mm) != 0
		&&  is_call_retaddr(ra) == 0) {
			oprofile_add_trace(ra);
			if(--d == 0)
				break;
		}
	}

	up_read(&mm->mmap_sem); //mmput(mm);
}

void
x86_ca_css(struct pt_regs * const regs, unsigned int depth, struct task_struct * task)
{

	if (user_mode_vm(regs) && depth > 0) {
		dump_user_ca_css(regs, depth, task);
	}

	return;
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0) 
// NOTE: This code is copied from mm/gup.c
/*
 * best effort, GUP based copy_from_user() that assumes IRQ or NMI context
 */
static unsigned long
copy_from_user_nmi(void *to, const void __user *from, unsigned long n)
{
        unsigned long offset, addr = (unsigned long)from;
        unsigned long size, len = 0;
        struct page *page;
        void *map;
        int ret;

        do {
                ret = __get_user_pages_fast(addr, 1, 0, &page);
                if (!ret)
                        break;

                offset = addr & (PAGE_SIZE - 1);
                size = min(PAGE_SIZE - offset, n - len);

                map = kmap_atomic(page, KM_UML_USERCOPY);
                memcpy(to, map+offset, size);
                kunmap_atomic(map, KM_UML_USERCOPY);
                put_page(page);

                len  += size;
                to   += size;
                addr += size;

        } while (len < n);

        return len;
}
#endif
#endif // CONFIG_CA_CSS
