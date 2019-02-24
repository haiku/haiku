/* Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <arch_cpu.h>
#include <arch/thread.h>
#include <boot/stage2.h>
#include <kernel.h>
#include <thread.h>
#include <vm/vm_types.h>


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
	//memcpy(&thread->arch_info, &sInitialState, sizeof(struct arch_thread));

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
}


void
arch_thread_dump_info(void *info)
{
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

