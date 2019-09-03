/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <arch/debug.h>

#include <arch_cpu.h>
#include <debug.h>
#include <debug_heap.h>
#include <elf.h>
#include <kernel.h>
#include <kimage.h>
#include <thread.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>


void
arch_debug_save_registers(struct arch_debug_registers* registers)
{
}


bool
arch_debug_contains_call(Thread *thread, const char *symbol,
	addr_t start, addr_t end)
{
	return false;
}


void
arch_debug_stack_trace(void)
{
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


bool
arch_is_debug_variable_defined(const char* variableName)
{
	return false;
}


status_t
arch_set_debug_variable(const char* variableName, uint64 value)
{
	return B_ENTRY_NOT_FOUND;
}


status_t
arch_get_debug_variable(const char* variableName, uint64* value)
{
	return B_ENTRY_NOT_FOUND;
}


status_t
arch_debug_init(kernel_args *args)
{
	return B_NO_ERROR;
}


void
arch_debug_unset_current_thread(void)
{
}


ssize_t
arch_debug_gdb_get_registers(char* buffer, size_t bufferSize)
{
	return B_NOT_SUPPORTED;
}
