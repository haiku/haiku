/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <string.h>

#include <debugger.h>
#include <arch/user_debugger.h>
#include <arch/thread.h>

void
arch_get_debug_cpu_state(struct debug_cpu_state *cpuState)
{
	struct iframe *frame = i386_get_current_iframe();
	memcpy(cpuState, frame, sizeof(debug_cpu_state));
}
