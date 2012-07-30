/*
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
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#include "paging/X86PagingStructures.h"
#include "paging/X86VMTranslationMap.h"


//#define TRACE_ARCH_THREAD
#ifdef TRACE_ARCH_THREAD
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


extern "C" void x86_64_thread_entry();

// Initial thread saved state.
static arch_thread sInitialState;


void
x86_restart_syscall(iframe* frame)
{
	panic("x86_restart_syscall: TODO");
}


void
x86_set_tls_context(Thread* thread)
{
	// Set FS segment base address to the TLS segment.
	x86_write_msr(IA32_MSR_FS_BASE, thread->user_local_storage);
}


static uint8*
get_signal_stack(Thread* thread, iframe* frame, struct sigaction* action)
{
	// Use the alternate signal stack if we should and can.
	if (thread->signal_stack_enabled
			&& (action->sa_flags & SA_ONSTACK) != 0
			&& (frame->user_sp < thread->signal_stack_base
				|| frame->user_sp >= thread->signal_stack_base
					+ thread->signal_stack_size)) {
		return (uint8*)(thread->signal_stack_base + thread->signal_stack_size);
	}

	// We are going to use the stack that we are already on. We must not touch
	// the red zone (128 byte area below the stack pointer, reserved for use
	// by functions to store temporary data and guaranteed not to be modified
	// by signal handlers).
	return (uint8*)(frame->user_sp - 128);
}


//	#pragma mark -


status_t
arch_thread_init(kernel_args* args)
{
	// Save one global valid FPU state; it will be copied in the arch dependent
	// part of each new thread.
	asm volatile ("clts; fninit; fnclex;");
	x86_fxsave(sInitialState.fpu_state);

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
	addr_t* stackTop = (addr_t*)_stackTop;

	TRACE("arch_thread_init_kthread_stack: stack top %p, function %p, data: "
		"%p\n", _stackTop, function, data);

	// Save the stack top for system call entry.
	thread->arch_info.syscall_rsp = (uint64*)thread->kernel_stack_top;

	// x86_64 uses registers for argument passing, first argument in RDI,
	// however we don't save RDI on every context switch (there is no need
	// for us to: it is not callee-save, and only contains the first argument
	// to x86_context_switch). However, this presents a problem since we
	// cannot store the argument for the entry function here. Therefore, we
	// save the function address in R14 and the argument in R15 (which are
	// restored), and then set up the stack to initially call a wrapper
	// function which passes the argument correctly.

	*--stackTop = 0;							// Dummy return address.
	*--stackTop = (addr_t)x86_64_thread_entry;	// Wrapper function.
	*--stackTop = (addr_t)data;					// R15: argument.
	*--stackTop = (addr_t)function;				// R14: entry function.
	*--stackTop = 0;							// R13.
	*--stackTop = 0;							// R12.
	*--stackTop = 0;							// RBP.
	*--stackTop = 0;							// RBX.

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

	TRACE("arch_thread_enter_userspace: entry %#lx, args %p %p, "
		"stackTop %#lx\n", entry, args1, args2, stackTop);

	// Copy the little stub that calls exit_thread() when the thread entry
	// function returns.
	// TODO: This will become a problem later if we want to support execute
	// disable, the stack shouldn't really be executable.
	size_t codeSize = (addr_t)x86_end_userspace_thread_exit
		- (addr_t)x86_userspace_thread_exit;
	stackTop -= codeSize;
	if (user_memcpy((void*)stackTop, (const void*)&x86_userspace_thread_exit,
			codeSize) != B_OK)
		return B_BAD_ADDRESS;

	// Copy the address of the stub to the top of the stack to act as the
	// return address.
	addr_t codeAddr = stackTop;
	stackTop -= sizeof(codeAddr);
	if (user_memcpy((void*)stackTop, (const void*)&codeAddr, sizeof(codeAddr))
			!= B_OK)
		return B_BAD_ADDRESS;

	// Prepare the user iframe.
	iframe frame = {};
	frame.type = IFRAME_TYPE_SYSCALL;
	frame.si = (uint64)args2;
	frame.di = (uint64)args1;
	frame.ip = entry;
	frame.cs = USER_CODE_SEG;
	frame.flags = X86_EFLAGS_RESERVED1 | X86_EFLAGS_INTERRUPT
		| (3 << X86_EFLAGS_IO_PRIVILEG_LEVEL_SHIFT);
	frame.sp = stackTop;
	frame.ss = USER_DATA_SEG;

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

	// Store the FPU state. There appears to be a bug in GCC where the aligned
	// attribute on a structure is being ignored when the structure is allocated
	// on the stack, so even if the fpu_state struct has aligned(16) it may not
	// get aligned correctly. Instead, use the current thread's FPU save area
	// and then memcpy() to the frame structure.
	x86_fxsave(thread->arch_info.fpu_state);
	memcpy((void*)&signalFrameData->context.uc_mcontext.fpu,
		thread->arch_info.fpu_state,
		sizeof(signalFrameData->context.uc_mcontext.fpu));

	// Fill in signalFrameData->context.uc_stack.
	signal_get_user_stack(frame->user_sp, &signalFrameData->context.uc_stack);

	// Store syscall_restart_return_value (TODO).
	//signalFrameData->syscall_restart_return_value = frame->orig_rax;

	// Get the stack to use and copy the frame data to it.
	uint8* userStack = get_signal_stack(thread, frame, action);

	userStack -= sizeof(*signalFrameData);
	signal_frame_data* userSignalFrameData = (signal_frame_data*)userStack;

	if (user_memcpy(userSignalFrameData, signalFrameData,
			sizeof(*signalFrameData)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	// Copy a return address to the stack so that backtraces will be correct.
	userStack -= sizeof(frame->ip);
	if (user_memcpy(userStack, &frame->ip, sizeof(frame->ip)) != B_OK)
		return B_BAD_ADDRESS;

	// Update Thread::user_signal_context, now that everything seems to have
	// gone fine.
	thread->user_signal_context = &userSignalFrameData->context;

	// Set up the iframe to execute the signal handler wrapper on our prepared
	// stack. First argument points to the frame data.
	frame->user_sp = (addr_t)userStack;
	frame->ip = ((addr_t*)USER_COMMPAGE_ADDR)[COMMPAGE_ENTRY_X86_SIGNAL_HANDLER];
	frame->di = (addr_t)userSignalFrameData;

	return B_OK;
}


int64
arch_restore_signal_frame(struct signal_frame_data* signalFrameData)
{
	iframe* frame = x86_get_current_iframe();

	// TODO
	//frame->orig_rax = signalFrameData->syscall_restart_return_value;

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

	// Same as above, alignment may not be correct. Copy to thread and restore
	// from there.
	Thread* thread = thread_get_current_thread();
	memcpy(thread->arch_info.fpu_state,
		(void*)&signalFrameData->context.uc_mcontext.fpu,
		sizeof(thread->arch_info.fpu_state));
	x86_fxrstor(thread->arch_info.fpu_state);

	// The syscall return code overwrites frame->ax with the return value of
	// the syscall, need to return it here to ensure the correct value is
	// restored.
	return frame->ax;
}
