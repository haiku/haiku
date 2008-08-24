/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_DEBUG_H
#define _KERNEL_ARCH_DEBUG_H


#include <SupportDefs.h>

struct kernel_args;
struct thread;


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_debug_init(kernel_args *args);
void *arch_debug_get_caller(void);
int32 arch_debug_get_stack_trace(addr_t* returnAddresses, int32 maxCount,
		int32 skipFrames, bool userOnly);
void *arch_debug_get_interrupt_pc();
bool arch_debug_contains_call(struct thread *thread, const char *symbol,
		addr_t start, addr_t end);
void arch_debug_save_registers(int *);

bool arch_is_debug_variable_defined(const char* variableName);
status_t arch_set_debug_variable(const char* variableName, uint64 value);
status_t arch_get_debug_variable(const char* variableName, uint64* value);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_ARCH_DEBUG_H */
