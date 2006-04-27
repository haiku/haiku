/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <string.h>

#include <debugger.h>
#include <driver_settings.h>
#include <int.h>
#include <thread.h>
#include <arch/user_debugger.h>

//#define TRACE_ARCH_USER_DEBUGGER
#ifdef TRACE_ARCH_USER_DEBUGGER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

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

// maps breakpoint slot index to B_i bit number
static const uint32 sDR6B[4] = {
	X86_DR6_B0, X86_DR6_B1, X86_DR6_B2, X86_DR6_B3
};

// Enables a hack to make single stepping work under qemu. Set via kernel
// driver settings.
static bool sQEmuSingleStepHack = false;


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
arch_clear_thread_debug_info(struct arch_thread_debug_info *info)
{
	info->flags = 0;
}


void
arch_destroy_thread_debug_info(struct arch_thread_debug_info *info)
{
	arch_clear_thread_debug_info(info);
}


void
arch_set_debug_cpu_state(const struct debug_cpu_state *cpuState)
{
	if (struct iframe *frame = i386_get_user_iframe()) {
		struct thread *thread = thread_get_current_thread();

		i386_frstor(cpuState->extended_regs);
			// For this to be correct the calling function must not use these
			// registers (not even indirectly).

//		frame->gs = cpuState->gs;
//		frame->fs = cpuState->fs;
//		frame->es = cpuState->es;
//		frame->ds = cpuState->ds;
		frame->edi = cpuState->edi;
		frame->esi = cpuState->esi;
		frame->ebp = cpuState->ebp;
		frame->esp = cpuState->esp;
		frame->ebx = cpuState->ebx;
		frame->edx = cpuState->edx;
		frame->ecx = cpuState->ecx;
		frame->eax = cpuState->eax;
//		frame->vector = cpuState->vector;
//		frame->error_code = cpuState->error_code;
		frame->eip = cpuState->eip;
//		frame->cs = cpuState->cs;
		frame->flags = (frame->flags & ~X86_EFLAGS_USER_SETTABLE_FLAGS)
			| (cpuState->eflags & X86_EFLAGS_USER_SETTABLE_FLAGS);
		frame->user_esp = cpuState->user_esp;
//		frame->user_ss = cpuState->user_ss;
	}
}

void
arch_get_debug_cpu_state(struct debug_cpu_state *cpuState)
{
	if (struct iframe *frame = i386_get_user_iframe()) {
		struct thread *thread = thread_get_current_thread();

		i386_fnsave(cpuState->extended_regs);
			// For this to be correct the calling function must not use these
			// registers (not even indirectly).

		cpuState->gs = frame->gs;
		cpuState->fs = frame->fs;
		cpuState->es = frame->es;
		cpuState->ds = frame->ds;
		cpuState->edi = frame->edi;
		cpuState->esi = frame->esi;
		cpuState->ebp = frame->ebp;
		cpuState->esp = frame->esp;
		cpuState->ebx = frame->ebx;
		cpuState->edx = frame->orig_edx;
		cpuState->ecx = frame->ecx;
		cpuState->eax = frame->orig_eax;
		cpuState->vector = frame->vector;
		cpuState->error_code = frame->error_code;
		cpuState->eip = frame->eip;
		cpuState->cs = frame->cs;
		cpuState->eflags = frame->flags;
		cpuState->user_esp = frame->user_esp;
		cpuState->user_ss = frame->user_ss;
	}
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


static inline void
install_breakpoints(const arch_team_debug_info &teamInfo)
{
	// set breakpoints
	asm("movl %0, %%dr0" : : "r"(teamInfo.breakpoints[0].address));
	asm("movl %0, %%dr1" : : "r"(teamInfo.breakpoints[1].address));
	asm("movl %0, %%dr2" : : "r"(teamInfo.breakpoints[2].address));
//	asm("movl %0, %%dr3" : : "r"(teamInfo.breakpoints[3].address));
		// DR3 is used to hold the current struct thread*.

	// enable breakpoints
	asm("movl %0, %%dr7" : : "r"(teamInfo.dr7));
}


/**
 *	Interrupts are enabled.
 */
void
i386_init_user_debug_at_kernel_exit(struct iframe *frame)
{
	struct thread *thread = thread_get_current_thread();

	cpu_status state = disable_interrupts();
	GRAB_THREAD_LOCK();
	GRAB_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);

	arch_team_debug_info &teamInfo = thread->team->debug_info.arch_info;

	// install the breakpoints
	install_breakpoints(teamInfo);
	thread->debug_info.arch_info.flags |= X86_THREAD_DEBUG_DR7_SET;

	// set/clear TF in EFLAGS depending on if single stepping is desired
	if (thread->debug_info.flags & B_THREAD_DEBUG_SINGLE_STEP)
		frame->flags |= (1 << X86_EFLAGS_TF);
	else
		frame->flags &= ~(1 << X86_EFLAGS_TF);
		// ToDo: Move into a function called from thread_hit_debug_event().
		// No need to have that here in the code executed for ever kernel->user
		// mode switch.

	RELEASE_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);
	RELEASE_THREAD_LOCK();
	restore_interrupts(state);
}


