/*
 * Copyright 2012-2022, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@upgrade-android.com>
 *
 */

#include <arch/arm/arch_cpu_defs.h>

#include <asm_defs.h>


.macro DISABLE_INTERRUPTS
	mrs		r0, cpsr
	orr		r0, r0, #(CPSR_I | CPSR_F)
	msr		cpsr_c, r0
.endm

/* The following two macros are taken from FreeBSD... */

.macro PUSH_FRAME_IN_SVC
	stmdb	sp, {r0-r3}					/* Save 4 registers */
	mov		r0, lr						/* Save xxx32 r14 */
	mov		r1, sp						/* Save xxx32 sp */
	mrs		r3, spsr					/* Save xxx32 spsr */
	mrs		r2, cpsr					/* Get the CPSR */
	bic		r2, r2, #(CPSR_MODE_MASK)	/* Fix for SVC mode */
	orr		r2, r2, #(CPSR_MODE_SVC)
	msr		cpsr_c, r2					/* Punch into SVC mode */
	mov		r2, sp						/* Save SVC sp */
	str		r0, [sp, #-4]!				/* Push return address */
	str		lr, [sp, #-4]!				/* Push SVC lr */
	str		r2, [sp, #-4]!				/* Push SVC sp */
	msr		spsr, r3					/* Restore correct spsr */
	ldmdb	r1, {r0-r3}					/* Restore 4 regs from xxx mode */
	sub		sp, sp, #(4*15)				/* Adjust the stack pointer */
	stmia	sp, {r0-r12}				/* Push the user mode registers */
	add		r0, sp, #(4*13)				/* Adjust the stack pointer */
	stmia	r0, {r13-r14}^				/* Push the user mode registers */
	mov		r0, r0						/* NOP for previous instruction */
	mrs		r0, spsr
	str		r0, [sp, #-4]!				/* Save spsr */
.endm

.macro PULL_FRAME_FROM_SVC_AND_EXIT
	ldr		r0, [sp], #0x0004			/* Get the SPSR from stack */
	msr		spsr, r0					/* restore SPSR */
	ldmia	sp, {r0-r14}^				/* Restore registers (usr mode) */
	mov		r0, r0						/* NOP for previous instruction */
	add		sp, sp, #(4*15)				/* Adjust the stack pointer */
	ldmia	sp, {sp, lr, pc}^			/* Restore lr and exit */
.endm

/* The following two macros are adapted from the two macros above, taken from FreeBSD. */

.macro	PUSH_FRAME
	str		lr, [sp, #-4]!				/* Push the return address */
	sub		sp, sp, #(4*17)				/* Adjust the stack pointer */
	stmia	sp, {r0-r12}				/* Store the general purpose registers */
	add		r0, sp, #(4*13)				/* Adjust the stack pointer */
	stmia	r0, {r13-r14}^				/* Store the user mode sp and lr registers */
	mrs		r0, spsr					/* Store the SPSR */
	str		r0, [sp, #-4]!
	mov		r0, #0						/* Fill in svc mode sp and lr with zeroes */
	str		r0, [sp, #(4*16)]
	str		r0, [sp, #(4*17)]
.endm

.macro	PULL_FRAME_AND_EXIT
	ldr		r0, [sp], #4				/* Get SPSR from stack */
	msr		spsr, r0
	ldmia	sp, {r0-r14}^				/* Restore user mode registers */
	add		sp, sp, #(4*17)				/* Adjust the stack pointer */
	ldr		lr, [sp], #4				/* Pull the return address */
	movs	pc, lr						/* Return to user mode */
.endm

.text

.globl _vectors_start
_vectors_start:
	ldr		pc, _arm_reset
	ldr		pc, _arm_undefined
	ldr		pc, _arm_syscall
	ldr		pc, _arm_prefetch_abort
	ldr		pc, _arm_data_abort
	ldr		pc, _arm_reserved
	ldr		pc, _arm_irq
	ldr		pc, _arm_fiq


_arm_reset:
	.word	arm_reserved // actually reset, but not used when mapped

_arm_undefined:
	.word	arm_undefined

_arm_syscall:
	.word	arm_syscall

_arm_prefetch_abort:
	.word	arm_prefetch_abort

_arm_data_abort:
	.word	arm_data_abort

_arm_reserved:
	.word	arm_reserved

_arm_irq:
	.word	arm_irq

_arm_fiq:
	.word	arm_fiq


.globl _vectors_end
_vectors_end:

.data

	.rept	64
	.word	0xdeadbeef
	.endr

abort_stack:
	.word	. - 4
	.word	0xdeadbeef

	.rept	64
	.word	0xcafebabe
	.endr

irq_stack:
	.word	. - 4
	.word	0xcafebabe

	.rept	64
	.word	0xaaaabbbb
	.endr

fiq_stack:
	.word	. - 4
	.word	0xaaaabbbb

	.rept	64
	.word	0xccccdddd
	.endr

und_stack:
	.word	. - 4
	.word	0xccccdddd

.text

FUNCTION(arm_undefined):
	PUSH_FRAME_IN_SVC

	mov		r0, sp		/* iframe */
	mov		fp, r0
	bl		arch_arm_undefined

	DISABLE_INTERRUPTS
	PULL_FRAME_FROM_SVC_AND_EXIT
FUNCTION_END(arm_undefined)


FUNCTION(arm_syscall):
	PUSH_FRAME

	mov		r0, sp		/* iframe */
	mov		fp, r0
	bl		arch_arm_syscall

	DISABLE_INTERRUPTS
	PULL_FRAME_AND_EXIT
FUNCTION_END(arm_syscall)


FUNCTION(arm_prefetch_abort):
#ifdef __XSCALE__
	nop					/* Make absolutely sure any pending */
	nop					/* imprecise aborts have occurred. */
#endif
	sub		lr, lr, #4	/* Adjust LR */
	PUSH_FRAME_IN_SVC

	mov		r0, sp		/* iframe */
	mov		fp, r0
	bl		arch_arm_prefetch_abort

	DISABLE_INTERRUPTS
	PULL_FRAME_FROM_SVC_AND_EXIT
FUNCTION_END(arm_prefetch_abort)


FUNCTION(arm_data_abort):
#ifdef __XSCALE__
	nop					/* Make absolutely sure any pending */
	nop					/* imprecise aborts have occurred. */
#endif
	sub		lr, lr, #8	/* Adjust LR */
	PUSH_FRAME_IN_SVC

	mov		r0, sp		/* iframe */
	mov		fp, r0
	bl		arch_arm_data_abort

	DISABLE_INTERRUPTS
	PULL_FRAME_FROM_SVC_AND_EXIT
FUNCTION_END(arm_data_abort)


FUNCTION(arm_reserved):
	b	.
FUNCTION_END(arm_reserved)


FUNCTION(arm_irq):
	sub		lr, lr, #4	/* Adjust LR */
	PUSH_FRAME_IN_SVC

	mov		r0, sp		/* iframe */
	mov		fp, r0
	bl		arch_arm_irq

	DISABLE_INTERRUPTS
	PULL_FRAME_FROM_SVC_AND_EXIT
FUNCTION_END(arm_irq)


FUNCTION(arm_fiq):
	sub		lr, lr, #4	/* Adjust LR */
	PUSH_FRAME_IN_SVC

	mov		r0, sp		/* iframe */
	mov		fp, r0
	bl		arch_arm_fiq

	DISABLE_INTERRUPTS
	PULL_FRAME_FROM_SVC_AND_EXIT
FUNCTION_END(arm_fiq)



FUNCTION(arm_vector_init):
	mrs	r1, cpsr
	bic	r1, r1, #CPSR_MODE_MASK

	/* move into modes and set initial sp */
	mov	r0, r1
	orr	r0, r0, #CPSR_MODE_FIQ
	msr	cpsr_c, r0
	ldr r2, =fiq_stack
	ldr sp, [r2]

	mov	r0, r1
	orr	r0, r0, #CPSR_MODE_IRQ
	msr	cpsr_c, r0
	ldr r2, =irq_stack
	ldr sp, [r2]

	mov	r0, r1
	orr	r0, r0, #CPSR_MODE_ABT
	msr	cpsr_c, r0
	ldr r2, =abort_stack
	ldr sp, [r2]

	mov	r0, r1
	orr	r0, r0, #CPSR_MODE_UND
	msr	cpsr_c, r0
	ldr r2, =und_stack
	ldr sp, [r2]

	/* ... and return back to supervisor mode */
	mov	r0, r1
	orr	r0, r0, #CPSR_MODE_SVC
	msr	cpsr_c, r0

	bx	lr
FUNCTION_END(arm_vector_init)
