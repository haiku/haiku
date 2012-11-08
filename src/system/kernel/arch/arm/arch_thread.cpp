/*
 * Copyright 2003-2010, Haiku Inc. All rights reserved.
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
#include <kernel.h>
#include <thread.h>
#include <tls.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>
#include <arch_vm.h>
#include <arch/vm_translation_map.h>

#include <string.h>

//#define TRACE_ARCH_THREAD
#ifdef TRACE_ARCH_THREAD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// Valid initial arch_thread state. We just memcpy() it when initializing
// a new thread structure.
static struct arch_thread sInitialState;

Thread *gCurrentThread;


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

	TRACE(("arch_thread_init_kthread_stack(%s): stack top %p, function %p, data: "
		"%p\n", thread->name, stackTop, function, data));

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
	uint32 tls[TLS_USER_THREAD_SLOT + 1];

	thread->user_local_storage = thread->user_stack_base
		+ thread->user_stack_size;

	// initialize default TLS fields
	memset(tls, 0, sizeof(tls));
	tls[TLS_BASE_ADDRESS_SLOT] = thread->user_local_storage;
	tls[TLS_THREAD_ID_SLOT] = thread->id;
	tls[TLS_USER_THREAD_SLOT] = (addr_t)thread->user_thread;

	return user_memcpy((void *)thread->user_local_storage, tls, sizeof(tls));
}

extern "C" void arm_context_switch(void *from, void *to);

void
arch_thread_context_switch(Thread *from, Thread *to)
{
	TRACE(("arch_thread_context_switch: %p(%s/%p) -> %p(%s/%p)\n",
		from, from->name, from->arch_info.sp, to, to->name, to->arch_info.sp));
	arm_context_switch(&from->arch_info, &to->arch_info);
	TRACE(("arch_thread_context_switch %p %p\n", to, from));
}


void
arch_thread_dump_info(void *info)
{
	struct arch_thread *at = (struct arch_thread *)info;

	dprintf("\tsp: %p\n", at->sp);
}


status_t
arch_thread_enter_userspace(Thread *thread, addr_t entry,
	void *arg1, void *arg2)
{
	panic("arch_thread_enter_uspace(): not yet implemented\n");
	return B_ERROR;
}


bool
arch_on_signal_stack(Thread *thread)
{
	return false;
}


status_t
arch_setup_signal_frame(Thread *thread, struct sigaction *sa,
	struct signal_frame_data *signalFrameData)
{
	return B_ERROR;
}


int64
arch_restore_signal_frame(struct signal_frame_data* signalFrameData)
{
	return 0;
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
}
