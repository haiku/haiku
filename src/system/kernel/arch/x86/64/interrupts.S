/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>

#include <thread_types.h>

#include <arch/x86/descriptors.h>
#include <arch/x86/arch_cpu.h>
#include <arch/x86/arch_kernel.h>

#include "asm_offsets.h"
#include "syscall_numbers.h"
#include "syscall_table.h"


// Push the remainder of the interrupt frame onto the stack.
#define PUSH_IFRAME_BOTTOM(iframeType)	\
	push	%rax;						\
	push	%rbx;						\
	push	%rcx;						\
	push	%rdx;						\
	push	%rdi;						\
	push	%rsi;						\
	push	%rbp;						\
	push	%r8;						\
	push	%r9;						\
	push	%r10;						\
	push	%r11;						\
	push	%r12;						\
	push	%r13;						\
	push	%r14;						\
	push	%r15;						\
	push	$iframeType;

// Restore the interrupt frame.
#define RESTORE_IFRAME()				\
	add		$8, %rsp;					\
	pop		%r15;						\
	pop		%r14;						\
	pop		%r13;						\
	pop		%r12;						\
	pop		%r11;						\
	pop		%r10;						\
	pop		%r9;						\
	pop		%r8;						\
	pop		%rbp;						\
	pop		%rsi;						\
	pop		%rdi;						\
	pop		%rdx;						\
	pop		%rcx;						\
	pop		%rbx;						\
	pop		%rax;						\
	addq	$16, %rsp;


// The macros below require R12 to contain the current thread pointer. R12 is
// callee-save so will be preserved through all function calls and only needs
// to be obtained once. R13 is used to store the system call start time, will
// also be preserved.

#define LOCK_THREAD_TIME()										\
	leaq	THREAD_time_lock(%r12), %rdi;						\
	call	acquire_spinlock;

#define UNLOCK_THREAD_TIME()									\
	leaq	THREAD_time_lock(%r12), %rdi;						\
	call	release_spinlock;									\

#define UPDATE_THREAD_USER_TIME()								\
	LOCK_THREAD_TIME()											\
																\
	call	system_time;										\
																\
	/* Preserve system_time for post syscall debug */			\
	movq	%rax, %r13;											\
																\
	/* thread->user_time += now - thread->last_time; */			\
	subq	THREAD_last_time(%r12), %rax;						\
	addq	%rax, THREAD_user_time(%r12);						\
																\
	/* thread->last_time = now; */								\
	movq	%r13, THREAD_last_time(%r12);						\
																\
	/* thread->in_kernel = true; */								\
	movb	$1, THREAD_in_kernel(%r12);							\
																\
	UNLOCK_THREAD_TIME()

#define UPDATE_THREAD_KERNEL_TIME()								\
	LOCK_THREAD_TIME()											\
																\
	call	system_time;										\
	movq	%rax, %r13;											\
																\
	/* thread->kernel_time += now - thread->last_time; */		\
	subq	THREAD_last_time(%r12), %rax;						\
	addq	%rax, THREAD_kernel_time(%r12);						\
																\
	/* thread->last_time = now; */								\
	movq	%r13, THREAD_last_time(%r12);						\
																\
	/* thread->in_kernel = false; */							\
	movb	$0, THREAD_in_kernel(%r12);							\
																\
	UNLOCK_THREAD_TIME()

// TODO
#if 0
#define STOP_USER_DEBUGGING()									\
	testl	$(THREAD_FLAGS_BREAKPOINTS_INSTALLED				\
			| THREAD_FLAGS_SINGLE_STEP), THREAD_flags(%r12);	\
	jz		1f;													\
	call	x86_exit_user_debug_at_kernel_entry;				\
  1:
#else
#define STOP_USER_DEBUGGING()	
#endif


// The following code defines the interrupt service routines for all 256
// interrupts. It creates a block of handlers, each 16 bytes, that the IDT
// initialization code just loops through.

// Interrupt with no error code, pushes a 0 error code.
#define DEFINE_ISR(nr)					\
	.align 16;							\
	push	$0;							\
	push	$nr;						\
	jmp		int_bottom;

// Interrupt with an error code.
#define DEFINE_ISR_E(nr)				\
	.align 16;							\
	push	$nr;						\
	jmp		int_bottom;

