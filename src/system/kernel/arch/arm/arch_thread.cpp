/*
 * Copyright 2003-2023, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		François Revol <revol@free.fr>
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <thread.h>
#include <arch_thread.h>

#include <arch_cpu.h>
#include <arch/thread.h>
#include <boot/stage2.h>
#include <commpage.h>
#include <kernel.h>
#include <thread.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>
#include <arch_vm.h>
#include <arch/vm_translation_map.h>

#include <string.h>

#include "ARMPagingStructures.h"
#include "ARMVMTranslationMap.h"

//#define TRACE_ARCH_THREAD
#ifdef TRACE_ARCH_THREAD
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif

// Valid initial arch_thread state. We just memcpy() it when initializing
// a new thread structure.
static struct arch_thread sInitialState;


void
arm_push_iframe(struct iframe_stack *stack, struct iframe *frame)
{
	ASSERT(stack->index < IFRAME_TRACE_DEPTH);
	stack->frames[stack->index++] = frame;
}


void
arm_pop_iframe(struct iframe_stack *stack)
{
	ASSERT(stack->index > 0);
	stack->index--;
}



status_t
arch_thread_init(struct kernel_args *args)
{
	// Initialize the static initial arch_thread state (sInitialState).
	// Currently nothing to do, i.e. zero initialized is just fine.

	return B_OK;
}


status_t
arch_team_init_team_struct(Team *team, bool kernel)
{
	// Nothing to do. The structure is empty.
	return B_OK;
}


status_t
arch_thread_init_thread_struct(Thread *thread)
{
	// set up an initial state (stack & fpu)
	memcpy(&thread->arch_info, &sInitialState, sizeof(struct arch_thread));

	return B_OK;
}


void
arch_thread_init_kthread_stack(Thread* thread, void* _stack, void* _stackTop,
	void (*function)(void*), const void* data)
{
	addr_t* stackTop = (addr_t*)_stackTop;

	TRACE("arch_thread_init_kthread_stack(%s): stack top %p, function %p, data: "
		"%p\n", thread->name, stackTop, function, data);

	// push the function address -- that's the return address used after the
	// context switch (lr/r14 register)
	*--stackTop = (addr_t)function;

	// simulate storing registers r1-r12
	for (int i = 1; i <= 12; i++)
		*--stackTop = 0;

	// push the function argument as r0
	*--stackTop = (addr_t)data;

	// save the stack position
	thread->arch_info.sp = stackTop;
}


status_t
arch_thread_init_tls(Thread *thread)
{
	thread->user_local_storage =
		thread->user_stack_base + thread->user_stack_size;
	return B_OK;
}


void
arm_swap_pgdir(uint32_t pageDirectoryAddress)
{
	// Set translation table base
	asm volatile("MCR p15, 0, %[addr], c2, c0, 0"::[addr] "r" (pageDirectoryAddress));
	isb();

	arch_cpu_global_TLB_invalidate();

	//TODO: update Context ID (incl. ASID)
	//TODO: check if any additional TLB or Cache maintenance is needed
}


void
arm_set_tls_context(Thread *thread)
{
	// Set TPIDRURO to point to TLS base
	asm volatile("MCR p15, 0, %0, c13, c0, 3"
		: : "r" (thread->user_local_storage));
}


void
arch_thread_context_switch(Thread *from, Thread *to)
{
	arm_set_tls_context(to);

	VMAddressSpace *oldAddressSpace = from->team->address_space;
	VMTranslationMap *oldTranslationMap = oldAddressSpace->TranslationMap();
	phys_addr_t oldPageDirectoryAddress =
		((ARMVMTranslationMap *)oldTranslationMap)->PagingStructures()->pgdir_phys;

	VMAddressSpace *newAddressSpace = to->team->address_space;
	VMTranslationMap *newTranslationMap = newAddressSpace->TranslationMap();
	phys_addr_t newPageDirectoryAddress =
		((ARMVMTranslationMap *)newTranslationMap)->PagingStructures()->pgdir_phys;

	if (oldPageDirectoryAddress != newPageDirectoryAddress) {
		TRACE("arch_thread_context_switch: swap pgdir: "
			"0x%08" B_PRIxPHYSADDR " -> 0x%08" B_PRIxPHYSADDR "\n",
			oldPageDirectoryAddress, newPageDirectoryAddress);
		arm_swap_pgdir(newPageDirectoryAddress);
	}

	TRACE("arch_thread_context_switch: %p(%s/%p) -> %p(%s/%p)\n",
		from, from->name, from->arch_info.sp, to, to->name, to->arch_info.sp);
	arm_save_fpu(&from->arch_info.fpuContext);
	arm_restore_fpu(&to->arch_info.fpuContext);
	arm_context_switch(&from->arch_info, &to->arch_info);
	TRACE("arch_thread_context_switch %p %p\n", to, from);
}


void
arch_thread_dump_info(void *info)
{
	struct arch_thread *at = (struct arch_thread *)info;

	dprintf("\tsp: %p\n", at->sp);
}


status_t
arch_thread_enter_userspace(Thread *thread, addr_t entry,
	void *args1, void *args2)
{
	arm_set_tls_context(thread);

	addr_t stackTop = thread->user_stack_base + thread->user_stack_size;

	TRACE("arch_thread_enter_userspace: entry 0x%" B_PRIxADDR ", args %p %p, "
		"ustack_top 0x%" B_PRIxADDR "\n", entry, args1, args2, stackTop);

	//stackTop = arch_randomize_stack_pointer(stackTop - sizeof(args));

	// Copy the address of the stub that calls exit_thread() when the thread
	// entry function returns to LR to act as the return address.
	// The stub is inside commpage.
	addr_t commPageAddress = (addr_t)thread->team->commpage_address;

	disable_interrupts();

	// prepare the user iframe
	iframe frame = {};
	frame.r0 = (uint32)args1;
	frame.r1 = (uint32)args2;
	frame.usr_sp = stackTop;
	frame.usr_lr = ((addr_t*)commPageAddress)[COMMPAGE_ENTRY_ARM_THREAD_EXIT]
		+ commPageAddress;
	frame.pc = entry;

	// return to userland
	arch_return_to_userland(&frame);

	// normally we don't get here
	return B_ERROR;
}


bool
arch_on_signal_stack(Thread *thread)
{
	struct iframe* frame = thread->arch_info.userFrame;
	if (frame == NULL) {
		panic("arch_on_signal_stack(): No user iframe!");
		return false;
	}

	return frame->usr_sp >= thread->signal_stack_base
		&& frame->usr_sp < thread->signal_stack_base
			+ thread->signal_stack_size;
}


static uint8*
get_signal_stack(Thread* thread, struct iframe* frame,
	struct sigaction* action, size_t spaceNeeded)
{
	// use the alternate signal stack if we should and can
	if (thread->signal_stack_enabled && (action->sa_flags & SA_ONSTACK) != 0
			&& (frame->usr_sp < thread->signal_stack_base
			|| frame->usr_sp >= thread->signal_stack_base + thread->signal_stack_size)) {
		addr_t stackTop = thread->signal_stack_base + thread->signal_stack_size;
		return (uint8*)ROUNDDOWN(stackTop - spaceNeeded, 16);
	}

	return (uint8*)ROUNDDOWN(frame->usr_sp - spaceNeeded, 16);
}


status_t
arch_setup_signal_frame(Thread *thread, struct sigaction *sa,
	struct signal_frame_data *signalFrameData)
{
	iframe* frame = thread->arch_info.userFrame;
	if (frame == NULL) {
		panic("arch_setup_signal_frame(): No user iframe!");
		return B_ERROR;
	}

	// store the register state in signalFrameData->context.uc_mcontext
	signalFrameData->context.uc_mcontext.r0   = frame->r0;
	signalFrameData->context.uc_mcontext.r1   = frame->r1;
	signalFrameData->context.uc_mcontext.r2   = frame->r2;
	signalFrameData->context.uc_mcontext.r3   = frame->r3;
	signalFrameData->context.uc_mcontext.r4   = frame->r4;
	signalFrameData->context.uc_mcontext.r5   = frame->r5;
	signalFrameData->context.uc_mcontext.r6   = frame->r6;
	signalFrameData->context.uc_mcontext.r7   = frame->r7;
	signalFrameData->context.uc_mcontext.r8   = frame->r8;
	signalFrameData->context.uc_mcontext.r9   = frame->r9;
	signalFrameData->context.uc_mcontext.r10  = frame->r10;
	signalFrameData->context.uc_mcontext.r11  = frame->r11;
	signalFrameData->context.uc_mcontext.r12  = frame->r12;
	signalFrameData->context.uc_mcontext.r13  = frame->usr_sp;
	signalFrameData->context.uc_mcontext.r14  = frame->usr_lr;
	signalFrameData->context.uc_mcontext.r15  = frame->pc;
	signalFrameData->context.uc_mcontext.cpsr = frame->spsr;

	arm_save_fpu((arch_fpu_context*)&signalFrameData->context.uc_mcontext.d[0]);

	// Fill in signalFrameData->context.uc_stack
	signal_get_user_stack(frame->usr_sp, &signalFrameData->context.uc_stack);

	// store oldR0 in syscall_restart_return_value
	signalFrameData->syscall_restart_return_value = thread->arch_info.oldR0;

	// get the stack to use -- that's either the current one or a special signal stack
	uint8* userStack = get_signal_stack(thread, frame, sa,
		sizeof(*signalFrameData));

	// copy the signal frame data onto the stack
	status_t res = user_memcpy(userStack, signalFrameData,
		sizeof(*signalFrameData));
	if (res < B_OK)
		return res;

	// prepare the user stack frame for a function call to the signal handler wrapper function
	addr_t commpageAddr = (addr_t)thread->team->commpage_address;
	addr_t signalHandlerAddr;
	ASSERT(user_memcpy(&signalHandlerAddr,
		&((addr_t*)commpageAddr)[COMMPAGE_ENTRY_ARM_SIGNAL_HANDLER],
		sizeof(signalHandlerAddr)) >= B_OK);
	signalHandlerAddr += commpageAddr;

	frame->usr_lr = frame->pc;
	frame->usr_sp = (addr_t)userStack;
	frame->pc = signalHandlerAddr;
	frame->r0 = frame->usr_sp;

	return B_OK;
}


int64
arch_restore_signal_frame(struct signal_frame_data* signalFrameData)
{
	iframe* frame = thread_get_current_thread()->arch_info.userFrame;
	if (frame == NULL) {
		panic("arch_restore_signal_frame(): No user iframe!");
		return 0;
	}

	thread_get_current_thread()->arch_info.oldR0
		= signalFrameData->syscall_restart_return_value;

	frame->r0     = signalFrameData->context.uc_mcontext.r0;
	frame->r1     = signalFrameData->context.uc_mcontext.r1;
	frame->r2     = signalFrameData->context.uc_mcontext.r2;
	frame->r3     = signalFrameData->context.uc_mcontext.r3;
	frame->r4     = signalFrameData->context.uc_mcontext.r4;
	frame->r5     = signalFrameData->context.uc_mcontext.r5;
	frame->r6     = signalFrameData->context.uc_mcontext.r6;
	frame->r7     = signalFrameData->context.uc_mcontext.r7;
	frame->r8     = signalFrameData->context.uc_mcontext.r8;
	frame->r9     = signalFrameData->context.uc_mcontext.r9;
	frame->r10    = signalFrameData->context.uc_mcontext.r10;
	frame->r11    = signalFrameData->context.uc_mcontext.r11;
	frame->r12    = signalFrameData->context.uc_mcontext.r12;
	frame->usr_sp = signalFrameData->context.uc_mcontext.r13;
	frame->usr_lr = signalFrameData->context.uc_mcontext.r14;
	frame->pc     = signalFrameData->context.uc_mcontext.r15;
	frame->spsr   = signalFrameData->context.uc_mcontext.cpsr;

	arm_restore_fpu((arch_fpu_context*)&signalFrameData->context.uc_mcontext.d[0]);

	return frame->r0;
}


void
arch_check_syscall_restart(Thread *thread)
{
}


/**	Saves everything needed to restore the frame in the child fork in the
 *	arch_fork_arg structure to be passed to arch_restore_fork_frame().
 *	Also makes sure to return the right value.
 */
void
arch_store_fork_frame(struct arch_fork_arg *arg)
{
	struct iframe* frame = thread_get_current_thread()->arch_info.userFrame;
	if (frame == NULL) {
		panic("arch_store_fork_frame(): No user iframe!");
	}

	arg->frame = *frame;
	arg->frame.r0 = 0; // fork return value
}


/** Restores the frame from a forked team as specified by the provided
 *	arch_fork_arg structure.
 *	Needs to be called from within the child team, ie. instead of
 *	arch_thread_enter_uspace() as thread "starter".
 *	This function does not return to the caller, but will enter userland
 *	in the child team at the same position where the parent team left of.
 */
void
arch_restore_fork_frame(struct arch_fork_arg *arg)
{
	disable_interrupts();
	arch_return_to_userland(&arg->frame);
}
