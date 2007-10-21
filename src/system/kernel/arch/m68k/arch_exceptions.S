/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2003, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#define FUNCTION(x) .global x; .type x,@function; x
#define LOCAL_FUNCTION(x) .type x,@function; x

/*	General exception handling concept:

	The PPC architecture specifies entry point offsets for the various
	exceptions in the first two physical pages. We put a short piece of code
	(VEC_ENTRY()) into each exception vector. It calls exception_vector_common,
	which is defined in the unused space at the beginning of the first physical
	page. It re-enables address translation and calls ppc_exception_tail which
	lies in the kernel. It dumps an iframe and invokes ppc_exception_entry()
	(arch_int.cpp), which handles the exception and returns eventually.
	The registers are restored from the iframe and we return from the
	interrupt.

	algorithm overview:

	* VEC_ENTRY
	* exception_vector_common
	* ppc_exception_tail
		- dump iframe
		- ppc_exception_entry()
		- restore registers and return from interrupt

	Here we use the following SPRG registers, which are at the disposal of the
	operating system:
	* SPRG0: Physical address pointer to a struct cpu_exception_context
			 for the current CPU. The structure contains helpful pointers
			 as well as some scratch memory for temporarily saving registers.
	* SPRG1: Scratch.

	struct cpu_exception_context (defined in arch_int.h):
	offset 0:  virtual address of the exception handler routine in the kernel
	offset 4:  virtual address of the exception context
	offset 8:  kernel stack for the current thread
	offset 12: start of scratch memory for saving registers etc.

	algorithm in detail:

	* VEC_ENTRY
		- save r1 in SPRG1 and load cpu_exception_context into r1
		- save r0, save LR in r0
	* exception_vector_common
		- params:
			. r0: old LR
			. r1: exception context (physical address)
			. SPRG1: original r1
		- save r0-3
		- load virtual exception context address to r1
		- turn on BAT for exception vector code
		- turn on address translation
		- get exception vector offset from LR
	* ppc_exception_tail
		- params:
			. r1: exception context (virtual address)
			. r3: exception vector offset
			. SPRG1: original r1
		- turn off BAT
		- get kernel stack pointer
		- dump iframe
		- ppc_exception_entry()
		- restore registers and return from interrupt
 */


/* exception vector definitions */

/* code in each exception vector */
#define VEC_ENTRY() \
	mtsprg1	%r1					; /* temporarily save r1 in SPRG1 */		\
	mfsprg0	%r1					; /* ppc_cpu_exception_context* -> r1 */	\
	stw		%r0, 16(%r1)		; /* save r0 */								\
	mflr	%r0					; /* save LR in r0 */						\
	bl		exception_vector_common	; /* continue with the common part */

/* defines an exception vector */
#define DEFINE_VECTOR(offset, name) 	\
.skip	offset - (. - __irqvec_start);	\
FUNCTION(name):							\
	VEC_ENTRY()


.global __irqvec_start
__irqvec_start:
	.long	0

/* Called by the exception vector code.
 * LR:    Points to the end of the exception vector code we're coming from.
 * r0:    original LR
 * r1:    ppc_cpu_exception_context* (physical address)
 * SPRG1: original r1
 */
exception_vector_common:
	stw		%r0, 20(%r1)			/* save original LR */
	stw		%r2, 24(%r1)			/* save r2 */
	stw		%r3, 28(%r1)			/* save r3 */

	/* load the virtual address of the ppc_cpu_exception_context for this CPU */
	lwz		%r1, 4(%r1)

	/* Address translation is turned off. We map this code via BAT, turn on
	   address translation, and continue in the kernel proper. */
	li		%r0, 0x10|0x2			/* BATL_MC | BATL_PP_RW */
	mtibatl	0, %r0					/* load lower word of the instruction BAT */
	li		%r0, 0x2				/* BEPI = 0, BL = 0 (128 KB), BATU_VS */
	mtibatu	0, %r0					/* load upper word of the instruction BAT */
	isync
	sync

	/* turn on address translation */
	mfsrr1	%r0						/* load saved msr */
	rlwinm	%r0, %r0, 28, 30, 31	/* extract mmu bits */
	mfmsr	%r3						/* load the current msr */
	rlwimi  %r3, %r0, 4, 26, 27		/* merge the mmu bits with the current msr */
	li		%r0, 1
	rlwimi  %r3, %r0, 13, 18, 18	/* turn on FPU, too */
	mtmsr	%r3						/* load new msr (turning the mmu back on) */
	isync

	/* Get LR -- it points to the end of the exception vector code. We adjust it
	   to point to the beginning and can use it to identify the vector later. */
	mflr	%r3
	subi	%r3, %r3, 20		/* 5 instructions */

	/* jump to kernel code (ppc_exception_tail) */
	lwz		%r2, 0(%r1)
	mtlr	%r2
	blr