// Array of interrupt service routines.
.align 16
SYMBOL(isr_array):
	// Exceptions (0-19) and reserved interrupts (20-31).
	DEFINE_ISR(0)
	DEFINE_ISR(1)
	DEFINE_ISR(2)
	DEFINE_ISR(3)
	DEFINE_ISR(4)
	DEFINE_ISR(5)
	DEFINE_ISR(6)
	DEFINE_ISR(7)
	DEFINE_ISR_E(8)
	DEFINE_ISR(9)
	DEFINE_ISR_E(10)
	DEFINE_ISR_E(11)
	DEFINE_ISR_E(12)
	DEFINE_ISR_E(13)
	DEFINE_ISR_E(14)
	DEFINE_ISR(15)
	DEFINE_ISR(16)
	DEFINE_ISR_E(17)
	DEFINE_ISR(18)
	DEFINE_ISR(19)
	DEFINE_ISR(20)
	DEFINE_ISR(21)
	DEFINE_ISR(22)
	DEFINE_ISR(23)
	DEFINE_ISR(24)
	DEFINE_ISR(25)
	DEFINE_ISR(26)
	DEFINE_ISR(27)
	DEFINE_ISR(28)
	DEFINE_ISR(29)
	DEFINE_ISR(30)
	DEFINE_ISR(31)

	// User-defined ISRs (32-255) - none take an error code.
	.Lintr = 32
	.rept 224
		DEFINE_ISR(.Lintr)
		.Lintr = .Lintr+1
	.endr


// Common interrupt handling code.
STATIC_FUNCTION(int_bottom):
	// Coming from user-mode requires special handling.
	testl	$3, 24(%rsp)
	jnz		int_bottom_user

	// Push the rest of the interrupt frame to the stack.
	PUSH_IFRAME_BOTTOM(IFRAME_TYPE_OTHER)

	cld

	// Frame pointer is the iframe.
	movq	%rsp, %rbp

	// Set the RF (resume flag) in RFLAGS. This prevents an instruction
	// breakpoint on the instruction we're returning to to trigger a debug
	// exception.
	orq		$X86_EFLAGS_RESUME, IFRAME_flags(%rbp)

	// Call the interrupt handler.
	movq	%rsp, %rdi
	movq	IFRAME_vector(%rsp), %rax
	call	*gInterruptHandlerTable(, %rax, 8)

	// Restore the saved registers.
	RESTORE_IFRAME()

	iretq
FUNCTION_END(int_bottom)


// Handler for an interrupt that occurred in user-mode.
STATIC_FUNCTION(int_bottom_user):
	// Load the kerrnel GS segment base.
	swapgs

	// Push the rest of the interrupt frame to the stack.
	PUSH_IFRAME_BOTTOM(IFRAME_TYPE_OTHER)

	cld

	// Frame pointer is the iframe.
	movq	%rsp, %rbp

	// Set the RF (resume flag) in RFLAGS. This prevents an instruction
	// breakpoint on the instruction we're returning to to trigger a debug
	// exception.
	orq		$X86_EFLAGS_RESUME, IFRAME_flags(%rbp)

	// Get thread pointer.
	movq	%gs:0, %r12

	STOP_USER_DEBUGGING()
	UPDATE_THREAD_USER_TIME()

	// Call the interrupt handler.
	movq	%rsp, %rdi
	movq	IFRAME_vector(%rsp), %rax
	call	*gInterruptHandlerTable(, %rax, 8)

	// If there are no signals pending or we're not debugging, we can avoid
	// most of the work here, just need to update the kernel time.
	testl	$(THREAD_FLAGS_DEBUGGER_INSTALLED | THREAD_FLAGS_SIGNALS_PENDING \
			| THREAD_FLAGS_DEBUG_THREAD | THREAD_FLAGS_BREAKPOINTS_DEFINED) \
			, THREAD_flags(%r12)
	jnz		.Lkernel_exit_work

	cli

	UPDATE_THREAD_KERNEL_TIME()

	// Restore the saved registers.
	RESTORE_IFRAME()

	// Restore the previous GS base and return.
	swapgs
	iretq

.Lkernel_exit_work:
	// Slow path for return to userland.

	// Do we need to handle signals?
	testl	$(THREAD_FLAGS_SIGNALS_PENDING | THREAD_FLAGS_DEBUG_THREAD) \
			, THREAD_flags(%r12)
	jnz		.Lkernel_exit_handle_signals
	cli
	call	thread_at_kernel_exit_no_signals

.Lkernel_exit_work_done:
	// Install breakpoints, if defined. TODO
	//testl	$THREAD_FLAGS_BREAKPOINTS_DEFINED, THREAD_flags(%r12)
	//jz	1f
	//movq	%rbp, %rdi
	//call	x86_init_user_debug_at_kernel_exit
//1:
	// Restore the saved registers.
	RESTORE_IFRAME()

	// Restore the previous GS base and return.
	swapgs
	iretq

.Lkernel_exit_handle_signals:
	// thread_at_kernel_exit requires interrupts to be enabled, it will disable
	// them after.
	sti
	call	thread_at_kernel_exit
	jmp		.Lkernel_exit_work_done
