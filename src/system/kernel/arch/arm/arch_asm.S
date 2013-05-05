/*
 * Copyright 2009, Wischert, johanneswi@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2003, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <arch/arm/arch_cpu.h>

#include <asm_defs.h>

#include "asm_offsets.h"

.text


/* int mmu_read_c1(void); */
FUNCTION(mmu_read_c1):
	mrc	p15, 0, r0, c1, c0, 0
	bx	lr
FUNCTION_END(mmu_read_c1)


/* void mmu_write_c1(int val); */
FUNCTION(mmu_write_c1):
	mcr	p15, 0, r0, c1, c0, 0
	bx	lr
FUNCTION_END(mmu_write_c1)


/* NOTE: the I bit in cpsr (bit 7) is *set* to disable... */


/* void arch_int_enable_interrupts(void) */
FUNCTION(arch_int_enable_interrupts):
        mrs     r0, cpsr
        bic     r0, r0, #(1<<7)
        msr     cpsr_c, r0
        bx      lr
FUNCTION_END(arch_int_enable_interrupts)


/* int arch_int_disable_interrupts(void) */
FUNCTION(arch_int_disable_interrupts):
        mrs     r0, cpsr
        orr     r1, r0, #(1<<7)
        msr     cpsr_c, r1
        bx      lr
FUNCTION_END(arch_int_disable_interrupts)


/* void arch_int_restore_interrupts(int oldState) */
FUNCTION(arch_int_restore_interrupts):
	mrs     r1, cpsr
	and	r0, r0, #(1<<7)
	bic     r1, r1, #(1<<7) 
        orr     r1, r1, r0
        msr     cpsr_c, r1
	bx 	lr
FUNCTION_END(arch_int_restore_interrupts)


/* bool arch_int_are_interrupts_enabled(void) */
FUNCTION(arch_int_are_interrupts_enabled):
        mrs     r0, cpsr
        and     r0, r0, #(1<<7)		/*read the I bit*/
	cmp 	r0, #0
	moveq	r0, #1
	movne	r0, #0
	bx 	lr
FUNCTION_END(arch_int_are_interrupts_enabled)


/* void arm_context_switch(struct arch_thread* oldState,
	struct arch_thread* newState); */
FUNCTION(arm_context_switch):
	stmfd   sp!, { r0-r12, lr }
	str	sp, [r0]
	ldr	sp, [r1]
	ldmfd   sp!, { r0-r12, lr }
	bx	lr
FUNCTION_END(arm_context_switch)

/* addr_t arm_get_fsr(void); */
FUNCTION(arm_get_fsr):
	mrc	p15, 0, r0, c5, c0, 0		@ get FSR
	bx	lr
FUNCTION_END(arm_get_fsr)

/* addr_t arm_get_far(void); */
FUNCTION(arm_get_far):
	mrc	p15, 0, r0, c6, c0, 0		@ get FAR
	bx	lr
FUNCTION_END(arm_get_far)

/* status_t arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr_t *faultHandler) */
FUNCTION(arch_cpu_user_memcpy):
	stmfd   sp!, { r4-r6 }
	ldr	r6, [r3]
	ldr	r4, =.L_user_memcpy_error
	str	r4, [r3]	/* set fault handler */
	mov	r4, r2, lsr #2	/* size / 4 */
1:
	ldr	r5, [r1]
	str	r5, [r0]
	add	r1, #4
	add	r0, #4
	sub	r4, #1
	bne	1b
2:
	and	r4, r2, #3	/* size % 4 */
	ldrb	r5, [r1]
	strb	r5, [r0]
	add	r1, #1
	add	r0, #1
	sub	r4, #1
	bne	2b

	str	r6, [r3]	/* restore fault handler */
	mov	r0, #0
	ldmfd   sp!, { r4-r6 }
	bx	lr

.L_user_memcpy_error:
	str	r6, [r3]	/* restore fault handler */
	mov	r0, #-1

	ldmfd   sp!, { r4-r6 }
	bx	lr