DEFINE_VECTOR(0x100, system_reset_exception)
DEFINE_VECTOR(0x200, machine_check_exception)
DEFINE_VECTOR(0x300, DSI_exception)
DEFINE_VECTOR(0x400, ISI_exception)
DEFINE_VECTOR(0x500, external_interrupt_exception)
DEFINE_VECTOR(0x600, alignment_exception)
DEFINE_VECTOR(0x700, program_exception)
DEFINE_VECTOR(0x800, FP_unavailable_exception)
DEFINE_VECTOR(0x900, decrementer_exception)
DEFINE_VECTOR(0xc00, system_call_exception)
DEFINE_VECTOR(0xd00, trace_exception)
DEFINE_VECTOR(0xe00, FP_assist_exception)
DEFINE_VECTOR(0xf00, perf_monitor_exception)
DEFINE_VECTOR(0xf20, altivec_unavailable_exception)
DEFINE_VECTOR(0x1000, ITLB_miss_exception)
DEFINE_VECTOR(0x1100, DTLB_miss_on_load_exception)
DEFINE_VECTOR(0x1200, DTLB_miss_on_store_exception)
DEFINE_VECTOR(0x1300, instruction_address_breakpoint_exception)
DEFINE_VECTOR(0x1400, system_management_exception)
DEFINE_VECTOR(0x1600, altivec_assist_exception)
DEFINE_VECTOR(0x1700, thermal_management_exception)

.global __irqvec_end
__irqvec_end:


/* This is where exception_vector_common continues. We're in the kernel here.
   r1:    ppc_cpu_exception_context* (virtual address)
   r3:    exception vector offset
   SPRG1: original r1
 */
FUNCTION(ppc_exception_tail):
	/* turn off BAT */
	li		%r2, 0
	mtibatu	0, %r2
	mtibatl	0, %r2
	isync
	sync

	/* save CR */
	mfcr	%r0

	mfsrr1	%r2					/* load saved msr */
	andi.	%r2, %r2, (1 << 14)	/* see if it was in kernel mode */
	beq		.kernel				/* yep */

	/* We come from userland. Load the kernel stack top address for the current
	   userland thread. */
	mr		%r2, %r1
	lwz		%r1, 8(%r1)
	b		.restore_stack_end

.kernel:
	mr		%r2, %r1
	mfsprg1	%r1

.restore_stack_end:
	/* now r2 points to the ppc_cpu_exception_context, r1 to the kernel stack */
	/* restore the CR, it was messed up in the previous compare */
	mtcrf	0xff, %r0

	/* align r1 to 8 bytes, so the iframe will be aligned too */
	rlwinm	%r1, %r1, 0, 0, 28

	/* save the registers */
	bl		__save_regs

	/* iframe pointer to r4 and a backup to r20 */
	mr		%r4, %r1
	mr		%r20, %r1

	/* adjust the stack pointer for ABI compatibility */
	subi	%r1, %r1, 8				/* make sure there's space for the previous
									   frame pointer and the return address */
	rlwinm	%r1, %r1, 0, 0, 27		/* 16 byte align the stack pointer */
	li		%r0, 0
	stw		%r0, 0(%r1)				/* previous frame pointer: NULL */
		/* 4(%r1) is room for the return address to be filled in by the
		   called function. */

	/* r3: exception vector offset
	   r4: iframe pointer */
	bl 		ppc_exception_entry

	/* move the iframe to r1 */
	mr		%r1, %r20

	b		__restore_regs_and_rfi


/* called by ppc_exception_tail
 * register expectations:
 *  r1:        stack
 *  r2:        ppc_cpu_exception_context*
 *  SPRG1:     original r1
 *  r0,r3, LR: scrambled, but saved in scratch memory
 * all other regs should have been unmodified by the exception handler,
 * and ready to be saved
 */
