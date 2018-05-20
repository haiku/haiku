/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <arch/thread.h>

#include <string.h>

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
#include <util/Random.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#ifdef _COMPAT_MODE
#	include <compat/ksignal_compat.h>
#endif

#include "paging/X86PagingStructures.h"
#include "paging/X86VMTranslationMap.h"


//#define TRACE_ARCH_THREAD
#ifdef TRACE_ARCH_THREAD
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
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


extern "C" void x86_64_thread_entry();

// Initial thread saved state.
static arch_thread sInitialState;


void
x86_restart_syscall(iframe* frame)
{
	Thread* thread = thread_get_current_thread();

	atomic_and(&thread->flags, ~THREAD_FLAGS_RESTART_SYSCALL);
	atomic_or(&thread->flags, THREAD_FLAGS_SYSCALL_RESTARTED);

	// Get back the original system call number and modify the frame to
	// re-execute the syscall instruction.
	frame->ax = frame->orig_rax;
	frame->ip -= 2;

	TSYSCALL(RestartSyscall());
}


#ifdef _COMPAT_MODE


static inline void
set_fs_register(uint32 segment)
{
	asm("movl %0,%%fs" :: "r" (segment));
}


#endif // _COMPAT_MODE


void
x86_set_tls_context(Thread* thread)
{
#ifdef _COMPAT_MODE
	if ((thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0) {
		unsigned index = x86_64_set_user_tls_segment_base(
			smp_get_current_cpu(), thread->user_local_storage);
		set_fs_register((index << 3) | DPL_USER);
	} else
#endif
	{
		// Set FS segment base address to the TLS segment.
		x86_write_msr(IA32_MSR_FS_BASE, thread->user_local_storage);
	}
}


static addr_t
arch_randomize_stack_pointer(addr_t value)
{
	static_assert(MAX_RANDOM_VALUE >= B_PAGE_SIZE - 1,
		"randomization range is too big");
	value -= random_value() & (B_PAGE_SIZE - 1);
	return (value & ~addr_t(0xf)) - 8;
		// This means, result % 16 == 8, which is what rsp should adhere to
		// when a function is entered for the stack to be considered aligned to
		// 16 byte.
}


#ifdef _COMPAT_MODE


static addr_t
arch_compat_randomize_stack_pointer(addr_t value)
{
	STATIC_ASSERT(MAX_RANDOM_VALUE >= B_PAGE_SIZE - 1);
	value -= random_value() & (B_PAGE_SIZE - 1);
	return (value & ~addr_t(0xf)) - 4;
		// This means, result % 16 == 12, which is what esp should adhere to
		// when a function is entered for the stack to be considered aligned to
		// 16 byte.
}


#endif // _COMPAT_MODE


static uint8*
get_signal_stack(Thread* thread, iframe* frame, struct sigaction* action,
	size_t spaceNeeded)
{
	// Use the alternate signal stack if we should and can.
	if (thread->signal_stack_enabled
			&& (action->sa_flags & SA_ONSTACK) != 0
			&& (frame->user_sp < thread->signal_stack_base
				|| frame->user_sp >= thread->signal_stack_base
					+ thread->signal_stack_size)) {
		addr_t stackTop = thread->signal_stack_base + thread->signal_stack_size;
		return (uint8*)arch_randomize_stack_pointer(stackTop - spaceNeeded);
	}

	// We are going to use the stack that we are already on. We must not touch
	// the red zone (128 byte area below the stack pointer, reserved for use
	// by functions to store temporary data and guaranteed not to be modified
	// by signal handlers).
	return (uint8*)((frame->user_sp - 128 - spaceNeeded) & ~addr_t(0xf)) - 8;
		// align stack pointer (cf. arch_randomize_stack_pointer())
}


//	#pragma mark -


status_t
arch_thread_init(kernel_args* args)
{
	// Save one global valid FPU state; it will be copied in the arch dependent
	// part of each new thread.
	asm volatile (
		"clts;"		\
		"fninit;"	\
		"fnclex;"	\
		"fxsave %0;"
		: "=m" (sInitialState.fpu_state));
	return B_OK;
}


status_t
arch_thread_init_thread_struct(Thread* thread)
{
	// Copy the initial saved FPU state to the new thread.
	memcpy(&thread->arch_info, &sInitialState, sizeof(arch_thread));

	// Initialise the current thread pointer.
	thread->arch_info.thread = thread;

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
	uintptr_t* stackTop = static_cast<uintptr_t*>(_stackTop);

	TRACE("arch_thread_init_kthread_stack: stack top %p, function %p, data: "
		"%p\n", _stackTop, function, data);

	// Save the stack top for system call entry.
	thread->arch_info.syscall_rsp = (uint64*)thread->kernel_stack_top;

	thread->arch_info.instruction_pointer
		= reinterpret_cast<uintptr_t>(x86_64_thread_entry);

	*--stackTop = uintptr_t(data);
	*--stackTop = uintptr_t(function);

	// Save the stack position.
	thread->arch_info.current_stack = stackTop;
}


void
arch_thread_dump_info(void* info)
{
	arch_thread* thread = (arch_thread*)info;

	kprintf("\trsp: %p\n", thread->current_stack);
	kprintf("\tsyscall_rsp: %p\n", thread->syscall_rsp);
	kprintf("\tuser_rsp: %p\n", thread->user_rsp);
	kprintf("\tfpu_state at %p\n", thread->fpu_state);
}


/*!	Sets up initial thread context and enters user space
*/
status_t
arch_thread_enter_userspace(Thread* thread, addr_t entry, void* args1,
	void* args2)
{
	addr_t stackTop = thread->user_stack_base + thread->user_stack_size;
	addr_t codeAddr;

	TRACE("arch_thread_enter_userspace: entry %#lx, args %p %p, "
		"stackTop %#lx\n", entry, args1, args2, stackTop);

	// Copy the address of the stub that calls exit_thread() when the thread
	// entry function returns to the top of the stack to act as the return
	// address. The stub is inside commpage.
	addr_t commPageAddress = (addr_t)thread->team->commpage_address;

#ifdef _COMPAT_MODE
	if ((thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0) {
		uint32 args[3];
		stackTop = arch_compat_randomize_stack_pointer(stackTop - sizeof(args));

		set_ac();
		args[0] = ((addr_t*)commPageAddress)[COMMPAGE_ENTRY_X86_THREAD_EXIT]
			+ commPageAddress;
		clear_ac();
		args[1] = (uint32)(addr_t)args1;
		args[2] = (uint32)(addr_t)args2;
		if (user_memcpy((void *)stackTop, args, sizeof(args)) < B_OK)
			return B_BAD_ADDRESS;

	} else
#endif
	{
		stackTop = arch_randomize_stack_pointer(stackTop - sizeof(codeAddr));

		set_ac();
		codeAddr = ((addr_t*)commPageAddress)[COMMPAGE_ENTRY_X86_THREAD_EXIT]
			+ commPageAddress;
		clear_ac();
		if (user_memcpy((void*)stackTop, (const void*)&codeAddr, sizeof(codeAddr))
			!= B_OK)
			return B_BAD_ADDRESS;
	}

	// Prepare the user iframe.
	iframe frame = {};
	frame.type = IFRAME_TYPE_SYSCALL;
	frame.ip = entry;
#ifdef _COMPAT_MODE
	if ((thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0) {
		frame.cs = USER32_CODE_SELECTOR;
		arch_thread_set_ds(USER_DATA_SELECTOR);
		arch_thread_set_es(USER_DATA_SELECTOR);
	} else
#endif
	{
		frame.si = (uint64)args2;
		frame.di = (uint64)args1;
		frame.cs = USER_CODE_SELECTOR;
	}
	frame.flags = X86_EFLAGS_RESERVED1 | X86_EFLAGS_INTERRUPT
		| (3 << X86_EFLAGS_IO_PRIVILEG_LEVEL_SHIFT);
	frame.sp = stackTop;
	frame.ss = USER_DATA_SELECTOR;

	// Return to userland. Never returns.
	x86_initial_return_to_userland(thread, &frame);

	return B_OK;
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
	- \c syscall_restart_return_value: Architecture specific use. On x86_64 the
		value of rax which is overwritten by the syscall return value.

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
	iframe* frame = x86_get_current_iframe();
	if (!IFRAME_IS_USER(frame)) {
		panic("arch_setup_signal_frame(): No user iframe!");
		return B_BAD_VALUE;
	}

	// Store the register state.
	signalFrameData->context.uc_mcontext.rax = frame->ax;
	signalFrameData->context.uc_mcontext.rbx = frame->bx;
	signalFrameData->context.uc_mcontext.rcx = frame->cx;
	signalFrameData->context.uc_mcontext.rdx = frame->dx;
	signalFrameData->context.uc_mcontext.rdi = frame->di;
	signalFrameData->context.uc_mcontext.rsi = frame->si;
	signalFrameData->context.uc_mcontext.rbp = frame->bp;
	signalFrameData->context.uc_mcontext.r8 = frame->r8;
	signalFrameData->context.uc_mcontext.r9 = frame->r9;
	signalFrameData->context.uc_mcontext.r10 = frame->r10;
	signalFrameData->context.uc_mcontext.r11 = frame->r11;
	signalFrameData->context.uc_mcontext.r12 = frame->r12;
	signalFrameData->context.uc_mcontext.r13 = frame->r13;
	signalFrameData->context.uc_mcontext.r14 = frame->r14;
	signalFrameData->context.uc_mcontext.r15 = frame->r15;
	signalFrameData->context.uc_mcontext.rsp = frame->user_sp;
	signalFrameData->context.uc_mcontext.rip = frame->ip;
	signalFrameData->context.uc_mcontext.rflags = frame->flags;

	if (frame->fpu != nullptr) {
		memcpy((void*)&signalFrameData->context.uc_mcontext.fpu, frame->fpu,
			sizeof(signalFrameData->context.uc_mcontext.fpu));
	} else {
		memcpy((void*)&signalFrameData->context.uc_mcontext.fpu,
			sInitialState.fpu_state,
			sizeof(signalFrameData->context.uc_mcontext.fpu));
	}

	// Fill in signalFrameData->context.uc_stack.
	signal_get_user_stack(frame->user_sp, &signalFrameData->context.uc_stack);

	// Store syscall_restart_return_value.
	signalFrameData->syscall_restart_return_value = frame->orig_rax;

	// Get the stack to use and copy the frame data to it.
	uint8* userStack = get_signal_stack(thread, frame, action,
		sizeof(*signalFrameData) + sizeof(frame->ip));

	signal_frame_data* userSignalFrameData
		= (signal_frame_data*)(userStack + sizeof(frame->ip));

	if (user_memcpy(userSignalFrameData, signalFrameData,
			sizeof(*signalFrameData)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	// Copy a return address to the stack so that backtraces will be correct.
	if (user_memcpy(userStack, &frame->ip, sizeof(frame->ip)) != B_OK)
		return B_BAD_ADDRESS;

	// Update Thread::user_signal_context, now that everything seems to have
	// gone fine.
	thread->user_signal_context = &userSignalFrameData->context;

	// Set up the iframe to execute the signal handler wrapper on our prepared
	// stack. First argument points to the frame data.
	addr_t* commPageAddress = (addr_t*)thread->team->commpage_address;
	frame->user_sp = (addr_t)userStack;
	set_ac();
	frame->ip = commPageAddress[COMMPAGE_ENTRY_X86_SIGNAL_HANDLER]
		+ (addr_t)commPageAddress;
	clear_ac();
	frame->di = (addr_t)userSignalFrameData;

	return B_OK;
}


int64
arch_restore_signal_frame(struct signal_frame_data* signalFrameData)
{
	iframe* frame = x86_get_current_iframe();

	frame->orig_rax = signalFrameData->syscall_restart_return_value;
	frame->ax = signalFrameData->context.uc_mcontext.rax;
	frame->bx = signalFrameData->context.uc_mcontext.rbx;
	frame->cx = signalFrameData->context.uc_mcontext.rcx;
	frame->dx = signalFrameData->context.uc_mcontext.rdx;
	frame->di = signalFrameData->context.uc_mcontext.rdi;
	frame->si = signalFrameData->context.uc_mcontext.rsi;
	frame->bp = signalFrameData->context.uc_mcontext.rbp;
	frame->r8 = signalFrameData->context.uc_mcontext.r8;
	frame->r9 = signalFrameData->context.uc_mcontext.r9;
	frame->r10 = signalFrameData->context.uc_mcontext.r10;
	frame->r11 = signalFrameData->context.uc_mcontext.r11;
	frame->r12 = signalFrameData->context.uc_mcontext.r12;
	frame->r13 = signalFrameData->context.uc_mcontext.r13;
	frame->r14 = signalFrameData->context.uc_mcontext.r14;
	frame->r15 = signalFrameData->context.uc_mcontext.r15;
	frame->user_sp = signalFrameData->context.uc_mcontext.rsp;
	frame->ip = signalFrameData->context.uc_mcontext.rip;
	frame->flags = (frame->flags & ~(uint64)X86_EFLAGS_USER_FLAGS)
		| (signalFrameData->context.uc_mcontext.rflags & X86_EFLAGS_USER_FLAGS);

	Thread* thread = thread_get_current_thread();

	memcpy(thread->arch_info.fpu_state,
		(void*)&signalFrameData->context.uc_mcontext.fpu,
		sizeof(thread->arch_info.fpu_state));
	frame->fpu = &thread->arch_info.fpu_state;

	// The syscall return code overwrites frame->ax with the return value of
	// the syscall, need to return it here to ensure the correct value is
	// restored.
	return frame->ax;
}


#ifdef _COMPAT_MODE

status_t
arch_setup_compat_signal_frame(Thread* thread, struct sigaction* action,
	struct compat_signal_frame_data* signalFrameData)
{
	iframe* frame = x86_get_current_iframe();
	if (!IFRAME_IS_USER(frame)) {
		panic("arch_setup_compat_signal_frame(): No user iframe!");
		return B_BAD_VALUE;
	}

	// Store the register state.
	signalFrameData->context.uc_mcontext.eax = frame->ax;
	signalFrameData->context.uc_mcontext.ebx = frame->bx;
	signalFrameData->context.uc_mcontext.ecx = frame->cx;
	signalFrameData->context.uc_mcontext.edx = frame->dx;
	signalFrameData->context.uc_mcontext.edi = frame->di;
	signalFrameData->context.uc_mcontext.esi = frame->si;
	signalFrameData->context.uc_mcontext.ebp = frame->bp;
	signalFrameData->context.uc_mcontext.esp = frame->user_sp;
	signalFrameData->context.uc_mcontext.eip = frame->ip;
	signalFrameData->context.uc_mcontext.eflags = frame->flags;
	x86_fnsave((void *)(&signalFrameData->context.uc_mcontext.xregs));

	// Fill in signalFrameData->context.uc_stack.
	stack_t stack;
	signal_get_user_stack(frame->user_sp, &stack);
	signalFrameData->context.uc_stack.ss_sp = (addr_t)stack.ss_sp;
	signalFrameData->context.uc_stack.ss_size = stack.ss_size;
	signalFrameData->context.uc_stack.ss_flags = stack.ss_flags;

	// Store syscall_restart_return_value.
	signalFrameData->syscall_restart_return_value = frame->orig_rax;

	// Get the stack to use and copy the frame data to it.
	uint32 stackFrame[2];
	uint8* userStack = get_signal_stack(thread, frame, action,
		sizeof(*signalFrameData) + sizeof(stackFrame));

	compat_signal_frame_data* userSignalFrameData
		= (compat_signal_frame_data*)(userStack + sizeof(stackFrame));

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
	thread->user_signal_context = (ucontext_t*)&userSignalFrameData->context;

	// Set up the iframe to execute the signal handler wrapper on our prepared
	// stack. First argument points to the frame data.
	addr_t* commPageAddress = (addr_t*)thread->team->commpage_address;
	frame->user_sp = (addr_t)userStack;
	set_ac();

	// from x86/arch_commpage_defs.h
#define COMPAT_COMMPAGE_ENTRY_X86_SIGNAL_HANDLER \
	(COMMPAGE_ENTRY_FIRST_ARCH_SPECIFIC + 3)
	frame->ip = commPageAddress[COMPAT_COMMPAGE_ENTRY_X86_SIGNAL_HANDLER]
		+ (addr_t)commPageAddress;
	clear_ac();

	return B_OK;
}


int64
arch_restore_compat_signal_frame(struct compat_signal_frame_data* signalFrameData)
{
	iframe* frame = x86_get_current_iframe();

	frame->orig_rax = signalFrameData->syscall_restart_return_value;
	frame->ax = signalFrameData->context.uc_mcontext.eax;
	frame->bx = signalFrameData->context.uc_mcontext.ebx;
	frame->cx = signalFrameData->context.uc_mcontext.ecx;
	frame->dx = signalFrameData->context.uc_mcontext.edx;
	frame->di = signalFrameData->context.uc_mcontext.edi;
	frame->si = signalFrameData->context.uc_mcontext.esi;
	frame->bp = signalFrameData->context.uc_mcontext.ebp;
	frame->user_sp = signalFrameData->context.uc_mcontext.esp;
	frame->ip = signalFrameData->context.uc_mcontext.eip;
	frame->flags = (frame->flags & ~(uint64)X86_EFLAGS_USER_FLAGS)
		| (signalFrameData->context.uc_mcontext.eflags & X86_EFLAGS_USER_FLAGS);

	x86_frstor((void*)(&signalFrameData->context.uc_mcontext.xregs));

	// The syscall return code overwrites frame->ax with the return value of
	// the syscall, need to return it here to ensure the correct value is
	// restored.
	return frame->ax;
}


void
arch_syscall_64_bit_return_value(void)
{
	Thread* thread = thread_get_current_thread();
	if ((thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0)
		atomic_or(&thread->flags, THREAD_FLAGS_64_BIT_SYSCALL_RETURN);
}

#endif // _COMPAT_MODE

