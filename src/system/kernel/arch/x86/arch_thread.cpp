/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <arch/thread.h>

#include <string.h>

#include <arch_cpu.h>
#include <cpu.h>
#include <kernel.h>
#include <ksignal.h>
#include <int.h>
#include <team.h>
#include <thread.h>
#include <tls.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#include "paging/X86PagingStructures.h"
#include "paging/X86VMTranslationMap.h"
#include "x86_syscalls.h"


// from arch_interrupts.S
extern "C" void x86_return_to_userland(iframe* frame);

// from arch_cpu.cpp
extern void (*gX86SwapFPUFunc)(void *oldState, const void *newState);


static struct iframe*
find_previous_iframe(Thread* thread, addr_t frame)
{
	// iterate backwards through the stack frames, until we hit an iframe
	while (frame >= thread->kernel_stack_base
		&& frame < thread->kernel_stack_top) {
		addr_t previousFrame = *(addr_t*)frame;
		if ((previousFrame & ~(addr_t)IFRAME_TYPE_MASK) == 0) {
			if (previousFrame == 0)
				return NULL;
			return (struct iframe*)frame;
		}

		frame = previousFrame;
	}

	return NULL;
}


static struct iframe*
get_previous_iframe(struct iframe* frame)
{
	if (frame == NULL)
		return NULL;

	return find_previous_iframe(thread_get_current_thread(), frame->bp);
}


/*!
	Returns the current iframe structure of the running thread.
	This function must only be called in a context where it's actually
	sure that such iframe exists; ie. from syscalls, but usually not
	from standard kernel threads.
*/
static struct iframe*
get_current_iframe(void)
{
	return find_previous_iframe(thread_get_current_thread(),
		x86_get_stack_frame());
}


/*!
	\brief Returns the current thread's topmost (i.e. most recent)
	userland->kernel transition iframe (usually the first one, save for
	interrupts in signal handlers).
	\return The iframe, or \c NULL, if there is no such iframe (e.g. when
			the thread is a kernel thread).
*/
struct iframe*
x86_get_user_iframe(void)
{
	struct iframe* frame = get_current_iframe();

	while (frame != NULL) {
		if (IFRAME_IS_USER(frame))
			return frame;
		frame = get_previous_iframe(frame);
	}

	return NULL;
}


/*!	\brief Like x86_get_user_iframe(), just for the given thread.
	The thread must not be running and the threads spinlock must be held.
*/
struct iframe*
x86_get_thread_user_iframe(Thread *thread)
{
	if (thread->state == B_THREAD_RUNNING)
		return NULL;

	// find the user iframe
	struct iframe* frame = find_previous_iframe(thread,
		thread->arch_info.GetFramePointer());

	while (frame != NULL) {
		if (IFRAME_IS_USER(frame))
			return frame;
		frame = get_previous_iframe(frame);
	}

	return NULL;
}


struct iframe*
x86_get_current_iframe(void)
{
	return get_current_iframe();
}


phys_addr_t
x86_next_page_directory(Thread* from, Thread* to)
{
	VMAddressSpace* toAddressSpace = to->team->address_space;
	if (from->team->address_space == toAddressSpace) {
		// don't change the pgdir, same address space
		return 0;
	}

	if (toAddressSpace == NULL)
		toAddressSpace = VMAddressSpace::Kernel();

	return static_cast<X86VMTranslationMap*>(toAddressSpace->TranslationMap())
		->PagingStructures()->pgdir_phys;
}


/*!	Returns to the userland environment given by \a frame for a thread not
	having been userland before.

	Before returning to userland all potentially necessary kernel exit work is
	done.

	\param thread The current thread.
	\param frame The iframe defining the userland environment. Must point to a
		location somewhere on the caller's stack (e.g. a local variable).
*/
void
x86_initial_return_to_userland(Thread* thread, iframe* frame)
{
	// disable interrupts and set up CPU specifics for this thread
	disable_interrupts();

	x86_set_tss_and_kstack(thread->kernel_stack_top);
	x86_set_tls_context(thread);
	x86_set_syscall_stack(thread->kernel_stack_top);

	// return to userland
	x86_return_to_userland(frame);
}