__save_regs:
	/* Note: The iframe must be 8 byte aligned. The stack pointer we are passed
	   in r1 is aligned. So we store the floating point registers first and
	   need to take care that an even number of 4 byte registers is stored,
	   or insert padding respectively. */

	/* push f0-f31 */
	stfdu	%f0, -8(%r1)
	stfdu	%f1, -8(%r1)
	stfdu	%f2, -8(%r1)
	stfdu	%f3, -8(%r1)
	stfdu	%f4, -8(%r1)
	stfdu	%f5, -8(%r1)
	stfdu	%f6, -8(%r1)
	stfdu	%f7, -8(%r1)
	stfdu	%f8, -8(%r1)
	stfdu	%f9, -8(%r1)
	stfdu	%f10, -8(%r1)
	stfdu	%f11, -8(%r1)
	stfdu	%f12, -8(%r1)
	stfdu	%f13, -8(%r1)
	stfdu	%f14, -8(%r1)
	stfdu	%f15, -8(%r1)
	stfdu	%f16, -8(%r1)
	stfdu	%f17, -8(%r1)
	stfdu	%f18, -8(%r1)
	stfdu	%f19, -8(%r1)
	stfdu	%f20, -8(%r1)
	stfdu	%f21, -8(%r1)
	stfdu	%f22, -8(%r1)
	stfdu	%f23, -8(%r1)
	stfdu	%f24, -8(%r1)
	stfdu	%f25, -8(%r1)
	stfdu	%f26, -8(%r1)
	stfdu	%f27, -8(%r1)
	stfdu	%f28, -8(%r1)
	stfdu	%f29, -8(%r1)
	stfdu	%f30, -8(%r1)
	stfdu	%f31, -8(%r1)

	/* push r0-r3 */
	lwz		%r0, 16(%r2)		/* original r0 */
	stwu	%r0, -4(%r1)		/* push r0 */
	mfsprg1	%r0					/* original r1 */
	stwu	%r0, -4(%r1)		/* push r1 */
	lwz		%r0, 24(%r2)		/* original r2 */
	stwu	%r0, -4(%r1)		/* push r2 */
	lwz		%r0, 28(%r2)		/* original r3 */
	stwu	%r0, -4(%r1)		/* push r3 */

	/* push r4-r31 */
	stwu	%r4, -4(%r1)		
	stwu	%r5, -4(%r1)
	stwu	%r6, -4(%r1)
	stwu	%r7, -4(%r1)
	stwu	%r8, -4(%r1)
	stwu	%r9, -4(%r1)
	stwu	%r10, -4(%r1)
	stwu	%r11, -4(%r1)
	stwu	%r12, -4(%r1)
	stwu	%r13, -4(%r1)
	stwu	%r14, -4(%r1)
	stwu	%r15, -4(%r1)
	stwu	%r16, -4(%r1)
	stwu	%r17, -4(%r1)
	stwu	%r18, -4(%r1)
	stwu	%r19, -4(%r1)
	stwu	%r20, -4(%r1)
	stwu	%r21, -4(%r1)
	stwu	%r22, -4(%r1)
	stwu	%r23, -4(%r1)
	stwu	%r24, -4(%r1)
	stwu	%r25, -4(%r1)
	stwu	%r26, -4(%r1)
	stwu	%r27, -4(%r1)
	stwu	%r28, -4(%r1)
	stwu	%r29, -4(%r1)
	stwu	%r30, -4(%r1)
	stwu	%r31, -4(%r1)

	/* save some of the other regs */
	mffs	%f0
	stfsu	%f0, -4(%r1)		/* push FPSCR */
	mfctr	%r0
	stwu	%r0, -4(%r1)		/* push CTR */
	mfxer	%r0
	stwu	%r0, -4(%r1)		/* push XER */
	mfcr	%r0
	stwu	%r0, -4(%r1)		/* push CR */
	lwz		%r0, 20(%r2)		/* original LR */
	stwu	%r0, -4(%r1)		/* push LR */
	mfspr	%r0, %dsisr
	stwu	%r0, -4(%r1)		/* push DSISR */
	mfspr	%r0, %dar
	stwu	%r0, -4(%r1)		/* push DAR */
	mfspr	%r0, %srr1
	stwu	%r0, -4(%r1)		/* push SRR1 */
	mfspr	%r0, %srr0
	stwu	%r0, -4(%r1)		/* push SRR0 */

	stwu	%r3, -4(%r1)		/* exception vector offset */

	blr


