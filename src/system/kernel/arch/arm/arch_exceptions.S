/*
 * Copyright 2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@upgrade-android.com>
 *
 */

#include <asm_defs.h>

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
	.word	0xaabbccdd	
	.endr
abort_stack:
	.word	.

FUNCTION(arm_undefined):
	stmfd 	sp!, { r0-r12, r14 }
	sub		sp, sp, #12
	mov		r0, sp
	mrs		r1, spsr
	stmia	r0, { r1, r13-r14 }
	b		arch_arm_undefined
	b		.

FUNCTION(arm_syscall):
	stmfd 	sp!, { r0-r12, r14 }
	sub		sp, sp, #12
	mov		r0, sp
	mrs		r1, spsr
	stmia	r0, { r1, r13-r14 }
	b		arch_arm_syscall
	b		.
	
FUNCTION(arm_prefetch_abort):
	ldr	sp, abort_stack
	stmfd 	sp!, { r0-r12, r14 }
	sub		sp, sp, #12
	mov		r0, sp
	mrs		r1, spsr
	stmia	r0, { r1, r13-r14 }
	b		arch_arm_prefetch_abort
	b		.

FUNCTION(arm_data_abort):
	ldr	sp, abort_stack
	/* XXX only deals with interrupting supervisor mode */

	/* save r4-r6 and use as a temporary place to save while we switch into supervisor mode */
	stmia	r13, { r4-r6 }
	mov	r4, r13
	sub	r5, lr, #8
	mrs	r6, spsr

	/* move into supervisor mode. irq/fiq disabled */
	msr	cpsr_c, #0x13

	/* save the return address */
	stmfd	sp!, { r5 }

	/* save C trashed regs, supervisor lr */
	stmfd	sp!, { r0-r3, r12, lr }

	/* save spsr */
	stmfd	sp!, { r6 }

	/* restore r4-r6 */
	ldmia	r4, { r4-r6 }

	/* call into higher level code */
        mrc     p15, 0, r2, c5, c0, 0           @ get FSR
        mrc     p15, 0, r3, c6, c0, 0           @ get FAR
	sub	sp, sp, #20
	mov	r0, sp /* iframe */
	stmia	r0, { r6,r2,r3,r4,r5 }
	bl	arch_arm_data_abort
	add	sp, sp, #20

	/* restore spsr */
	ldmfd	sp!, { r0 }
	msr     spsr_cxsf, r0

	/* restore back to where we came from */
	ldmfd	sp!, { r0-r3, r12, lr, pc }^

FUNCTION(arm_reserved):
	b	.
	
FUNCTION(arm_irq):
	ldr	sp, abort_stack
	/* XXX only deals with interrupting supervisor mode */

	/* save r4-r6 and use as a temporary place to save while we switch into supervisor mode */
	stmia	r13, { r4-r6 }
	mov	r4, r13
	sub	r5, lr, #4
	mrs	r6, spsr

	/* move into supervisor mode. irq/fiq disabled */
	msr	cpsr_c, #(3<<6 | 0x13)

	/* save the return address */
	stmfd	sp!, { r5 }

	/* save C trashed regs, supervisor lr */
	stmfd	sp!, { r0-r3, r12, lr }

	/* save spsr */
	stmfd	sp!, { r6 }

	/* restore r4-r6 */
	ldmia	r4, { r4-r6 }

	/* call into higher level code */
	mov	r0, sp /* iframe */
	bl	arch_arm_irq

	/* restore spsr */
	ldmfd	sp!, { r0 }
	msr     spsr_cxsf, r0

	/* restore back to where we came from */
	ldmfd	sp!, { r0-r3, r12, lr, pc }^

.bss
.align 2
	.global irq_save_spot
irq_save_spot:
	.word	0	/* r4 */
	.word	0	/* r5 */
	.word	0	/* r6 */
	
.text
FUNCTION(arm_fiq):
	ldr	sp, abort_stack
	sub	lr, lr, #4
	stmfd	sp!, { r0-r3, r12, lr }

	bl	arch_arm_fiq
	
	ldmfd	sp!, { r0-r3, r12, pc }^

