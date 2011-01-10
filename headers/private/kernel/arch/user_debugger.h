/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_USER_DEBUGGER_H
#define KERNEL_ARCH_USER_DEBUGGER_H


#include "kernel_debug_config.h"

#ifndef _ASSEMBLER

#include <debugger.h>

#ifdef __cplusplus
extern "C" {
#endif

struct arch_team_debug_info;
struct arch_thread_debug_info;


void arch_clear_team_debug_info(struct arch_team_debug_info *info);
void arch_destroy_team_debug_info(struct arch_team_debug_info *info);
void arch_clear_thread_debug_info(struct arch_thread_debug_info *info);
void arch_destroy_thread_debug_info(struct arch_thread_debug_info *info);

void arch_update_thread_single_step();

void arch_set_debug_cpu_state(const debug_cpu_state *cpuState);
void arch_get_debug_cpu_state(debug_cpu_state *cpuState);

status_t arch_set_breakpoint(void *address);
status_t arch_clear_breakpoint(void *address);
status_t arch_set_watchpoint(void *address, uint32 type, int32 length);
status_t arch_clear_watchpoint(void *address);
bool arch_has_breakpoints(struct arch_team_debug_info *info);

#if KERNEL_BREAKPOINTS
status_t arch_set_kernel_breakpoint(void *address);
status_t arch_clear_kernel_breakpoint(void *address);
status_t arch_set_kernel_watchpoint(void *address, uint32 type, int32 length);
status_t arch_clear_kernel_watchpoint(void *address);
#endif

#ifdef __cplusplus
}
#endif

#include <arch_user_debugger.h>

// Defaults for macros defined by the architecture specific header:

// maximum number of instruction breakpoints
#ifndef DEBUG_MAX_BREAKPOINTS
#	define DEBUG_MAX_BREAKPOINTS 0
#endif

// maximum number of data watchpoints
#ifndef DEBUG_MAX_WATCHPOINTS
#	define DEBUG_MAX_WATCHPOINTS 0
#endif

// the software breakpoint instruction
#if !defined(DEBUG_SOFTWARE_BREAKPOINT)	\
	|| !defined(DEBUG_SOFTWARE_BREAKPOINT_SIZE)
#	undef DEBUG_SOFTWARE_BREAKPOINT
#	undef DEBUG_SOFTWARE_BREAKPOINT_SIZE
#	define DEBUG_SOFTWARE_BREAKPOINT		NULL
#	define DEBUG_SOFTWARE_BREAKPOINT_SIZE	0
#endif

// Boolean whether break- and watchpoints use the same registers. If != 0, then
// DEBUG_MAX_BREAKPOINTS == DEBUG_MAX_WATCHPOINTS and either specifies the
// total count of break- plus watchpoints.
#ifndef DEBUG_SHARED_BREAK_AND_WATCHPOINTS
#	define DEBUG_SHARED_BREAK_AND_WATCHPOINTS 0
#endif

#endif	// _ASSEMBLER

#endif	// KERNEL_ARCH_USER_DEBUGGER_H