/* called at the tail end of each of the exceptions
 * r1: iframe pointer
 */
__restore_regs_and_rfi:
	lwzu	%r0, 4(%r1)		/* SRR0 (skip vector offset) */
	mtspr	%srr0, %r0
	lwzu	%r0, 4(%r1)		/* SRR1 */
	mtspr	%srr1, %r0
	lwzu	%r0, 4(%r1)		/* DAR */
	mtspr	%dar, %r0
	lwzu	%r0, 4(%r1)		/* DSISR */
	mtspr	%dsisr, %r0
	lwzu	%r0, 4(%r1)		/* LR */
	mtlr	%r0
	lwzu	%r0, 4(%r1)		/* CR */
	mtcr	%r0
	lwzu	%r0, 4(%r1)		/* XER */
	mtxer	%r0
	lwzu	%r0, 4(%r1)		/* CTR */
	mtctr	%r0
	lfsu	%f0, 4(%r1)		/* FPSCR */
	mtfsf	0xff, %f0

	lwzu	%r31, 4(%r1)
	lwzu	%r30, 4(%r1)
	lwzu	%r29, 4(%r1)
	lwzu	%r28, 4(%r1)
	lwzu	%r27, 4(%r1)
	lwzu	%r26, 4(%r1)
	lwzu	%r25, 4(%r1)
	lwzu	%r24, 4(%r1)
	lwzu	%r23, 4(%r1)
	lwzu	%r22, 4(%r1)
	lwzu	%r21, 4(%r1)
	lwzu	%r20, 4(%r1)
	lwzu	%r19, 4(%r1)
	lwzu	%r18, 4(%r1)
	lwzu	%r17, 4(%r1)
	lwzu	%r16, 4(%r1)
	lwzu	%r15, 4(%r1)
	lwzu	%r14, 4(%r1)
	lwzu	%r13, 4(%r1)
	lwzu	%r12, 4(%r1)
	lwzu	%r11, 4(%r1)
	lwzu	%r10, 4(%r1)
	lwzu	%r9, 4(%r1)
	lwzu	%r8, 4(%r1)
	lwzu	%r7, 4(%r1)
	lwzu	%r6, 4(%r1)
	lwzu	%r5, 4(%r1)
	lwzu	%r4, 4(%r1)
	lwzu	%r3, 4(%r1)
	
	/* Stop here, before we overwrite r1, and continue with the floating point
	   registers first. */
	addi	%r2, %r1, 16		/* skip r3-r0 */

	/* f31-f0 */
	lfd		%f31, 0(%r2)
	lfdu	%f30, 8(%r2)
	lfdu	%f29, 8(%r2)
	lfdu	%f28, 8(%r2)
	lfdu	%f27, 8(%r2)
	lfdu	%f26, 8(%r2)
	lfdu	%f25, 8(%r2)
	lfdu	%f24, 8(%r2)
	lfdu	%f23, 8(%r2)
	lfdu	%f22, 8(%r2)
	lfdu	%f21, 8(%r2)
	lfdu	%f20, 8(%r2)
	lfdu	%f19, 8(%r2)
	lfdu	%f18, 8(%r2)
	lfdu	%f17, 8(%r2)
	lfdu	%f16, 8(%r2)
	lfdu	%f15, 8(%r2)
	lfdu	%f14, 8(%r2)
	lfdu	%f13, 8(%r2)
	lfdu	%f12, 8(%r2)
	lfdu	%f11, 8(%r2)
	lfdu	%f10, 8(%r2)
	lfdu	%f9, 8(%r2)
	lfdu	%f8, 8(%r2)
	lfdu	%f7, 8(%r2)
	lfdu	%f6, 8(%r2)
	lfdu	%f5, 8(%r2)
	lfdu	%f4, 8(%r2)
	lfdu	%f3, 8(%r2)
	lfdu	%f2, 8(%r2)
	lfdu	%f1, 8(%r2)
	lfd		%f0, 8(%r2)

	/* r2-r0 */
	lwzu	%r2, 4(%r1)
	lwz		%r0, 8(%r1)
	lwz		%r1, 4(%r1)

	/* return from interrupt */
	rfi
