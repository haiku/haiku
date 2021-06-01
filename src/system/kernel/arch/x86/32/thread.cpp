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
#include <commpage.h>
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
#include <util/Random.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#include "paging/X86PagingStructures.h"
#include "paging/X86VMTranslationMap.h"
#include "x86_signals.h"


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


// from arch_cpu.cpp
extern bool gHasSSE;

static struct arch_thread sInitialState _ALIGNED(16);
	// the fpu_state must be aligned on a 16 byte boundary, so that fxsave can use it


static inline void
set_fs_register(uint32 segment)
{
	asm("movl %0,%%fs" :: "r" (segment));
}


void
x86_restart_syscall(struct iframe* frame)
{
	Thread* thread = thread_get_current_thread();

	atomic_and(&thread->flags, ~THREAD_FLAGS_RESTART_SYSCALL);
	atomic_or(&thread->flags, THREAD_FLAGS_SYSCALL_RESTARTED);

	frame->ax = frame->orig_eax;
	frame->dx = frame->orig_edx;
	frame->ip -= 2;
		// undoes the "int $99"/"sysenter"/"syscall" instruction
		// (so that it'll be executed again)

	TSYSCALL(RestartSyscall());
}


void
x86_set_tls_context(Thread *thread)
{
	segment_descriptor* gdt = get_gdt(smp_get_current_cpu());
	set_segment_descriptor_base(&gdt[USER_TLS_SEGMENT],
		thread->user_local_storage);
	set_fs_register((USER_TLS_SEGMENT << 3) | DPL_USER);
}


static addr_t
arch_randomize_stack_pointer(addr_t value)
{
	STATIC_ASSERT(MAX_RANDOM_VALUE >= B_PAGE_SIZE - 1);
	value -= random_value() & (B_PAGE_SIZE - 1);
	return (value & ~addr_t(0xf)) - 4;
		// This means, result % 16 == 12, which is what esp should adhere to
		// when a function is entered for the stack to be considered aligned to
		// 16 byte.
}


static uint8*
get_signal_stack(Thread* thread, struct iframe* frame, struct sigaction* action,
	size_t spaceNeeded)
{
	// use the alternate signal stack if we should and can
	if (thread->signal_stack_enabled
		&& (action->sa_flags & SA_ONSTACK) != 0
		&& (frame->user_sp < thread->signal_stack_base
			|| frame->user_sp >= thread->signal_stack_base
				+ thread->signal_stack_size)) {
		addr_t stackTop = thread->signal_stack_base + thread->signal_stack_size;
		return (uint8*)arch_randomize_stack_pointer(stackTop - spaceNeeded);
	}

	return (uint8*)((frame->user_sp - spaceNeeded) & ~addr_t(0xf)) - 4;
		// align stack pointer (cf. arch_randomize_stack_pointer())
}


//	#pragma mark -


