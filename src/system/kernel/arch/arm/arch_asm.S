/*
 * Copyright 2009, Wischert, johanneswi@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2003, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <arch/arm/arch_cpu.h>

#include <asm_defs.h>

.text



/* void arch_int_enable_interrupts(void) */
FUNCTION(arch_int_enable_interrupts):
        mrs     r0, cpsr
        bic     r0, r0, #(1<<7)         /* clear the I bit */
        msr     cpsr_c, r0
        bx      lr
FUNCTION_END(arch_int_enable_interrupts)


/* int arch_int_disable_interrupts(void) 
 */
FUNCTION(arch_int_disable_interrupts):
        mrs     r0, cpsr
        orr     r0, r0, #(1<<7)
        msr     cpsr_c, r0
        bx      lr
FUNCTION_END(arch_int_disable_interrupts)


/* void arch_int_restore_interrupts(int oldState) 
 */
FUNCTION(arch_int_restore_interrupts):
	mrs     r1, cpsr
	orr	r0,r0, #(1<<7)
	bic     r1, r1,#(1<<7) 
        orr     r1, r1, r0
        msr     cpsr_c, r1
	bx 	lr
FUNCTION_END(arch_int_restore_interrupts)


/* bool arch_int_are_interrupts_enabled(void) */
FUNCTION(arch_int_are_interrupts_enabled):
        mrs     r0, cpsr
        and     r0, r0, #(1<<7)		/*read the I bit*/
	cmp 	r0,#0
	moveq	r0,#1
	movne	r0,#0
	bx 	lr
FUNCTION_END(arch_int_are_interrupts_enabled)


