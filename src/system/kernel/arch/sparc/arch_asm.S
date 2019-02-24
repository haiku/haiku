/*
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


.text


/* void arch_int_enable_interrupts(void) */
FUNCTION(arch_int_enable_interrupts):
	rdpr %pstate, %o0
	or   %o0, 2, %o0
	wrpr %o0, 0, %pstate
	ret
FUNCTION_END(arch_int_enable_interrupts)


/* int arch_int_disable_interrupts(void)
 */
FUNCTION(arch_int_disable_interrupts):
	rdpr %pstate, %o1
	// Set output register to previous state
	and %o1, 2, %o0
	// Clear interrupt bit
	andn %o1, 2, %o1

	wrpr %o1, 0, %pstate
	ret
FUNCTION_END(arch_int_disable_interrupts)


/* void arch_int_restore_interrupts(int oldState)
 */
FUNCTION(arch_int_restore_interrupts):
	rdpr %pstate, %o1
	// Clear interrupt bit
	andn %o1, 2, %o1
	// Restore previous interript bit state
	or %o1, %o0, %o1
	wrpr %o1, 0, %pstate
	ret
FUNCTION_END(arch_int_restore_interrupts)


/* bool arch_int_are_interrupts_enabled(void) */
FUNCTION(arch_int_are_interrupts_enabled):
	rdpr %pstate, %o0
	andn %o0, 2, %o0
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
