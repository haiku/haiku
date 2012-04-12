/*
 * Copyright 2003-2011, Haiku, Inc. All rights reserved.
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


#include <arch_thread.h>

#include <string.h>

#include <arch_cpu.h>
#include <arch/thread.h>
#include <boot/stage2.h>
#include <kernel.h>
#include <thread.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>
#include <arch_vm.h>
//#include <arch/vm_translation_map.h>

#include "paging/M68KPagingMethod.h"
#include "paging/M68KPagingStructures.h"
#include "paging/M68KVMTranslationMap.h"


#warning M68K: writeme!
// Valid initial arch_thread state. We just memcpy() it when initializing
// a new thread structure.
static struct arch_thread sInitialState;

Thread *gCurrentThread;

// Helper function for thread creation, defined in arch_asm.S.
extern "C" void m68k_kernel_thread_root();


void
m68k_push_iframe(struct iframe_stack *stack, struct iframe *frame)
{
	ASSERT(stack->index < IFRAME_TRACE_DEPTH);
	stack->frames[stack->index++] = frame;
}


void
m68k_pop_iframe(struct iframe_stack *stack)
{
	ASSERT(stack->index > 0);
	stack->index--;
}


/**	Returns the current iframe structure of the running thread.
 *	This function must only be called in a context where it's actually
 *	sure that such iframe exists; ie. from syscalls, but usually not
 *	from standard kernel threads.
 */
static struct iframe *
m68k_get_current_iframe(void)
{
	Thread *thread = thread_get_current_thread();

	ASSERT(thread->arch_info.iframes.index >= 0);
	return thread->arch_info.iframes.frames[thread->arch_info.iframes.index - 1];
}


/** \brief Returns the current thread's topmost (i.e. most recent)
 *  userland->kernel transition iframe (usually the first one, save for
 *  interrupts in signal handlers).
 *  \return The iframe, or \c NULL, if there is no such iframe (e.g. when
 *          the thread is a kernel thread).
 */
struct iframe *
m68k_get_user_iframe(void)
{
	Thread *thread = thread_get_current_thread();
	int i;

	for (i = thread->arch_info.iframes.index - 1; i >= 0; i--) {
		struct iframe *frame = thread->arch_info.iframes.frames[i];
		if ((frame->cpu.sr & (1 << M68K_SR_S)) == 0)
			return frame;
	}

	return NULL;
}


uint32
m68k_next_page_directory(Thread *from, Thread *to)
{
	VMAddressSpace* toAddressSpace = to->team->address_space;
	if (from->team->address_space == toAddressSpace) {
		// don't change the pgdir, same address space
		return 0;
	}

	if (toAddressSpace == NULL)
		toAddressSpace = VMAddressSpace::Kernel();

	return static_cast<M68KVMTranslationMap*>(toAddressSpace->TranslationMap())
		->PagingStructures()->pgroot_phys;
}

// #pragma mark -


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
#if 0
	addr_t *kstack = (addr_t *)t->kernel_stack_base;
	addr_t *kstackTop = (addr_t *)t->kernel_stack_base;

	// clear the kernel stack
#ifdef DEBUG_KERNEL_STACKS
#	ifdef STACK_GROWS_DOWNWARDS
	memset((void *)((addr_t)kstack + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE), 0,
		KERNEL_STACK_SIZE);
#	else
	memset(kstack, 0, KERNEL_STACK_SIZE);
#	endif
#else
	memset(kstack, 0, KERNEL_STACK_SIZE);
#endif

	// space for frame pointer and return address, and stack frames must be
	// 16 byte aligned
	kstackTop -= 2;
	kstackTop = (addr_t*)((addr_t)kstackTop & ~0xf);

	// LR, CR, r2, r13-r31, f13-f31, as pushed by m68k_context_switch()
	kstackTop -= 22 + 2 * 19;

	// let LR point to m68k_kernel_thread_root()
	kstackTop[0] = (addr_t)&m68k_kernel_thread_root;

	// the arguments of m68k_kernel_thread_root() are the functions to call,
	// provided in registers r13-r15
	kstackTop[3] = (addr_t)entry_func;
	kstackTop[4] = (addr_t)start_func;
	kstackTop[5] = (addr_t)exit_func;

	// save this stack position
	t->arch_info.sp = (void *)kstackTop;

	return B_OK;
#else
	panic("arch_thread_init_kthread_stack(): Implement me!");
#endif
}


status_t
arch_thread_init_tls(Thread *thread)
{
	// TODO: Implement!
	return B_OK;
}


void
arch_thread_context_switch(Thread *from, Thread *to)
{
	addr_t newPageDirectory;

	newPageDirectory = (addr_t)m68k_next_page_directory(from, to);

	if ((newPageDirectory % B_PAGE_SIZE) != 0)
		panic("arch_thread_context_switch: bad pgdir 0x%lx\n", newPageDirectory);
#warning M68K: export from arch_vm.c

	//m68k_set_pgdir((void *)newPageDirectory);
	gM68KPagingMethod->SetPageRoot(newPageDirectory);

	m68k_context_switch(&from->arch_info.sp, to->arch_info.sp);
}


void
arch_thread_dump_info(void *info)
{
	struct arch_thread *at = (struct arch_thread *)info;

	dprintf("\tsp: %p\n", at->sp);
}


status_t
arch_thread_enter_userspace(Thread *thread, addr_t entry, void *arg1, void *arg2)
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

