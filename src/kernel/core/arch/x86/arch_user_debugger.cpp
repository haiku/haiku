/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <string.h>

#include <debugger.h>
#include <thread.h>
#include <arch/user_debugger.h>

#define B_NO_MORE_BREAKPOINTS				B_ERROR
#define B_NO_MORE_WATCHPOINTS				B_ERROR
#define B_BAD_WATCHPOINT_ALIGNMENT			B_ERROR
#define B_WATCHPOINT_TYPE_NOT_SUPPORTED		B_ERROR
#define B_WATCHPOINT_LENGTH_NOT_SUPPORTED	B_ERROR
#define B_BREAKPOINT_NOT_FOUND				B_ERROR
#define B_WATCHPOINT_NOT_FOUND				B_ERROR
	// ToDo: Make those real error codes.

// maps breakpoint slot index to LEN_i LSB number
static const uint32 sDR7Len[4] = {
	X86_DR7_LEN0_LSB, X86_DR7_LEN1_LSB, X86_DR7_LEN2_LSB, X86_DR7_LEN3_LSB
};

// maps breakpoint slot index to R/W_i LSB number
static const uint32 sDR7RW[4] = {
	X86_DR7_RW0_LSB, X86_DR7_RW1_LSB, X86_DR7_RW2_LSB, X86_DR7_RW3_LSB
};

// maps breakpoint slot index to L_i bit number
static const uint32 sDR7L[4] = {
	X86_DR7_L0, X86_DR7_L1, X86_DR7_L2, X86_DR7_L3
};


void
arch_clear_team_debug_info(struct arch_team_debug_info *info)
{
	for (int32 i = 0; i < X86_BREAKPOINT_COUNT; i++)
		info->breakpoints[i].address = NULL;

	info->dr7 = X86_BREAKPOINTS_DISABLED_DR7;
}


void
arch_destroy_team_debug_info(struct arch_team_debug_info *info)
{
	arch_clear_team_debug_info(info);
}


void
arch_set_debug_cpu_state(const struct debug_cpu_state *cpuState)
{
	// ToDo: Implement!
}

void
arch_get_debug_cpu_state(struct debug_cpu_state *cpuState)
{
	struct iframe *frame = i386_get_current_iframe();
	memcpy(cpuState, frame, sizeof(debug_cpu_state));
}

static status_t
set_breakpoint(void *address, uint32 type, uint32 length)
{
	if (!address)
		return B_BAD_VALUE;

	struct thread *thread = thread_get_current_thread();

	status_t error = B_OK;

	cpu_status state = disable_interrupts();
	GRAB_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);

	arch_team_debug_info &info = thread->team->debug_info.arch_info;

	// check, if there is already a breakpoint at that address
	bool alreadySet = false;
	for (int32 i = 0; i < X86_BREAKPOINT_COUNT; i++) {
		if (info.breakpoints[i].address == address
			&& info.breakpoints[i].type == type) {
			alreadySet = true;
			break;
		}
	}

	if (!alreadySet) {
		// find a free slot
		int32 slot = -1;
		for (int32 i = 0; i < X86_BREAKPOINT_COUNT; i++) {
			if (!info.breakpoints[i].address) {
				slot = i;
				break;
			}
		}

		// init the breakpoint
		if (slot >= 0) {
			info.breakpoints[slot].address = address;
			info.breakpoints[slot].type = type;
			info.breakpoints[slot].length = length;

			info.dr7 |= (length << sDR7Len[slot])
				| (type << sDR7RW[slot])
				| (1 << sDR7L[slot]);

			// set the respective debug address register (DR0-DR3)
			switch (slot) {
				case 0: asm("movl %0, %%dr0" : : "r"(address)); break;
				case 1: asm("movl %0, %%dr1" : : "r"(address)); break;
				case 2: asm("movl %0, %%dr2" : : "r"(address)); break;
				case 3: asm("movl %0, %%dr3" : : "r"(address)); break;
			}
		} else {
			if (type == X86_INSTRUCTION_BREAKPOINT)
				error = B_NO_MORE_BREAKPOINTS;
			else
				error = B_NO_MORE_WATCHPOINTS;
		}
	}

	RELEASE_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);
	restore_interrupts(state);

	return error;
}


static status_t
clear_breakpoint(void *address, bool watchpoint)
{
	if (!address)
		return B_BAD_VALUE;

	struct thread *thread = thread_get_current_thread();

	status_t error = B_OK;

	cpu_status state = disable_interrupts();
	GRAB_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);

	arch_team_debug_info &info = thread->team->debug_info.arch_info;

	// find the breakpoint
	int32 slot = -1;
	for (int32 i = 0; i < X86_BREAKPOINT_COUNT; i++) {
		if (info.breakpoints[i].address == address
			&& (watchpoint
				!= (info.breakpoints[i].type == X86_INSTRUCTION_BREAKPOINT))) {
			slot = i;
			break;
		}
	}

	// clear the breakpoint
	if (slot >= 0) {
		info.breakpoints[slot].address = NULL;

		info.dr7 &= ~((0x3 << sDR7Len[slot])
			| (0x3 << sDR7RW[slot])
			| (1 << sDR7L[slot]));
	} else {
		if (watchpoint)
			error = B_WATCHPOINT_NOT_FOUND;
		else
			error = B_BREAKPOINT_NOT_FOUND;
	}

	RELEASE_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);
	restore_interrupts(state);

	return error;
}


status_t
arch_set_breakpoint(void *address)
{
	return set_breakpoint(address, X86_INSTRUCTION_BREAKPOINT,
		X86_BREAKPOINT_LENGTH_1);
}


status_t
arch_clear_breakpoint(void *address)
{
	return clear_breakpoint(address, false);
}


status_t
arch_set_watchpoint(void *address, uint32 type, int32 length)
{
	// check type
	uint32 archType;
	switch (type) {
		case B_DATA_WRITE_WATCHPOINT:
			archType = X86_DATA_WRITE_BREAKPOINT;
			break;
		case B_DATA_READ_WRITE_WATCHPOINT:
			archType = X86_DATA_READ_WRITE_BREAKPOINT;
			break;
		case B_DATA_READ_WATCHPOINT:
		default:
			return B_WATCHPOINT_TYPE_NOT_SUPPORTED;
			break;
	}

	// check length and alignment
	uint32 archLength;
	switch (length) {
		case 1:
			archLength = X86_BREAKPOINT_LENGTH_1;
			break;
		case 2:
			if ((uint32)address & 0x1)
				return B_BAD_WATCHPOINT_ALIGNMENT;
			archLength = X86_BREAKPOINT_LENGTH_2;
			break;
		case 4:
			if ((uint32)address & 0x3)
				return B_BAD_WATCHPOINT_ALIGNMENT;
			archLength = X86_BREAKPOINT_LENGTH_4;
			break;
		default:
			return B_WATCHPOINT_LENGTH_NOT_SUPPORTED;
	}

	return set_breakpoint(address, archType, archLength);
}


status_t
arch_clear_watchpoint(void *address)
{
	return clear_breakpoint(address, false);
}


