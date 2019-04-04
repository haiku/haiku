/*
 * Copyright 2019, Haiku, Inc. All Rights Reserved
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>
#include <arch_cpu_defs.h>


.text


#warning TODO: Fix overly simplistic IRQ

/* void arch_int_enable_interrupts(void) */
FUNCTION(arch_int_enable_interrupts):
	li	t0, ARCH_SR_SIE
	csrrs	zero, sstatus, t0
	ret
FUNCTION_END(arch_int_enable_interrupts)


/* int arch_int_disable_interrupts(void)
 */
FUNCTION(arch_int_disable_interrupts):
	li	t0, ARCH_SR_SIE
	csrrc	zero, sstatus, ARCH_SR_SIE
	ret
FUNCTION_END(arch_int_disable_interrupts)


/* void arch_int_restore_interrupts(int oldState)
 */
FUNCTION(arch_int_restore_interrupts):
	// TODO
	ret
FUNCTION_END(arch_int_restore_interrupts)


/* bool arch_int_are_interrupts_enabled(void) */
FUNCTION(arch_int_are_interrupts_enabled):
	// TODO
	ret
FUNCTION_END(arch_int_are_interrupts_enabled)


/* status_t arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr_t *faultHandler) */
FUNCTION(_arch_cpu_user_memcpy):
	// TODO
	ret
FUNCTION_END(_arch_cpu_user_memcpy)


/* status_t arch_cpu_user_memset(void *to, char c, size_t count, addr_t *faultHandler) */
FUNCTION(_arch_cpu_user_memset):
	// TODO
	ret
FUNCTION_END(_arch_cpu_user_memset)


/* ssize_t arch_cpu_user_strlcpy(void *to, const void *from, size_t size, addr_t *faultHandler) */
FUNCTION(_arch_cpu_user_strlcpy):
	// TODO
	ret
FUNCTION_END(_arch_cpu_user_strlcpy)
