/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <arch/arm/arch_cpu.h>
#include <asm_defs.h>

.text

/* status_t arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr_t *faultHandler) */
FUNCTION(_arch_cpu_user_memcpy):
	mov		x0, xzr
	ret
FUNCTION_END(_arch_cpu_user_memcpy)

/* status_t arch_cpu_user_memset(void *to, char c, size_t count, addr_t *faultHandler) */
FUNCTION(_arch_cpu_user_memset):
	mov		x0, xzr
	ret
FUNCTION_END(_arch_cpu_user_memset)

/* ssize_t arch_cpu_user_strlcpy(void *to, const void *from, size_t size, addr_t *faultHandler) */
FUNCTION(_arch_cpu_user_strlcpy):
	mov		x0, xzr
	ret
FUNCTION_END(_arch_cpu_user_strlcpy)

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
	mov		x0, xzr
	ret
FUNCTION_END(arch_debug_call_with_fault_handler)