FUNCTION_END(int_bottom_user)


// SYSCALL entry point.
FUNCTION(x86_64_syscall_entry):
	// Upon entry, RSP still points at the user stack.  Load the kernel GS
	// segment base address, which points at the current thread's arch_thread
	// structure. This contains our kernel stack pointer and a temporary
	// scratch space to store the user stack pointer in before we can push it
	// to the stack.
	swapgs
	movq	%rsp, %gs:ARCH_THREAD_user_rsp
	movq	%gs:ARCH_THREAD_syscall_rsp, %rsp

	// Set up an iframe on the stack (R11 = saved RFLAGS, RCX = saved RIP).
	push	$USER_DATA_SEG				// ss
	push	%gs:ARCH_THREAD_user_rsp	// rsp
	push	%r11						// flags
	push	$USER_CODE_SEG				// cs
	push	%rcx						// ip
	push	$0							// error_code
	push	$0							// vector
	PUSH_IFRAME_BOTTOM(IFRAME_TYPE_SYSCALL)

	cld

	// Frame pointer is the iframe.
	movq	%rsp, %rbp

	// Preserve call number (R14 is callee-save), get thread pointer.
	movq	%rax, %r14
	movq	%gs:0, %r12

	STOP_USER_DEBUGGING()
	UPDATE_THREAD_USER_TIME()

	// No longer need interrupts disabled.
	sti

	// Check whether the syscall number is valid.
	cmpq	$SYSCALL_COUNT, %r14
	jae		.Lbad_syscall_number

	// Get the system call table entry. Note I'm hardcoding the shift because
	// sizeof(syscall_info) is 16 and scale factors of 16 aren't supported,
	// so can't just do leaq kSyscallInfos(, %rax, SYSCALL_INFO_sizeof).
	movq	%r14, %rax
	shlq	$4, %rax
	leaq	kSyscallInfos(, %rax, 1), %rax

	// Check the number of call arguments, greater than 6 (6 * 8 = 48) requires
	// a stack copy.
	movq	SYSCALL_INFO_parameter_size(%rax), %rcx
	cmpq	$48, %rcx
	ja		.Lsyscall_stack_args

	// Restore the arguments from the iframe. UPDATE_THREAD_USER_TIME() makes
	// 2 function calls which means they may have been overwritten. Note that
	// argument 4 is in R10 on the frame rather than RCX as RCX is used by
	// SYSCALL.
	movq	IFRAME_di(%rbp), %rdi
	movq	IFRAME_si(%rbp), %rsi
	movq	IFRAME_dx(%rbp), %rdx
	movq	IFRAME_r10(%rbp), %rcx
	movq	IFRAME_r8(%rbp), %r8
	movq	IFRAME_r9(%rbp), %r9

	// TODO: pre-syscall tracing/debug

	// Call the function and save its return value.
	call	*SYSCALL_INFO_function(%rax)
	movq	%rax, IFRAME_ax(%rbp)

	// TODO: post-syscall tracing

.Lbad_syscall_number:
.Lsyscall_return:
	// Clear the restarted flag.
	testl	$THREAD_FLAGS_SYSCALL_RESTARTED, THREAD_flags(%r12)
	jz		2f
1:
    movl	THREAD_flags(%r12), %eax
	movl	%eax, %edx
    andl	$~THREAD_FLAGS_SYSCALL_RESTARTED, %edx
    lock
    cmpxchgl	%edx, THREAD_flags(%r12)
    jnz		1b
2:
	testl	$(THREAD_FLAGS_DEBUGGER_INSTALLED | THREAD_FLAGS_SIGNALS_PENDING \
			| THREAD_FLAGS_DEBUG_THREAD | THREAD_FLAGS_BREAKPOINTS_DEFINED \
			| THREAD_FLAGS_RESTART_SYSCALL) \
			, THREAD_flags(%r12)
	jnz		.Lpost_syscall_work

	cli

	UPDATE_THREAD_KERNEL_TIME()

	// If we've just restored a signal frame, use the IRET path.
	cmpq	$SYSCALL_RESTORE_SIGNAL_FRAME, %r14
	je		.Liret

	// If the return address is not canonical, return using the IRET path.
	// On Intel's implementation of SYSRET, the canonical address check for the
	// return address is performed before the switch to user mode, so a fault
	// for a non-canonical address will be triggered in kernel mode. However, by
	// then we will have switched onto the user stack meaning the kernel could
	// be tricked into executing with the user RSP. Using IRET will handle
	// non-canonical addresses properly.
	movq	IFRAME_user_sp(%rsp), %rax
	sarq	$47, %rax
	incl	%eax
	cmpl	$1, %eax
	ja		.Liret

	// Restore the iframe and RCX/R11 for SYSRET.
	RESTORE_IFRAME()
	pop		%rcx
	addq	$8, %rsp
	pop		%r11
	pop		%rsp

	// Restore previous GS base and return.
	swapgs
	sysretq

