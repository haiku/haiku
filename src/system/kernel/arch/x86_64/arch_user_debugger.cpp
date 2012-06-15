/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <arch/user_debugger.h>

#include <debugger.h>


// The software breakpoint instruction (int3).
const uint8 kX86SoftwareBreakpoint[1] = { 0xcc };


void
arch_clear_team_debug_info(struct arch_team_debug_info *info)
{

}


void
arch_destroy_team_debug_info(struct arch_team_debug_info *info)
{

}


void
arch_clear_thread_debug_info(struct arch_thread_debug_info *info)
{

}


void
arch_destroy_thread_debug_info(struct arch_thread_debug_info *info)
{

}


void
arch_update_thread_single_step()
{

}


void
arch_set_debug_cpu_state(const debug_cpu_state *cpuState)
{

}


void
arch_get_debug_cpu_state(debug_cpu_state *cpuState)
{

}


status_t
arch_set_breakpoint(void *address)
{
	return B_OK;
}


status_t
arch_clear_breakpoint(void *address)
{
	return B_OK;
}


status_t
arch_set_watchpoint(void *address, uint32 type, int32 length)
{
	return B_OK;
}


status_t
arch_clear_watchpoint(void *address)
{
	return B_OK;
}


bool
arch_has_breakpoints(struct arch_team_debug_info *info)
{
	return false;
}


void
x86_init_user_debug()
{

}

