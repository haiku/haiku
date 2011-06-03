/*
 * Copyright 2003-201, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		François Revol <revol@free.fr>
 *		Jonas Sundström <jonas@kirilla.com>
 */


#include <arch/debug.h>

#include <arch_cpu.h>
#include <debug.h>
#include <elf.h>
#include <kernel.h>
#include <kimage.h>
#include <thread.h>


struct stack_frame {
	struct stack_frame	*previous;
	addr_t				return_address;
};


#define NUM_PREVIOUS_LOCATIONS 32


extern struct iframe_stack gBootFrameStack;


// #pragma mark -


void
arch_debug_save_registers(struct arch_debug_registers* registers)
{
#warning IMPLEMENT arch_debug_save_registers
}


bool
arch_debug_contains_call(Thread* thread, const char* symbol,
	addr_t start, addr_t end)
{
#warning IMPLEMENT arch_debug_contains_call
	return false;
}


void*
arch_debug_get_caller(void)
{
#warning IMPLEMENT arch_debug_get_caller
	return NULL;
}


int32
arch_debug_get_stack_trace(addr_t* returnAddresses, int32 maxCount,
	int32 skipIframes, int32 skipFrames, uint32 flags)
{
#warning IMPLEMENT arch_debug_get_stack_trace
	return 0;
}


void*
arch_debug_get_interrupt_pc(bool* _isSyscall)
{
#warning IMPLEMENT arch_debug_get_interrupt_pc
	return NULL;
}


bool
arch_is_debug_variable_defined(const char* variableName)
{
#warning IMPLEMENT arch_is_debug_variable_defined
	return false;
}


status_t
arch_set_debug_variable(const char* variableName, uint64 value)
{
#warning IMPLEMENT arch_set_debug_variable
	return B_ENTRY_NOT_FOUND;
}


status_t
arch_get_debug_variable(const char* variableName, uint64* value)
{
#warning IMPLEMENT arch_get_debug_variable
	return B_ENTRY_NOT_FOUND;
}


ssize_t
arch_debug_gdb_get_registers(char* buffer, size_t bufferSize)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


status_t
arch_debug_init(kernel_args* args)
{
#warning IMPLEMENT arch_debug_init
	return B_ERROR;
}


void
arch_debug_call_with_fault_handler(cpu_ent* cpu, jmp_buf jumpBuffer,
        void (*function)(void*), void* parameter)
{
#warning IMPLEMENT arch_debug_call_with_fault_handler
	longjmp(jumpBuffer, 1);
}


void
arch_debug_unset_current_thread(void)
{
#warning IMPLEMENT arch_debug_unset_current_thread
}

