/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <arch/thread.h>

#include <string.h>

#include <arch/user_debugger.h>
#include <arch_cpu.h>
#include <cpu.h>
#include <debug.h>
#include <kernel.h>
#include <ksignal.h>
#include <int.h>
#include <team.h>
#include <thread.h>
#include <tls.h>
#include <tracing.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#include "paging/X86PagingStructures.h"
#include "paging/X86VMTranslationMap.h"
#include "x86_syscalls.h"


//#define TRACE_ARCH_THREAD
#ifdef TRACE_ARCH_THREAD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#ifdef SYSCALL_TRACING

namespace SyscallTracing {

class RestartSyscall : public AbstractTraceEntry {
	public:
		RestartSyscall()
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("syscall restart");
		}
};

}

#	define TSYSCALL(x)	new(std::nothrow) SyscallTracing::x

#else
#	define TSYSCALL(x)
#endif	// SYSCALL_TRACING


// from arch_interrupts.S
extern "C" void i386_stack_init(struct farcall *interrupt_stack_offset);
extern "C" void i386_restore_frame_from_syscall(struct iframe frame);

// from arch_cpu.c
extern void (*gX86SwapFPUFunc)(void *oldState, const void *newState);
extern bool gHasSSE;

static struct arch_thread sInitialState _ALIGNED(16);
	// the fpu_state must be aligned on a 16 byte boundary, so that fxsave can use it


status_t
arch_thread_init(struct kernel_args *args)
{
	// save one global valid FPU state; it will be copied in the arch dependent
	// part of each new thread

	asm volatile ("clts; fninit; fnclex;");
	if (gHasSSE)
		i386_fxsave(sInitialState.fpu_state);
	else
		i386_fnsave(sInitialState.fpu_state);

	return B_OK;
}