FUNCTION_END(arch_cpu_user_memcpy)

/* status_t arch_cpu_user_memset(void *to, char c, size_t count, addr_t *faultHandler) */
FUNCTION(arch_cpu_user_memset):
	stmfd   sp!, { r4-r5 }
	ldr	r5, [r3]
	ldr	r4, =.L_user_memset_error
	str	r4, [r3]

	and	r1, r1, #0xff
	add	r1, r1, lsl #8
	add	r1, r1, lsl #16
	add	r1, r1, lsl #24

	mov	r4, r2, lsr #2	/* count / 4 */
1:
	str	r1, [r0]
	add	r0, r0, #4
	sub	r4, r4, #1
	bne	1b

	and	r4, r2, #3	/* count % 4 */
2:
	strb	r1, [r0]
	add	r0, r0, #1
	sub	r4, r4, #1
	bne	2b

	mov	r0, #0
	str	r5, [r3]

	ldmfd   sp!, { r4-r5 }
	bx	lr

.L_user_memset_error:
	mov	r0, #-1
	str	r5, [r3]

	ldmfd   sp!, { r4-r5 }
	bx	lr
FUNCTION_END(arch_cpu_user_memset)

/* ssize_t arch_cpu_user_strlcpy(void *to, const void *from, size_t size, addr_t *faultHandler) */
FUNCTION(arch_cpu_user_strlcpy):
	stmfd   sp!, { r4-r6 }
	ldr	r5, [r3]
	ldr	r4, =.L_user_strlcpy_error
	str	r4, [r3]
	mov	r6, #0
1:
	ldrb	r4, [r1, r6]
	strb	r4, [r0, r6]
	add	r6, r6, #1
	and	r4, #0xff	/* done yet? */
	beq	2f
	cmp	r6, r2		/* reached max length? */
	blt	1b
2:
	mov	r4, #0
	strb	r4, [r0, r6]

	mov	r0, r4		/* return B_OK */
	str	r5, [r3]	/* restore fault handler */

	ldmfd   sp!, { r4-r6 }
	bx	lr

.L_user_strlcpy_error:
	mov	r0, #-1
	str	r5, [r3]

	ldmfd   sp!, { r4-r6 }
	bx	lr
FUNCTION_END(arch_cpu_user_strlcpy)


/*!	\fn void arch_debug_call_with_fault_handler(cpu_ent* cpu,
		jmp_buf jumpBuffer, void (*function)(void*), void* parameter)

	Called by debug_call_with_fault_handler() to do the dirty work of setting
	the fault handler and calling the function. If the function causes a page
	fault, the arch_debug_call_with_fault_handler() calls longjmp() with the
	given \a jumpBuffer. Otherwise it returns normally.

	debug_call_with_fault_handler() has already saved the CPU's fault_handler
	and fault_handler_stack_pointer and will reset them later, so
	arch_debug_call_with_fault_handler() doesn't need to care about it.

	\param cpu The \c cpu_ent for the current CPU.
	\param jumpBuffer Buffer to be used for longjmp().
	\param function The function to be called.
	\param parameter The parameter to be passed to the function to be called.
*/
FUNCTION(arch_debug_call_with_fault_handler):
	stmfd   sp!, { r4, lr }

	// Set fault handler address, and fault handler stack pointer address. We
	// don't need to save the previous values, since that's done by the caller.
	ldr	r4, =1f
	str	r4, [ r0, #CPU_ENT_fault_handler ]
	str	sp, [ r0, #CPU_ENT_fault_handler_stack_pointer ]
	mov	r4, r1

	// call the function
	mov	r0, r3
	blx	r2

	// regular return
	ldmfd   sp!, { r4, lr }
	bx	lr

	// fault -- return via longjmp(jumpBuffer, 1)
1:
	mov	r0, r4
	mov	r1, #1
	bl	longjmp
FUNCTION_END(arch_debug_call_with_fault_handler)
