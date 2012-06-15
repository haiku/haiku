/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <debug.h>

#include <arch/debug.h>


void
arch_debug_save_registers(struct arch_debug_registers* registers)
{

}


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


int32
arch_debug_get_stack_trace(addr_t* returnAddresses, int32 maxCount,
	int32 skipIframes, int32 skipFrames, uint32 flags)
{
	return 0;
}


void*
arch_debug_get_interrupt_pc(bool* _isSyscall)
{
	return NULL;
}


void
arch_debug_unset_current_thread(void)
{

}


bool
arch_is_debug_variable_defined(const char* variableName)
{
	return false;
}


status_t
arch_set_debug_variable(const char* variableName, uint64 value)
{
	return B_OK;
}


status_t
arch_get_debug_variable(const char* variableName, uint64* value)
{
	return B_OK;
}


ssize_t
arch_debug_gdb_get_registers(char* buffer, size_t bufferSize)
{
	return B_ERROR;
}


status_t
arch_debug_init(kernel_args *args)
{
	return B_OK;
}

void
arch_debug_call_with_fault_handler(cpu_ent* cpu, jmp_buf jumpBuffer,
	void (*function)(void*), void* parameter)
{
	// To be implemented in asm, not here.
}