static struct iframe *
find_previous_iframe(Thread *thread, addr_t frame)
{
	// iterate backwards through the stack frames, until we hit an iframe
	while (frame >= thread->kernel_stack_base
		&& frame < thread->kernel_stack_top) {
		addr_t previousFrame = *(addr_t*)frame;
		if ((previousFrame & ~IFRAME_TYPE_MASK) == 0) {
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

	return find_previous_iframe(thread_get_current_thread(), frame->ebp);
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
	return find_previous_iframe(thread_get_current_thread(), x86_read_ebp());
}


/*!
	\brief Returns the current thread's topmost (i.e. most recent)
	userland->kernel transition iframe (usually the first one, save for
	interrupts in signal handlers).
	\return The iframe, or \c NULL, if there is no such iframe (e.g. when
			the thread is a kernel thread).
*/
struct iframe *
i386_get_user_iframe(void)
{
	struct iframe* frame = get_current_iframe();

	while (frame != NULL) {
		if (IFRAME_IS_USER(frame))
			return frame;
		frame = get_previous_iframe(frame);
	}

	return NULL;
}


/*!	\brief Like i386_get_user_iframe(), just for the given thread.
	The thread must not be running and the threads spinlock must be held.
*/
struct iframe *
i386_get_thread_user_iframe(Thread *thread)
{
	if (thread->state == B_THREAD_RUNNING)
		return NULL;

	// read %ebp from the thread's stack stored by a pushad
	addr_t ebp = thread->arch_info.current_stack.esp[2];

	// find the user iframe
	struct iframe *frame = find_previous_iframe(thread, ebp);

	while (frame != NULL) {
		if (IFRAME_IS_USER(frame))
			return frame;
		frame = get_previous_iframe(frame);
	}

	return NULL;
}


struct iframe *
i386_get_current_iframe(void)
{
	return get_current_iframe();
}


uint32
x86_next_page_directory(Thread *from, Thread *to)
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


static inline void
set_fs_register(uint32 segment)
{
	asm("movl %0,%%fs" :: "r" (segment));
}


static void
set_tls_context(Thread *thread)
{
	int entry = smp_get_current_cpu() + TLS_BASE_SEGMENT;

	set_segment_descriptor_base(&gGDT[entry], thread->user_local_storage);
	set_fs_register((entry << 3) | DPL_USER);
}


void
x86_restart_syscall(struct iframe* frame)
{
	Thread* thread = thread_get_current_thread();

	atomic_and(&thread->flags, ~THREAD_FLAGS_RESTART_SYSCALL);
	atomic_or(&thread->flags, THREAD_FLAGS_SYSCALL_RESTARTED);

	frame->eax = frame->orig_eax;
	frame->edx = frame->orig_edx;
	frame->eip -= 2;
		// undoes the "int $99"/"sysenter"/"syscall" instruction
		// (so that it'll be executed again)

	TSYSCALL(RestartSyscall());
}


static uint32 *
get_signal_stack(Thread *thread, struct iframe *frame, int signal)
{
	// use the alternate signal stack if we should and can
	if (thread->signal_stack_enabled
		&& (thread->sig_action[signal - 1].sa_flags & SA_ONSTACK) != 0
		&& (frame->user_esp < thread->signal_stack_base
			|| frame->user_esp >= thread->signal_stack_base
				+ thread->signal_stack_size)) {
		return (uint32 *)(thread->signal_stack_base
			+ thread->signal_stack_size);
	}

	return (uint32 *)frame->user_esp;
}


//	#pragma mark -


status_t
arch_team_init_team_struct(Team *p, bool kernel)
{
	return B_OK;
}


status_t
arch_thread_init_thread_struct(Thread *thread)
{
	// set up an initial state (stack & fpu)
	memcpy(&thread->arch_info, &sInitialState, sizeof(struct arch_thread));
	return B_OK;
}


status_t
arch_thread_init_kthread_stack(Thread *t, int (*start_func)(void),
	void (*entry_func)(void), void (*exit_func)(void))
{
	addr_t *kstack = (addr_t *)t->kernel_stack_base;
	addr_t *kstack_top = (addr_t *)t->kernel_stack_top;
	int i;

	TRACE(("arch_thread_initialize_kthread_stack: kstack 0x%p, start_func 0x%p, entry_func 0x%p\n",
		kstack, start_func, entry_func));

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

	// set the final return address to be thread_kthread_exit
	kstack_top--;
	*kstack_top = (unsigned int)exit_func;

	// set the return address to be the start of the first function
	kstack_top--;
	*kstack_top = (unsigned int)start_func;

	// set the return address to be the start of the entry (thread setup)
	// function
	kstack_top--;
	*kstack_top = (unsigned int)entry_func;

	// simulate pushfl
//	kstack_top--;
//	*kstack_top = 0x00; // interrupts still disabled after the switch

	// simulate initial popad
	for (i = 0; i < 8; i++) {
		kstack_top--;
		*kstack_top = 0;
	}

	// save the stack position
	t->arch_info.current_stack.esp = kstack_top;
	t->arch_info.current_stack.ss = (addr_t *)KERNEL_DATA_SEG;

	return B_OK;
}


/** Initializes the user-space TLS local storage pointer in
 *	the thread structure, and the reserved TLS slots.
 *
 *	Is called from _create_user_thread_kentry().
 */

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


void
arch_thread_context_switch(Thread *from, Thread *to)
{
	i386_set_tss_and_kstack(to->kernel_stack_top);
	x86_set_syscall_stack(to->kernel_stack_top);

	// set TLS GDT entry to the current thread - since this action is
	// dependent on the current CPU, we have to do it here
	if (to->user_local_storage != 0)
		set_tls_context(to);

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
		uint32 newPageDirectory = toPagingStructures->pgdir_phys;
		if (newPageDirectory != activePagingStructures->pgdir_phys)
			x86_swap_pgdir(newPageDirectory);

		// This CPU no longer uses the previous paging structures.
		activePagingStructures->RemoveReference();
	}

	gX86SwapFPUFunc(from->arch_info.fpu_state, to->arch_info.fpu_state);
	x86_context_switch(&from->arch_info, &to->arch_info);
}


void
arch_thread_dump_info(void *info)
{
	struct arch_thread *at = (struct arch_thread *)info;

	kprintf("\tesp: %p\n", at->current_stack.esp);
	kprintf("\tss: %p\n", at->current_stack.ss);
	kprintf("\tfpu_state at %p\n", at->fpu_state);
}


/** Sets up initial thread context and enters user space
 */

status_t
arch_thread_enter_userspace(Thread *t, addr_t entry, void *args1,
	void *args2)
{
	addr_t stackTop = t->user_stack_base + t->user_stack_size;
	uint32 codeSize = (addr_t)x86_end_userspace_thread_exit
		- (addr_t)x86_userspace_thread_exit;
	uint32 args[3];

	TRACE(("arch_thread_enter_uspace: entry 0x%lx, args %p %p, ustack_top 0x%lx\n",
		entry, args1, args2, stackTop));

	// copy the little stub that calls exit_thread() when the thread entry
	// function returns, as well as the arguments of the entry function
	stackTop -= codeSize;

	if (user_memcpy((void *)stackTop, (const void *)&x86_userspace_thread_exit, codeSize) < B_OK)
		return B_BAD_ADDRESS;

	args[0] = stackTop;
	args[1] = (uint32)args1;
	args[2] = (uint32)args2;
	stackTop -= sizeof(args);

	if (user_memcpy((void *)stackTop, args, sizeof(args)) < B_OK)
		return B_BAD_ADDRESS;

	thread_at_kernel_exit();
		// also disables interrupts

	// install user breakpoints, if any
	if ((t->flags & THREAD_FLAGS_BREAKPOINTS_DEFINED) != 0)
		x86_init_user_debug_at_kernel_exit(NULL);

	i386_set_tss_and_kstack(t->kernel_stack_top);

	// set the CPU dependent GDT entry for TLS
	set_tls_context(t);

	x86_set_syscall_stack(t->kernel_stack_top);
	x86_enter_userspace(entry, stackTop);

	return B_OK;
		// never gets here
}


bool
arch_on_signal_stack(Thread *thread)
{
	struct iframe *frame = get_current_iframe();

	return frame->user_esp >= thread->signal_stack_base
		&& frame->user_esp < thread->signal_stack_base
			+ thread->signal_stack_size;
}


status_t
arch_setup_signal_frame(Thread *thread, struct sigaction *action,
	int signal, int signalMask)
{
	struct iframe *frame = get_current_iframe();
	if (!IFRAME_IS_USER(frame)) {
		panic("arch_setup_signal_frame(): No user iframe!");
		return B_BAD_VALUE;
	}

	uint32 *signalCode;
	uint32 *userRegs;
	struct vregs regs;
	uint32 buffer[6];
	status_t status;

	// start stuffing stuff on the user stack
	uint32* userStack = get_signal_stack(thread, frame, signal);

	// copy syscall restart info onto the user stack
	userStack -= (sizeof(thread->syscall_restart.parameters) + 12 + 3) / 4;
	uint32 threadFlags = atomic_and(&thread->flags,
		~(THREAD_FLAGS_RESTART_SYSCALL | THREAD_FLAGS_64_BIT_SYSCALL_RETURN));
	if (user_memcpy(userStack, &threadFlags, 4) < B_OK
		|| user_memcpy(userStack + 1, &frame->orig_eax, 4) < B_OK
		|| user_memcpy(userStack + 2, &frame->orig_edx, 4) < B_OK)
		return B_BAD_ADDRESS;
	status = user_memcpy(userStack + 3, thread->syscall_restart.parameters,
		sizeof(thread->syscall_restart.parameters));
	if (status < B_OK)
		return status;

	// store the saved regs onto the user stack
	regs.eip = frame->eip;
	regs.eflags = frame->flags;
	regs.eax = frame->eax;
	regs.ecx = frame->ecx;
	regs.edx = frame->edx;
	regs.ebp = frame->ebp;
	regs.esp = frame->esp;
	regs._reserved_1 = frame->user_esp;
	regs._reserved_2[0] = frame->edi;
	regs._reserved_2[1] = frame->esi;
	regs._reserved_2[2] = frame->ebx;
	i386_fnsave((void *)(&regs.xregs));

	userStack -= (sizeof(struct vregs) + 3) / 4;
	userRegs = userStack;
	status = user_memcpy(userRegs, &regs, sizeof(regs));
	if (status < B_OK)
		return status;

	// now store a code snippet on the stack
	userStack -= ((uint32)i386_end_return_from_signal + 3
		- (uint32)i386_return_from_signal) / 4;
	signalCode = userStack;
	status = user_memcpy(signalCode, (const void *)&i386_return_from_signal,
		((uint32)i386_end_return_from_signal
			- (uint32)i386_return_from_signal));
	if (status < B_OK)
		return status;

	// now set up the final part
	buffer[0] = (uint32)signalCode;	// return address when sa_handler done
	buffer[1] = signal;				// arguments to sa_handler
	buffer[2] = (uint32)action->sa_userdata;
	buffer[3] = (uint32)userRegs;

	buffer[4] = signalMask;			// Old signal mask to restore
	buffer[5] = (uint32)userRegs;	// Int frame + extra regs to restore

	userStack -= sizeof(buffer) / 4;

	status = user_memcpy(userStack, buffer, sizeof(buffer));
	if (status < B_OK)
		return status;

	frame->user_esp = (uint32)userStack;
	frame->eip = (uint32)action->sa_handler;

	return B_OK;
}


int64
arch_restore_signal_frame(void)
{
	Thread *thread = thread_get_current_thread();
	struct iframe *frame = get_current_iframe();
	int32 signalMask;
	uint32 *userStack;
	struct vregs* regsPointer;
	struct vregs regs;

	TRACE(("### arch_restore_signal_frame: entry\n"));

	userStack = (uint32 *)frame->user_esp;
	if (user_memcpy(&signalMask, &userStack[0], 4) < B_OK
		|| user_memcpy(&regsPointer, &userStack[1], 4) < B_OK
		|| user_memcpy(&regs, regsPointer, sizeof(vregs)) < B_OK) {
		return B_BAD_ADDRESS;
	}

	uint32* syscallRestartInfo
		= (uint32*)regsPointer + (sizeof(struct vregs) + 3) / 4;
	uint32 threadFlags;
	if (user_memcpy(&threadFlags, syscallRestartInfo, 4) < B_OK
		|| user_memcpy(&frame->orig_eax, syscallRestartInfo + 1, 4) < B_OK
		|| user_memcpy(&frame->orig_edx, syscallRestartInfo + 2, 4) < B_OK
		|| user_memcpy(thread->syscall_restart.parameters,
			syscallRestartInfo + 3,
			sizeof(thread->syscall_restart.parameters)) < B_OK) {
		return B_BAD_ADDRESS;
	}

	// set restart/64bit return value flags from previous syscall
	atomic_and(&thread->flags,
		~(THREAD_FLAGS_RESTART_SYSCALL | THREAD_FLAGS_64_BIT_SYSCALL_RETURN));
	atomic_or(&thread->flags, threadFlags
		& (THREAD_FLAGS_RESTART_SYSCALL | THREAD_FLAGS_64_BIT_SYSCALL_RETURN));

	// TODO: Verify that just restoring the old signal mask is right! Bash for
	// instance changes the procmask in a signal handler. Those changes are
	// lost the way we do it.
	atomic_set(&thread->sig_block_mask, signalMask);
	update_current_thread_signals_flag();

	frame->eip = regs.eip;
	frame->flags = regs.eflags;
	frame->eax = regs.eax;
	frame->ecx = regs.ecx;
	frame->edx = regs.edx;
	frame->ebp = regs.ebp;
	frame->esp = regs.esp;
	frame->user_esp = regs._reserved_1;
	frame->edi = regs._reserved_2[0];
	frame->esi = regs._reserved_2[1];
	frame->ebx = regs._reserved_2[2];

	i386_frstor((void *)(&regs.xregs));

	TRACE(("### arch_restore_signal_frame: exit\n"));

	return (int64)frame->eax | ((int64)frame->edx << 32);
}


/**	Saves everything needed to restore the frame in the child fork in the
 *	arch_fork_arg structure to be passed to arch_restore_fork_frame().
 *	Also makes sure to return the right value.
 */

void
arch_store_fork_frame(struct arch_fork_arg *arg)
{
	struct iframe *frame = get_current_iframe();

	// we need to copy the threads current iframe
	arg->iframe = *frame;

	// we also want fork() to return 0 for the child
	arg->iframe.eax = 0;
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
	Thread *thread = thread_get_current_thread();

	disable_interrupts();

	i386_set_tss_and_kstack(thread->kernel_stack_top);

	// set the CPU dependent GDT entry for TLS (set the current %fs register)
	set_tls_context(thread);

	i386_restore_frame_from_syscall(arg->iframe);
}


void
arch_syscall_64_bit_return_value(void)
{
	Thread* thread = thread_get_current_thread();
	atomic_or(&thread->flags, THREAD_FLAGS_64_BIT_SYSCALL_RETURN);
}
