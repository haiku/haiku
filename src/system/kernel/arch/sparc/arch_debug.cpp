/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Adrien Destugues <pulkomandy@pulkomandy.tk>
 */


#include <arch/debug.h>


void
arch_debug_stack_trace(void)
{
}


bool
arch_debug_contains_call(Thread *thread, const char *symbol,
	addr_t start, addr_t end)
{
	return false;
}


void *
arch_debug_get_caller(void)
{
	return NULL;
}


void
arch_debug_save_registers(struct arch_debug_registers* registers)
{
}


void
arch_debug_call_with_fault_handler(cpu_ent* cpu, jmp_buf jumpBuffer,
	void (*function)(void*), void* parameter)
{
}


bool
arch_is_debug_variable_defined(const char* variableName)
{
	// TODO: Implement!
	return false;
}


status_t
arch_set_debug_variable(const char* variableName, uint64 value)
{
	// TODO: Implement!
	return B_ENTRY_NOT_FOUND;
}


status_t
arch_get_debug_variable(const char* variableName, uint64* value)
{
	// TODO: Implement!
	return B_ENTRY_NOT_FOUND;
}


int32
arch_debug_get_stack_trace(addr_t* returnAddresses, int32 maxCount,
	int32 skipIframes, int32 skipFrames, uint32 flags)
{
	return 0;
}


ssize_t
arch_debug_gdb_get_registers(char* buffer, size_t bufferSize)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


void*
arch_debug_get_interrupt_pc(bool* _isSyscall)
{
	// TODO: Implement!
	return NULL;
}


status_t
arch_debug_init(kernel_args *args)
{
#if 0
	add_debugger_command("where", &stack_trace, "Same as \"sc\"");
	add_debugger_command("bt", &stack_trace, "Same as \"sc\" (as in gdb)");
	add_debugger_command("sc", &stack_trace, "Stack crawl for current thread");
#endif

	return B_NO_ERROR;
}

