/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2007 François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */


#include <debugger.h>
#include <int.h>
#include <thread.h>
#include <arch/user_debugger.h>


void
arch_clear_team_debug_info(struct arch_team_debug_info* info)
{
#warning IMPLEMENT arch_clear_team_debug_info
}


void
arch_destroy_team_debug_info(struct arch_team_debug_info* info)
{
	arch_clear_team_debug_info(info);
}


void
arch_clear_thread_debug_info(struct arch_thread_debug_info* info)
{
#warning IMPLEMENT arch_clear_thread_debug_info
}


void
arch_destroy_thread_debug_info(struct arch_thread_debug_info* info)
{
	arch_clear_thread_debug_info(info);
}


void
arch_update_thread_single_step()
{
#warning IMPLEMENT arch_update_thread_single_step
}


void
arch_set_debug_cpu_state(const debug_cpu_state* cpuState)
{
#warning IMPLEMENT arch_set_debug_cpu_state
}


void
arch_get_debug_cpu_state(debug_cpu_state* cpuState)
{
#warning IMPLEMENT arch_get_debug_cpu_state
}


status_t
arch_get_thread_debug_cpu_state(Thread* thread,
	debug_cpu_state* cpuState)
{
#warning IMPLEMENT arch_get_thread_debug_cpu_state
	return B_ERROR;
}


status_t
arch_set_breakpoint(void* address)
{
#warning IMPLEMENT arch_set_breakpoint
	return B_ERROR;
}


status_t
arch_clear_breakpoint(void* address)
{
#warning IMPLEMENT arch_clear_breakpoint
	return B_ERROR;
}


status_t
arch_set_watchpoint(void* address, uint32 type, int32 length)
{
#warning IMPLEMENT arch_set_watchpoint
	return B_ERROR;
}


status_t
arch_clear_watchpoint(void* address)
{
#warning IMPLEMENT arch_clear_watchpoint
	return B_ERROR;
}


bool
arch_has_breakpoints(struct arch_team_debug_info* info)
{
#warning IMPLEMENT arch_has_breakpoints
	return false;
}

