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
#include <util/AutoLock.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#include "paging/X86PagingStructures.h"
#include "paging/X86VMTranslationMap.h"
#include "x86_signals.h"
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
extern "C" void x86_return_to_userland(iframe* frame);

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


static inline void
set_fs_register(uint32 segment)
{
	asm("movl %0,%%fs" :: "r" (segment));
}


/*!	Returns to the userland environment given by \a frame for a thread not
	having been userland before.

	Before returning to userland all potentially necessary kernel exit work is
	done.

	\param thread The current thread.
	\param frame The iframe defining the userland environment. Must point to a
		location somewhere on the caller's stack (e.g. a local variable).
*/
static void
initial_return_to_userland(Thread* thread, iframe* frame)
{
	// disable interrupts and set up CPU specifics for this thread
	disable_interrupts();

	i386_set_tss_and_kstack(thread->kernel_stack_top);
	x86_set_tls_context(thread);
	x86_set_syscall_stack(thread->kernel_stack_top);

	// return to userland
	x86_return_to_userland(frame);
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


void
x86_set_tls_context(Thread *thread)
{
	int entry = smp_get_current_cpu() + TLS_BASE_SEGMENT;

	set_segment_descriptor_base(&gGDT[entry], thread->user_local_storage);
	set_fs_register((entry << 3) | DPL_USER);
}


static uint8*
get_signal_stack(Thread* thread, struct iframe* frame, struct sigaction* action)
{
	// use the alternate signal stack if we should and can
	if (thread->signal_stack_enabled
		&& (action->sa_flags & SA_ONSTACK) != 0
		&& (frame->user_esp < thread->signal_stack_base
			|| frame->user_esp >= thread->signal_stack_base
				+ thread->signal_stack_size)) {
		return (uint8*)(thread->signal_stack_base + thread->signal_stack_size);
	}

	return (uint8*)frame->user_esp;
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


/*!	Prepares the given thread's kernel stack for executing its entry function.

	\param thread The thread.
	\param stack The usable bottom of the thread's kernel stack.
	\param stackTop The usable top of the thread's kernel stack.
	\param function The entry function the thread shall execute.
	\param data Pointer to be passed to the entry function.
*/
void
arch_thread_init_kthread_stack(Thread* thread, void* _stack, void* _stackTop,
	void (*function)(void*), const void* data)
{
	addr_t* stackTop = (addr_t*)_stackTop;

	TRACE(("arch_thread_init_kthread_stack: stack top %p, function %, data: "
		"%p\n", stackTop, function, data));

	// push the function argument, a pointer to the data
	*--stackTop = (addr_t)data;

	// push a dummy return address for the function
	*--stackTop = 0;

	// push the function address -- that's the return address used after the
	// context switch
	*--stackTop = (addr_t)function;

	// simulate pushad as done by x86_context_switch()
	for (int i = 0; i < 8; i++)
		*--stackTop = 0;

	// save the stack position
	thread->arch_info.current_stack.esp = stackTop;
	thread->arch_info.current_stack.ss = (addr_t*)KERNEL_DATA_SEG;
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


/*!	Sets up initial thread context and enters user space
*/
status_t
arch_thread_enter_userspace(Thread* thread, addr_t entry, void* args1,
	void* args2)
{
	addr_t stackTop = thread->user_stack_base + thread->user_stack_size;
	uint32 codeSize = (addr_t)x86_end_userspace_thread_exit
		- (addr_t)x86_userspace_thread_exit;
	uint32 args[3];

	TRACE(("arch_thread_enter_userspace: entry 0x%lx, args %p %p, "
		"ustack_top 0x%lx\n", entry, args1, args2, stackTop));

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

	// prepare the user iframe
	iframe frame = {};
	frame.type = IFRAME_TYPE_SYSCALL;
	frame.gs = USER_DATA_SEG;
	// frame.fs not used, we call x86_set_tls_context() on context switch
	frame.es = USER_DATA_SEG;
	frame.ds = USER_DATA_SEG;
	frame.eip = entry;
	frame.cs = USER_CODE_SEG;
	frame.flags = X86_EFLAGS_RESERVED1 | X86_EFLAGS_INTERRUPT
		| (3 << X86_EFLAGS_IO_PRIVILEG_LEVEL_SHIFT);
	frame.user_esp = stackTop;
	frame.user_ss = USER_DATA_SEG;

	// return to userland
	initial_return_to_userland(thread, &frame);

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


/*!	Sets up the user iframe for invoking a signal handler.

	The function fills in the remaining fields of the given \a signalFrameData,
	copies it to the thread's userland stack (the one on which the signal shall
	be handled), and sets up the user iframe so that when returning to userland
	a wrapper function is executed that calls the user-defined signal handler.
	When the signal handler returns, the wrapper function shall call the
	"restore signal frame" syscall with the (possibly modified) signal frame
	data.

	The following fields of the \a signalFrameData structure still need to be
	filled in:
	- \c context.uc_stack: The stack currently used by the thread.
	- \c context.uc_mcontext: The current userland state of the registers.
	- \c syscall_restart_return_value: Architecture specific use. On x86 the
		value of eax and edx which are overwritten by the syscall return value.

	Furthermore the function needs to set \c thread->user_signal_context to the
	userland pointer to the \c ucontext_t on the user stack.

	\param thread The current thread.
	\param action The signal action specified for the signal to be handled.
	\param signalFrameData A partially initialized structure of all the data
		that need to be copied to userland.
	\return \c B_OK on success, another error code, if something goes wrong.
*/
status_t
arch_setup_signal_frame(Thread* thread, struct sigaction* action,
	struct signal_frame_data* signalFrameData)
{
	struct iframe *frame = get_current_iframe();
	if (!IFRAME_IS_USER(frame)) {
		panic("arch_setup_signal_frame(): No user iframe!");
		return B_BAD_VALUE;
	}

	// In case of a BeOS compatible handler map SIGBUS to SIGSEGV, since they
	// had the same signal number.
	if ((action->sa_flags & SA_BEOS_COMPATIBLE_HANDLER) != 0
		&& signalFrameData->info.si_signo == SIGBUS) {
		signalFrameData->info.si_signo = SIGSEGV;
	}

	// store the register state in signalFrameData->context.uc_mcontext
	signalFrameData->context.uc_mcontext.eip = frame->eip;
	signalFrameData->context.uc_mcontext.eflags = frame->flags;
	signalFrameData->context.uc_mcontext.eax = frame->eax;
	signalFrameData->context.uc_mcontext.ecx = frame->ecx;
	signalFrameData->context.uc_mcontext.edx = frame->edx;
	signalFrameData->context.uc_mcontext.ebp = frame->ebp;
	signalFrameData->context.uc_mcontext.esp = frame->user_esp;
	signalFrameData->context.uc_mcontext.edi = frame->edi;
	signalFrameData->context.uc_mcontext.esi = frame->esi;
	signalFrameData->context.uc_mcontext.ebx = frame->ebx;
	i386_fnsave((void *)(&signalFrameData->context.uc_mcontext.xregs));

	// fill in signalFrameData->context.uc_stack
	signal_get_user_stack(frame->user_esp, &signalFrameData->context.uc_stack);

	// store orig_eax/orig_edx in syscall_restart_return_value
	signalFrameData->syscall_restart_return_value
		= (uint64)frame->orig_edx << 32 | frame->orig_eax;

	// get the stack to use -- that's either the current one or a special signal
	// stack
	uint8* userStack = get_signal_stack(thread, frame, action);

	// copy the signal frame data onto the stack
	userStack -= sizeof(*signalFrameData);
	signal_frame_data* userSignalFrameData = (signal_frame_data*)userStack;
	if (user_memcpy(userSignalFrameData, signalFrameData,
			sizeof(*signalFrameData)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	// prepare the user stack frame for a function call to the signal handler
	// wrapper function
	uint32 stackFrame[2] = {
		frame->eip,		// return address
		(addr_t)userSignalFrameData, // parameter: pointer to signal frame data
	};

	userStack -= sizeof(stackFrame);
	if (user_memcpy(userStack, stackFrame, sizeof(stackFrame)) != B_OK)
		return B_BAD_ADDRESS;

	// Update Thread::user_signal_context, now that everything seems to have
	// gone fine.
	thread->user_signal_context = &userSignalFrameData->context;

	// Adjust the iframe's esp and eip, so that the thread will continue with
	// the prepared stack, executing the signal handler wrapper function.
	frame->user_esp = (addr_t)userStack;
	frame->eip = x86_get_user_signal_handler_wrapper(
		(action->sa_flags & SA_BEOS_COMPATIBLE_HANDLER) != 0);

	return B_OK;
}


int64
arch_restore_signal_frame(struct signal_frame_data* signalFrameData)
{
	struct iframe* frame = get_current_iframe();

	TRACE(("### arch_restore_signal_frame: entry\n"));

	frame->orig_eax = (uint32)signalFrameData->syscall_restart_return_value;
	frame->orig_edx
		= (uint32)(signalFrameData->syscall_restart_return_value >> 32);

	frame->eip = signalFrameData->context.uc_mcontext.eip;
	frame->flags = (frame->flags & ~(uint32)X86_EFLAGS_USER_FLAGS)
		| (signalFrameData->context.uc_mcontext.eflags & X86_EFLAGS_USER_FLAGS);
	frame->eax = signalFrameData->context.uc_mcontext.eax;
	frame->ecx = signalFrameData->context.uc_mcontext.ecx;
	frame->edx = signalFrameData->context.uc_mcontext.edx;
	frame->ebp = signalFrameData->context.uc_mcontext.ebp;
	frame->user_esp = signalFrameData->context.uc_mcontext.esp;
	frame->edi = signalFrameData->context.uc_mcontext.edi;
	frame->esi = signalFrameData->context.uc_mcontext.esi;
	frame->ebx = signalFrameData->context.uc_mcontext.ebx;

	i386_frstor((void*)(&signalFrameData->context.uc_mcontext.xregs));

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
	initial_return_to_userland(thread_get_current_thread(), &arg->iframe);
}


void
arch_syscall_64_bit_return_value(void)
{
	Thread* thread = thread_get_current_thread();
	atomic_or(&thread->flags, THREAD_FLAGS_64_BIT_SYSCALL_RETURN);
}
