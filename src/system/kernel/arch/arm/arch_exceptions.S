/*
 * Copyright 2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@upgrade-android.com>
 *
 */

#include <asm_defs.h>

#define CPSR_MODE_MASK	0x1f
#define CPSR_MODE_USR	0x10
#define CPSR_MODE_FIQ	0x11
#define CPSR_MODE_IRQ	0x12
#define CPSR_MODE_SVC	0x13
#define CPSR_MODE_ABT	0x17
#define CPSR_MODE_UND	0x1b

/* The following two macros are taken from FreeBSD... */

	.macro PUSHFRAMEINSVC
	stmdb   sp, {r0-r3}		/* Save 4 registers */
	mov     r0, lr			/* Save xxx32 r14 */
	mov     r1, sp			/* Save xxx32 sp */
	mrs     r3, spsr		/* Save xxx32 spsr */
	mrs     r2, cpsr		/* Get the CPSR */
	bic     r2, r2, #(CPSR_MODE_MASK)/* Fix for SVC mode */
	orr     r2, r2, #(CPSR_MODE_SVC)
	msr     cpsr_c, r2		/* Punch into SVC mode */
	mov     r2, sp			/* Save SVC sp */
	str     r0, [sp, #-4]!		/* Push return address */
	str     lr, [sp, #-4]!		/* Push SVC lr */
	str     r2, [sp, #-4]!		/* Push SVC sp */
	msr     spsr_all, r3		/* Restore correct spsr */
	ldmdb   r1, {r0-r3}		/* Restore 4 regs from xxx mode */
	sub     sp, sp, #(4*15)		/* Adjust the stack pointer */
	stmia   sp, {r0-r12}		/* Push the user mode registers */
	add     r0, sp, #(4*13)		/* Adjust the stack pointer */
	stmia   r0, {r13-r14}^		/* Push the user mode registers */
	mov     r0, r0			/* NOP for previous instruction */
	mrs     r0, spsr_all
	str     r0, [sp, #-4]!		/* Save spsr */
	.endm

	.macro PULLFRAMEFROMSVCANDEXIT
	ldr     r0, [sp], #0x0004	/* Get the SPSR from stack */
	msr     spsr_all, r0		/* restore SPSR */
	ldmia   sp, {r0-r14}^		/* Restore registers (usr mode) */
	mov     r0, r0			/* NOP for previous instruction */
	add     sp, sp, #(4*15)		/* Adjust the stack pointer */
	ldmia   sp, {sp, lr, pc}^	/* Restore lr and exit */
	.endm

	.text

.globl _vectors_start
_vectors_start:
        ldr		pc, _arm_reset  
        ldr             pc, _arm_undefined
        ldr             pc, _arm_syscall
        ldr             pc, _arm_prefetch_abort
        ldr             pc, _arm_data_abort
        ldr             pc, _arm_reserved
        ldr             pc, _arm_irq
        ldr             pc, _arm_fiq

_arm_reset:
	.word		arm_reserved // actually reset, but not used when mapped
_arm_undefined: 
	.word		arm_undefined
_arm_syscall:
	.word		arm_syscall
_arm_prefetch_abort:
	.word		arm_prefetch_abort
_arm_data_abort:
	.word		arm_data_abort
_arm_reserved:
	.word		arm_reserved
_arm_irq:       
	.word		arm_irq
_arm_fiq:       
	.word		arm_fiq 
.globl _vectors_end
_vectors_end:


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


FUNCTION(arm_undefined):
	PUSHFRAMEINSVC

	mov	r0, sp
	bl	arch_arm_undefined

	PULLFRAMEFROMSVCANDEXIT
FUNCTION_END(arm_undefined)


FUNCTION(arm_syscall):
	PUSHFRAMEINSVC

	mov	r0, sp
	bl	arch_arm_syscall

	PULLFRAMEFROMSVCANDEXIT
FUNCTION_END(arm_syscall)


FUNCTION(arm_prefetch_abort):
#ifdef __XSCALE__
	nop				/* Make absolutely sure any pending */
	nop				/* imprecise aborts have occurred. */
#endif
 	add	lr, lr, #4
	PUSHFRAMEINSVC

	mov	r0, sp
	bl	arch_arm_prefetch_abort

	PULLFRAMEFROMSVCANDEXIT
FUNCTION_END(arm_prefetch_abort)


FUNCTION(arm_data_abort):
#ifdef __XSCALE__
	nop                             /* Make absolutely sure any pending */
	nop                             /* imprecise aborts have occurred. */
#endif
	sub	lr, lr, #8		/* Adjust the lr */
	PUSHFRAMEINSVC

	mov	r0, sp
	bl	arch_arm_data_abort

	PULLFRAMEFROMSVCANDEXIT


FUNCTION(arm_reserved):
	b	.
FUNCTION_END(arm_reserved)

FUNCTION(arm_irq):
	sub	lr, lr, #4
	PUSHFRAMEINSVC

	mov	r0, sp /* iframe */
	bl	arch_arm_irq

	PULLFRAMEFROMSVCANDEXIT
FUNCTION_END(arm_irq)


FUNCTION(arm_fiq):
	sub	lr, lr, #4
	PUSHFRAMEINSVC

	mov	r0, sp /* iframe */
	bl	arch_arm_fiq
	
	PULLFRAMEFROMSVCANDEXIT
FUNCTION_END(arm_fiq)



FUNCTION(arm_vector_init):
	mrs	r1, cpsr
	bic	r1, r1, #CPSR_MODE_MASK

	/* move into modes and set initial sp */
	mov	r0, r1
	orr	r0, r0, #CPSR_MODE_FIQ
	msr	cpsr_c, r0
	ldr	sp, fiq_stack

	mov	r0, r1
	orr	r0, r0, #CPSR_MODE_IRQ
	msr	cpsr_c, r0
	ldr	sp, irq_stack

	mov	r0, r1
	orr	r0, r0, #CPSR_MODE_ABT
	msr	cpsr_c, r0
	ldr	sp, abort_stack

	mov	r0, r1
	orr	r0, r0, #CPSR_MODE_UND
	msr	cpsr_c, r0
	ldr	sp, und_stack

	/* ... and return back to supervisor mode */
	mov	r0, r1
	orr	r0, r0, #CPSR_MODE_SVC
	msr	cpsr_c, r0

	bx	lr
FUNCTION_END(arm_vector_init)