.Lpost_syscall_work:
	// TODO: post-syscall debugging

	// Do we need to handle signals?
	testl	$(THREAD_FLAGS_SIGNALS_PENDING | THREAD_FLAGS_DEBUG_THREAD) \
			, THREAD_flags(%r12)
	jnz		.Lpost_syscall_handle_signals
	cli
	call	thread_at_kernel_exit_no_signals

.Lpost_syscall_work_done:
	// Handle syscall restarting.
	testl	$THREAD_FLAGS_RESTART_SYSCALL, THREAD_flags(%r12)
	jz		1f
	movq	%rsp, %rdi
	call	x86_restart_syscall
1:
	// Install breakpoints, if defined. TODO
	//testl	$THREAD_FLAGS_BREAKPOINTS_DEFINED, THREAD_flags(%r12)
	//jz	.Liret
	//movq	%rbp, %rdi
	//call	x86_init_user_debug_at_kernel_exit

	// On this return path it is possible that the frame has been modified,
	// for example to execute a signal handler. In this case it is safer to
	// return via IRET.

.Liret:
	// Restore the saved registers.
	RESTORE_IFRAME()

	// Restore the previous GS base and return.
	swapgs
	iretq

.Lpost_syscall_handle_signals:
	call	thread_at_kernel_exit
	jmp		.Lpost_syscall_work_done

.Lsyscall_stack_args:
	// Some arguments are on the stack, work out what we need to copy. 6
	// arguments (48 bytes) are already in registers.
	// RAX = syscall table entry address, RCX = argument size.
	subq	$48, %rcx

	// Get the address to copy from.
	movq	IFRAME_user_sp(%rbp), %rsi
	addq	$8, %rsi
	movabs	$(USER_BASE + USER_SIZE), %rdx
	cmp		%rdx, %rsi
	jae		.Lbad_syscall_args

	// Make space on the stack.
	subq	%rcx, %rsp
	movq	%rsp, %rdi

	// Set a fault handler.
	movq	$.Lbad_syscall_args, THREAD_fault_handler(%r12)

	// Copy them by quadwords.
	shrq	$3, %rcx
	rep
	movsq
	movq	$0, THREAD_fault_handler(%r12)

	// Restore the register arguments from the iframe.
	movq	IFRAME_di(%rbp), %rdi
	movq	IFRAME_si(%rbp), %rsi
	movq	IFRAME_dx(%rbp), %rdx
	movq	IFRAME_r10(%rbp), %rcx
	movq	IFRAME_r8(%rbp), %r8
	movq	IFRAME_r9(%rbp), %r9

	// TODO: pre-syscall tracing/debug

	// Call the function and save its return value.
	call	*SYSCALL_INFO_function(%rax)
	movq	%rax, IFRAME_ax(%rbp)

	// TODO: post-syscall tracing

	// Restore the original stack pointer and return.
	movq	%rbp, %rsp
	jmp		.Lsyscall_return

.Lbad_syscall_args:
	movq	$0, THREAD_fault_handler(%r12)
	movq	%rbp, %rsp
	jmp		.Lsyscall_return
FUNCTION_END(x86_64_syscall_entry)


/*!	\fn void x86_return_to_userland(iframe* frame)
	\brief Returns to the userland environment given by \a frame.

	Before returning to userland all potentially necessary kernel exit work is
	done.

	\a frame must point to a location somewhere on the caller's stack (e.g. a
	local variable).
	The function must be called with interrupts disabled.

	\param frame The iframe defining the userland environment.
*/
FUNCTION(x86_return_to_userland):
	movq	%rdi, %rbp
	movq	%rbp, %rsp

	// Perform kernel exit work.
	movq	%gs:0, %r12
	testl	$(THREAD_FLAGS_DEBUGGER_INSTALLED | THREAD_FLAGS_SIGNALS_PENDING \
			| THREAD_FLAGS_DEBUG_THREAD | THREAD_FLAGS_BREAKPOINTS_DEFINED) \
			, THREAD_flags(%r12)
	jnz		.Lkernel_exit_work

	// update the thread's kernel time and return
	UPDATE_THREAD_KERNEL_TIME()

	// Restore the frame and return.
	RESTORE_IFRAME()
	swapgs
	iretq
FUNCTION_END(x86_return_to_userland)