uint8*
x86_get_signal_stack(Thread* thread, struct iframe* frame,
	struct sigaction* action)
{
	// use the alternate signal stack if we should and can
	if (thread->signal_stack_enabled
		&& (action->sa_flags & SA_ONSTACK) != 0
		&& (frame->user_sp < thread->signal_stack_base
			|| frame->user_sp >= thread->signal_stack_base
				+ thread->signal_stack_size)) {
		return (uint8*)(thread->signal_stack_base + thread->signal_stack_size);
	}

	return (uint8*)frame->user_sp;
}


//	#pragma mark -


status_t
arch_team_init_team_struct(Team* p, bool kernel)
{
	return B_OK;
}


/*!	Initializes the user-space TLS local storage pointer in
	the thread structure, and the reserved TLS slots.
	
	Is called from _create_user_thread_kentry().
*/
status_t
arch_thread_init_tls(Thread* thread)
{
	size_t tls[TLS_USER_THREAD_SLOT + 1];

	thread->user_local_storage = thread->user_stack_base
		+ thread->user_stack_size;

	// initialize default TLS fields
	memset(tls, 0, sizeof(tls));
	tls[TLS_BASE_ADDRESS_SLOT] = thread->user_local_storage;
	tls[TLS_THREAD_ID_SLOT] = thread->id;
	tls[TLS_USER_THREAD_SLOT] = (addr_t)thread->user_thread;

	return user_memcpy((void*)thread->user_local_storage, tls, sizeof(tls));
}


void
arch_thread_context_switch(Thread* from, Thread* to)
{
	x86_set_tss_and_kstack(to->kernel_stack_top);
	x86_set_syscall_stack(to->kernel_stack_top);

	// set TLS GDT entry to the current thread - since this action is
	// dependent on the current CPU, we have to do it here
	if (to->user_local_storage != 0)
		x86_set_tls_context(to);

	struct cpu_ent* cpuData = to->cpu;
	X86PagingStructures* activePagingStructures
		= cpuData->arch.active_paging_structures;
	VMAddressSpace* toAddressSpace = to->team->address_space;

	X86PagingStructures* toPagingStructures;
	if (toAddressSpace != NULL
		&& (toPagingStructures = static_cast<X86VMTranslationMap*>(
				toAddressSpace->TranslationMap())->PagingStructures())
					!= activePagingStructures) {
		// update on which CPUs the address space is used
		int cpu = cpuData->cpu_num;
		atomic_and(&activePagingStructures->active_on_cpus,
			~((uint32)1 << cpu));
		atomic_or(&toPagingStructures->active_on_cpus, (uint32)1 << cpu);

		// assign the new paging structures to the CPU
		toPagingStructures->AddReference();
		cpuData->arch.active_paging_structures = toPagingStructures;

		// set the page directory, if it changes
		addr_t newPageDirectory = toPagingStructures->pgdir_phys;
		if (newPageDirectory != activePagingStructures->pgdir_phys)
			x86_swap_pgdir(newPageDirectory);

		// This CPU no longer uses the previous paging structures.
		activePagingStructures->RemoveReference();
	}

	gX86SwapFPUFunc(from->arch_info.fpu_state, to->arch_info.fpu_state);
	x86_context_switch(&from->arch_info, &to->arch_info);
}


bool
arch_on_signal_stack(Thread *thread)
{
	struct iframe* frame = get_current_iframe();

	return frame->user_sp >= thread->signal_stack_base
		&& frame->user_sp < thread->signal_stack_base
			+ thread->signal_stack_size;
}


/*!	Saves everything needed to restore the frame in the child fork in the
	arch_fork_arg structure to be passed to arch_restore_fork_frame().
	Also makes sure to return the right value.
*/
void
arch_store_fork_frame(struct arch_fork_arg* arg)
{
	struct iframe* frame = x86_get_current_iframe();

	// we need to copy the threads current iframe
	arg->iframe = *frame;

	// we also want fork() to return 0 for the child
	arg->iframe.ax = 0;
}


/*!	Restores the frame from a forked team as specified by the provided
	arch_fork_arg structure.
	Needs to be called from within the child team, i.e. instead of
	arch_thread_enter_userspace() as thread "starter".
	This function does not return to the caller, but will enter userland
	in the child team at the same position where the parent team left of.

	\param arg The architecture specific fork arguments including the
		environment to restore. Must point to a location somewhere on the
		caller's stack.
*/
void
arch_restore_fork_frame(struct arch_fork_arg* arg)
{
	x86_initial_return_to_userland(thread_get_current_thread(), &arg->iframe);
}
