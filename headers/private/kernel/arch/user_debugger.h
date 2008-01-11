/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_USER_DEBUGGER_H
#define KERNEL_ARCH_USER_DEBUGGER_H

#include <debugger.h>

#ifdef __cplusplus
extern "C" {
#endif

// Enable this to get support for kernel breakpoints.
//#define KERNEL_BREAKPOINTS 1

struct arch_team_debug_info;
struct arch_thread_debug_info;

void arch_clear_team_debug_info(struct arch_team_debug_info *info);
void arch_destroy_team_debug_info(struct arch_team_debug_info *info);
void arch_clear_thread_debug_info(struct arch_thread_debug_info *info);
void arch_destroy_thread_debug_info(struct arch_thread_debug_info *info);

void arch_update_thread_single_step();

void arch_set_debug_cpu_state(const struct debug_cpu_state *cpuState);
void arch_get_debug_cpu_state(struct debug_cpu_state *cpuState);

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

#endif	// KERNEL_ARCH_USER_DEBUGGER_H
