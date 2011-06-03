/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_DEBUG_H
#define _KERNEL_ARCH_DEBUG_H


#include <SupportDefs.h>

#include <arch_debug.h>
#include <cpu.h>


struct kernel_args;

namespace BKernel {
	struct Thread;
}

using BKernel::Thread;


// arch_debug_get_stack_trace() flags
#define STACK_TRACE_KERNEL	0x01
#define STACK_TRACE_USER	0x02


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_debug_init(kernel_args *args);
void arch_debug_stack_trace(void);
void *arch_debug_get_caller(void);
int32 arch_debug_get_stack_trace(addr_t* returnAddresses, int32 maxCount,
		int32 skipIframes, int32 skipFrames, uint32 flags);
void* arch_debug_get_interrupt_pc(bool* _isSyscall);
bool arch_debug_contains_call(Thread *thread, const char *symbol,
		addr_t start, addr_t end);
void arch_debug_save_registers(struct arch_debug_registers* registers);
void arch_debug_unset_current_thread(void);
void arch_debug_call_with_fault_handler(cpu_ent* cpu, jmp_buf jumpBuffer,
		void (*function)(void*), void* parameter);

bool arch_is_debug_variable_defined(const char* variableName);
status_t arch_set_debug_variable(const char* variableName, uint64 value);
status_t arch_get_debug_variable(const char* variableName, uint64* value);

ssize_t arch_debug_gdb_get_registers(char* buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_ARCH_DEBUG_H */