/**
 *	Interrupts may be enabled.
 */
void
i386_exit_user_debug_at_kernel_entry()
{
	struct thread *thread = thread_get_current_thread();

	cpu_status state = disable_interrupts();
	GRAB_THREAD_LOCK();

	// disable breakpoints
	asm("movl %0, %%dr7" : : "r"(X86_BREAKPOINTS_DISABLED_DR7));
	thread->debug_info.arch_info.flags &= ~X86_THREAD_DEBUG_DR7_SET;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);
}


/**
 *	Interrupts are disabled and the thread lock is being held.
 */
void
i386_reinit_user_debug_after_context_switch(struct thread *thread)
{
	if (thread->debug_info.arch_info.flags & X86_THREAD_DEBUG_DR7_SET) {
		GRAB_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);

		install_breakpoints(thread->team->debug_info.arch_info);

		RELEASE_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);
	}
}


/**
 *	Interrupts are disabled and will be enabled by the function.
 */
int
i386_handle_debug_exception(struct iframe *frame)
{
	// get debug status and control registers
	uint32 dr6, dr7;
	asm("movl %%dr6, %0" : "=r"(dr6));
	asm("movl %%dr7, %0" : "=r"(dr7));

	TRACE(("i386_handle_debug_exception(): DR6: %lx, DR7: %lx\n", dr6, dr7));

	// check, which exception condition applies
	if (dr6 & X86_DR6_BREAKPOINT_MASK) {
		// breakpoint

		// check which breakpoint was taken
		bool watchpoint = true;
		for (int32 i = 0; i < X86_BREAKPOINT_COUNT; i++) {
			if (dr6 & (1 << sDR6B[i])) {
				// If it is an instruction breakpoint, we need to set RF in
				// EFLAGS to prevent triggering the same exception
				// again (breakpoint instructions are triggered *before*
				// executing the instruction).
				uint32 type = (dr7 >> sDR7RW[i]) & 0x3;
				if (type == X86_INSTRUCTION_BREAKPOINT) {
					frame->flags |= (1 << X86_EFLAGS_RF);
					watchpoint = false;
				}
			}
		}

		// enable interrupts and notify the debugger
		enable_interrupts();

		if (watchpoint)
			user_debug_watchpoint_hit();
		else
			user_debug_breakpoint_hit(false);
	} else if (dr6 & (1 << X86_DR6_BD)) {
		// general detect exception
		// Occurs only, if GD in DR7 is set (which we don't do) and someone
		// tries to write to the debug registers.
		dprintf("i386_handle_debug_exception(): ignoring spurious general "
			"detect exception\n");

		enable_interrupts();
	} else if ((dr6 & (1 << X86_DR6_BS)) || sQEmuSingleStepHack) {
		// single step

		// enable interrupts and notify the debugger
		enable_interrupts();

		user_debug_single_stepped();
	} else if (dr6 & (1 << X86_DR6_BT)) {
		// task switch
		// Occurs only, if T in EFLAGS is set (which we don't do).
		dprintf("i386_handle_debug_exception(): ignoring spurious task switch "
			"exception\n");

		enable_interrupts();
	} else {
		TRACE(("i386_handle_debug_exception(): ignoring spurious debug "
			"exception (no condition recognized)\n"));

		enable_interrupts();
	}

	return B_HANDLED_INTERRUPT;
}


/**
 *	Interrupts are disabled and will be enabled by the function.
 */
int
i386_handle_breakpoint_exception(struct iframe *frame)
{
	TRACE(("i386_handle_breakpoint_exception()\n"));

	enable_interrupts();

	user_debug_breakpoint_hit(true);

	return B_HANDLED_INTERRUPT;
}


void
i386_init_user_debug()
{
	// get debug settings
	if (void *handle = load_driver_settings("kernel")) {
		sQEmuSingleStepHack = get_driver_boolean_parameter(handle,
			"qemu_single_step_hack", false, false);;

		unload_driver_settings(handle);
	}
}

