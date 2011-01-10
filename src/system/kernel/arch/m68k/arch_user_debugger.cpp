/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2005, Axel Dörfler, axeld@pinc-softare.de
 * Distributed under the terms of the MIT License.
 */


#include <debugger.h>
#include <int.h>
#include <thread.h>
#include <arch/user_debugger.h>


#warning M68K: WRITEME
void
arch_clear_team_debug_info(struct arch_team_debug_info *info)
{
}


void
arch_destroy_team_debug_info(struct arch_team_debug_info *info)
{
	arch_clear_team_debug_info(info);
}


void
arch_clear_thread_debug_info(struct arch_thread_debug_info *info)
{
}


void
arch_destroy_thread_debug_info(struct arch_thread_debug_info *info)
{
	arch_clear_thread_debug_info(info);
}


void
arch_update_thread_single_step()
{
	if (struct iframe* frame = m68k_get_user_iframe()) {
		Thread* thread = thread_get_current_thread();

		// set/clear T1 in SR depending on if single stepping is desired
		// T1 T0
		// 0  0  no tracing
		// 0  1  trace on flow
		// 1  0  single step
		// 1  1  undef
		// note 060 and 020(?) only have T1 bit,
		// but this should be compatible as well.
		if (thread->debug_info.flags & B_THREAD_DEBUG_SINGLE_STEP) {
			frame->cpu.sr &= ~(M68K_SR_T_MASK);
			frame->cpu.sr |= (1 << M68K_SR_T1);
		} else {
			frame->cpu.sr &= ~(M68K_SR_T_MASK);
		}
	}
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
	return B_ERROR;
}


status_t
arch_clear_breakpoint(void *address)
{
	return B_ERROR;
}


status_t
arch_set_watchpoint(void *address, uint32 type, int32 length)
{
	return B_ERROR;
}


status_t
arch_clear_watchpoint(void *address)
{
	return B_ERROR;
}

bool
arch_has_breakpoints(struct arch_team_debug_info *info)
{
	return false;
}