status_t
arch_thread_init(struct kernel_args *args)
{
	// save one global valid FPU state; it will be copied in the arch dependent
	// part of each new thread

	asm volatile ("clts; fninit; fnclex;");
	if (gHasSSE)
		x86_fxsave(sInitialState.fpu_state);
	else
		x86_fnsave(sInitialState.fpu_state);

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

	TRACE(("arch_thread_init_kthread_stack: stack top %p, function %p, data: "
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
	thread->arch_info.current_stack.ss = (addr_t*)KERNEL_DATA_SELECTOR;
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
	uint32 args[3];

	TRACE(("arch_thread_enter_userspace: entry 0x%lx, args %p %p, "
		"ustack_top 0x%lx\n", entry, args1, args2, stackTop));

	stackTop = arch_randomize_stack_pointer(stackTop - sizeof(args));

	// Copy the address of the stub that calls exit_thread() when the thread
	// entry function returns to the top of the stack to act as the return
	// address. The stub is inside commpage.
	addr_t commPageAddress = (addr_t)thread->team->commpage_address;
	args[0] = ((addr_t*)commPageAddress)[COMMPAGE_ENTRY_X86_THREAD_EXIT]
		+ commPageAddress;
	args[1] = (uint32)args1;
	args[2] = (uint32)args2;

	if (user_memcpy((void *)stackTop, args, sizeof(args)) < B_OK)
		return B_BAD_ADDRESS;

	// prepare the user iframe
	iframe frame = {};
	frame.type = IFRAME_TYPE_SYSCALL;
	frame.gs = USER_DATA_SELECTOR;
	// frame.fs not used, we call x86_set_tls_context() on context switch
	frame.es = USER_DATA_SELECTOR;
	frame.ds = USER_DATA_SELECTOR;
	frame.ip = entry;
	frame.cs = USER_CODE_SELECTOR;
	frame.flags = X86_EFLAGS_RESERVED1 | X86_EFLAGS_INTERRUPT
		| (3 << X86_EFLAGS_IO_PRIVILEG_LEVEL_SHIFT);
	frame.user_sp = stackTop;
	frame.user_ss = USER_DATA_SELECTOR;

	// return to userland
	x86_initial_return_to_userland(thread, &frame);

	return B_OK;
		// never gets here
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
	struct iframe *frame = x86_get_current_iframe();
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
	signalFrameData->context.uc_mcontext.eip = frame->ip;
	signalFrameData->context.uc_mcontext.eflags = frame->flags;
	signalFrameData->context.uc_mcontext.eax = frame->ax;
	signalFrameData->context.uc_mcontext.ecx = frame->cx;
	signalFrameData->context.uc_mcontext.edx = frame->dx;
	signalFrameData->context.uc_mcontext.ebp = frame->bp;
	signalFrameData->context.uc_mcontext.esp = frame->user_sp;
	signalFrameData->context.uc_mcontext.edi = frame->di;
	signalFrameData->context.uc_mcontext.esi = frame->si;
	signalFrameData->context.uc_mcontext.ebx = frame->bx;
	x86_fnsave((void *)(&signalFrameData->context.uc_mcontext.xregs));

	// Fill in signalFrameData->context.uc_stack
	signal_get_user_stack(frame->user_sp, &signalFrameData->context.uc_stack);

	// store orig_eax/orig_edx in syscall_restart_return_value
	signalFrameData->syscall_restart_return_value
		= (uint64)frame->orig_edx << 32 | frame->orig_eax;

	// get the stack to use -- that's either the current one or a special signal
	// stack
	uint32 stackFrame[2];
	uint8* userStack = get_signal_stack(thread, frame, action,
		sizeof(*signalFrameData) + sizeof(stackFrame));

	// copy the signal frame data onto the stack
	signal_frame_data* userSignalFrameData
		= (signal_frame_data*)(userStack + sizeof(stackFrame));
	if (user_memcpy(userSignalFrameData, signalFrameData,
			sizeof(*signalFrameData)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	// prepare the user stack frame for a function call to the signal handler
	// wrapper function
	stackFrame[0] = frame->ip;
	stackFrame[1] = (addr_t)userSignalFrameData;
		// parameter: pointer to signal frame data

	if (user_memcpy(userStack, stackFrame, sizeof(stackFrame)) != B_OK)
		return B_BAD_ADDRESS;

	// Update Thread::user_signal_context, now that everything seems to have
	// gone fine.
	thread->user_signal_context = &userSignalFrameData->context;

	// Adjust the iframe's esp and eip, so that the thread will continue with
	// the prepared stack, executing the signal handler wrapper function.
	frame->user_sp = (addr_t)userStack;
	frame->ip = x86_get_user_signal_handler_wrapper(
		(action->sa_flags & SA_BEOS_COMPATIBLE_HANDLER) != 0,
		thread->team->commpage_address);
	frame->flags &= ~(X86_EFLAGS_TRAP | X86_EFLAGS_DIRECTION);

	return B_OK;
}


int64
arch_restore_signal_frame(struct signal_frame_data* signalFrameData)
{
	struct iframe* frame = x86_get_current_iframe();

	TRACE(("### arch_restore_signal_frame: entry\n"));

	frame->orig_eax = (uint32)signalFrameData->syscall_restart_return_value;
	frame->orig_edx
		= (uint32)(signalFrameData->syscall_restart_return_value >> 32);

	frame->ip = signalFrameData->context.uc_mcontext.eip;
	frame->flags = (frame->flags & ~(uint32)X86_EFLAGS_USER_FLAGS)
		| (signalFrameData->context.uc_mcontext.eflags & X86_EFLAGS_USER_FLAGS);
	frame->ax = signalFrameData->context.uc_mcontext.eax;
	frame->cx = signalFrameData->context.uc_mcontext.ecx;
	frame->dx = signalFrameData->context.uc_mcontext.edx;
	frame->bp = signalFrameData->context.uc_mcontext.ebp;
	frame->user_sp = signalFrameData->context.uc_mcontext.esp;
	frame->di = signalFrameData->context.uc_mcontext.edi;
	frame->si = signalFrameData->context.uc_mcontext.esi;
	frame->bx = signalFrameData->context.uc_mcontext.ebx;

	x86_frstor((void*)(&signalFrameData->context.uc_mcontext.xregs));

	TRACE(("### arch_restore_signal_frame: exit\n"));

	return (int64)frame->ax | ((int64)frame->dx << 32);
}


void
arch_syscall_64_bit_return_value(void)
{
	Thread* thread = thread_get_current_thread();
	atomic_or(&thread->flags, THREAD_FLAGS_64_BIT_SYSCALL_RETURN);
}
