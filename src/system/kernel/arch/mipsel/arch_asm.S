/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2009 Wischert, johanneswi@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2003, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <arch/mipsel/arch_cpu.h>

#include <asm_defs.h>

.text


/* void arch_int_enable_interrupts(void) */
FUNCTION(arch_int_enable_interrupts):
	nop
FUNCTION_END(arch_int_enable_interrupts)


/* int arch_int_disable_interrupts(void) 
 */
FUNCTION(arch_int_disable_interrupts):
	nop
FUNCTION_END(arch_int_disable_interrupts)


/* void arch_int_restore_interrupts(int oldState) 
 */
FUNCTION(arch_int_restore_interrupts):
	nop
FUNCTION_END(arch_int_restore_interrupts)


/* bool arch_int_are_interrupts_enabled(void) */
FUNCTION(arch_int_are_interrupts_enabled):
	nop
FUNCTION_END(arch_int_are_interrupts_enabled)

