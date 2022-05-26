/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>

#include <thread_types.h>

#include <arch/x86/descriptors.h>
#include <arch/x86/arch_altcodepatch.h>
#include <arch/x86/arch_cpu.h>
#include <arch/x86/arch_kernel.h>
#define COMMPAGE_COMPAT
#include <commpage_defs.h>

#include "asm_offsets.h"
#include "syscall_numbers.h"
#include "syscall_table.h"


// Push the remainder of the interrupt frame onto the stack.
#define PUSH_IFRAME_BOTTOM(iframeType)	\
	push	%rax;	/* orig_rax */		\
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
	pushq	$0;							\
	push	$iframeType;


// Restore the interrupt frame.
#define RESTORE_IFRAME()				\
	add		$16, %rsp;					\
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
	addq	$24, %rsp;


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

#define STOP_USER_DEBUGGING()									\
	testl	$(THREAD_FLAGS_BREAKPOINTS_INSTALLED				\
			| THREAD_FLAGS_SINGLE_STEP), THREAD_flags(%r12);	\
	jz		1f;													\
	call	x86_exit_user_debug_at_kernel_entry;				\
  1:

#define CLEAR_FPU_STATE() \
	pxor %xmm0, %xmm0; \
	pxor %xmm1, %xmm1; \
	pxor %xmm2, %xmm2; \
	pxor %xmm3, %xmm3; \
	pxor %xmm4, %xmm4; \
	pxor %xmm5, %xmm5; \
	pxor %xmm6, %xmm6; \
	pxor %xmm7, %xmm7; \
	pxor %xmm8, %xmm8; \
	pxor %xmm9, %xmm9; \
	pxor %xmm10, %xmm10; \
	pxor %xmm11, %xmm11; \
	pxor %xmm12, %xmm12; \
	pxor %xmm13, %xmm13; \
	pxor %xmm14, %xmm14; \
	pxor %xmm15, %xmm15


// SYSCALL entry point.
FUNCTION(x86_64_syscall32_entry):
	// TODO: implement for AMD SYSCALL
	sysret
FUNCTION_END(x86_64_syscall32_entry)


// SYSENTER entry point.
//	ecx - user esp
FUNCTION(x86_64_sysenter32_entry):
	swapgs

	// Set up an iframe on the stack (ECX = saved ESP).
	push	$USER_DATA_SELECTOR			// ss
	// zero extend %ecx
	movl	%ecx, %ecx
	push	%rcx						// rsp
	pushfq								// flags
	orl		$(1 << 9), (%rsp)		// set the IF (interrupts) bit
	push	$USER32_CODE_SELECTOR		// cs

	movq	%gs:0, %rdx
	movq	THREAD_team(%rdx), %rdx
	movq	TEAM_commpage_address(%rdx), %rdx
	ASM_STAC
	add		4 * COMMPAGE_ENTRY_X86_SYSCALL(%rdx), %rdx
	ASM_CLAC
	add		$4, %rdx				// sysenter is at offset 2, 2 bytes long
	push	%rdx						// ip

	push	$0							// error_code
	push	$99							// vector
	PUSH_IFRAME_BOTTOM(IFRAME_TYPE_SYSCALL)

	cld

	// Frame pointer is the iframe.
	movq	%rsp, %rbp
	andq	$~15, %rsp

	// Preserve call number (R14 is callee-save), get thread pointer.
	movq	%rax, %r14
	movq	%gs:0, %r12

	STOP_USER_DEBUGGING()
	UPDATE_THREAD_USER_TIME()

	// No longer need interrupts disabled.
	sti

	// Check whether the syscall number is valid.
	cmpq	$SYSCALL_COUNT, %r14
	jae		.Lsyscall_return

	// Get the system call table entry. Note I'm hardcoding the shift because
	// sizeof(syscall_info) is 16 and scale factors of 16 aren't supported,
	// so can't just do leaq kSyscallInfos(, %rax, SYSCALL_INFO_sizeof).
	movq	%r14, %rax
	shlq	$4, %rax
	leaq	kSyscallCompatInfos(, %rax, 1), %rax

	// Restore the arguments from the stack.
	movq	SYSCALL_INFO_parameter_size(%rax), %rcx

	// Get the address to copy from.
	movq	IFRAME_user_sp(%rbp), %rsi
	addq	$4, %rsi
	movabs	$(USER_BASE + USER_SIZE), %rdx
	cmp		%rdx, %rsi
	jae		.Lbad_syscall_args

	// Make space on the stack for the double size.
	shlq	$1, %rcx
	cmpq	$48, %rcx
	ja		.Lprepare_stack
	movq	$48, %rcx
.Lprepare_stack:
	subq	%rcx, %rsp
	andq	$~15, %rsp
	movq	%rsp, %rdi

	// Get the extended system call table entry.
	movq	%r14, %r15
	imulq	$ EXTENDED_SYSCALL_INFO_sizeof, %r15
	leaq	kExtendedSyscallCompatInfos(, %r15, 1), %r15
	xor		%rcx, %rcx
	movl	EXTENDED_SYSCALL_INFO_parameter_count(%r15), %ecx
	leaq	EXTENDED_SYSCALL_INFO_parameters(%r15), %r15

	// Set a fault handler.
	movq	$.Lbad_syscall_args, THREAD_fault_handler(%r12)

	ASM_STAC

	jmp 	2f
	// Copy them by doublewords.
1:
	// Advance to next parameter
	addq	$ SYSCALL_PARAMETER_INFO_sizeof, %r15
	subq	$1, %rcx
2:
	cmpq	$0, %rcx
	je		4f
	movsd
	cmpl	$0x8, SYSCALL_PARAMETER_INFO_used_size(%r15)
	je		3f
	movl	$0,	(%rdi)
	addq	$4, %rdi
	jmp		1b
3:
	// Copy the next doubleword
	movsd
	jmp		1b
4:
	ASM_CLAC
	movq	$0, THREAD_fault_handler(%r12)

.Lperform_syscall:
	testl	$THREAD_FLAGS_DEBUGGER_INSTALLED, THREAD_flags(%r12)
	jnz		.Lpre_syscall_debug

.Lpre_syscall_debug_done:
	// arguments on the stack, copy in the registers
	pop		%rdi
	pop		%rsi
	pop		%rdx
	pop		%rcx
	pop		%r8
	pop		%r9

	// TODO: pre-syscall tracing

	// Call the function and save its return value.
	call	*SYSCALL_INFO_function(%rax)
	movq	%rax, %rdx
	movq	%rax, IFRAME_ax(%rbp)
	shrq	$32, %rdx
	movq	%rdx, IFRAME_dx(%rbp)

	// TODO: post-syscall tracing

.Lsyscall_return:
	// Restore the original stack pointer and return.
	movq	%rbp, %rsp

	testl	$(THREAD_FLAGS_DEBUGGER_INSTALLED | THREAD_FLAGS_SIGNALS_PENDING \
			| THREAD_FLAGS_DEBUG_THREAD | THREAD_FLAGS_BREAKPOINTS_DEFINED \
			| THREAD_FLAGS_TRAP_FOR_CORE_DUMP \
			| THREAD_FLAGS_64_BIT_SYSCALL_RETURN \
			| THREAD_FLAGS_RESTART_SYSCALL | THREAD_FLAGS_SYSCALL_RESTARTED) \
			, THREAD_flags(%r12)
	jnz		.Lpost_syscall_work

	cli

	UPDATE_THREAD_KERNEL_TIME()

	// If we've just restored a signal frame, use the IRET path.
	cmpq	$SYSCALL_RESTORE_SIGNAL_FRAME, %r14
	je		.Lrestore_fpu

	CLEAR_FPU_STATE()

	// Restore the iframe and RCX/RDX for SYSRET.
	RESTORE_IFRAME()
	pop		%rdx
	addq	$8, %rsp
	andl  $~0x200,(%rsp)
	popfq
	pop		%rcx

	// Restore previous GS base and return.
	swapgs
	sti
	sysexit


.Lpre_syscall_debug:
	// preserve registers
	push	%rdi
	push	%rsi

	// user_debug_pre_syscall expects a pointer to a block of arguments, need
	// to push the register arguments onto the stack.
	movq	%r14, %rdi				// syscall number
	movq	0x10(%rsp), %rsi
	push	%rax
	call	user_debug_pre_syscall
	pop		%rax

	// restore registers
	pop		%rsi
	pop		%rdi
	jmp		.Lpre_syscall_debug_done

.Lpost_syscall_work:
	// Clear the restarted flag.
	testl	$(THREAD_FLAGS_64_BIT_SYSCALL_RETURN \
				| THREAD_FLAGS_SYSCALL_RESTARTED), THREAD_flags(%r12)
	jz		2f
1:
	movl	THREAD_flags(%r12), %eax
	movl	%eax, %edx
	andl	$~(THREAD_FLAGS_64_BIT_SYSCALL_RETURN \
				| THREAD_FLAGS_SYSCALL_RESTARTED), %edx
	lock
	cmpxchgl	%edx, THREAD_flags(%r12)
	jnz		1b
2:
	testl	$THREAD_FLAGS_DEBUGGER_INSTALLED, THREAD_flags(%r12)
	jz		1f

	// Post-syscall debugging. Same as above, need a block of arguments.
	// TODO: restore arguments from the stack
	push	IFRAME_r9(%rbp)
	push	IFRAME_r8(%rbp)
	push	IFRAME_r10(%rbp)
	push	IFRAME_dx(%rbp)
	push	IFRAME_si(%rbp)
	push	IFRAME_di(%rbp)
	movq	%r14, %rdi				// syscall number
	movq	%rsp, %rsi
	movq	IFRAME_ax(%rbp), %rdx	// return value
	movq	%r13, %rcx				// start time, preserved earlier
	call	user_debug_post_syscall
	addq	$48, %rsp
1:
	// Do we need to handle signals?
	testl	$(THREAD_FLAGS_SIGNALS_PENDING | THREAD_FLAGS_DEBUG_THREAD \
			| THREAD_FLAGS_TRAP_FOR_CORE_DUMP) \
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
	// Install breakpoints, if defined.
	testl	$THREAD_FLAGS_BREAKPOINTS_DEFINED, THREAD_flags(%r12)
	jz		1f
	movq	%rbp, %rdi
	call	x86_init_user_debug_at_kernel_exit
1:
	// On this return path it is possible that the frame has been modified,
	// for example to execute a signal handler. In this case it is safer to
	// return via IRET.
	CLEAR_FPU_STATE()
	jmp .Liret

.Lrestore_fpu:
	movq	IFRAME_fpu(%rbp), %rax
	fxrstorq	(%rax)
.Liret:
	// Restore the saved registers.
	RESTORE_IFRAME()

	// Restore the previous GS base and return.
	swapgs
	iretq

.Lpost_syscall_handle_signals:
	call	thread_at_kernel_exit
	jmp		.Lpost_syscall_work_done

.Lbad_syscall_args:
	movq	$0, THREAD_fault_handler(%r12)
	movq	%rbp, %rsp
	jmp		.Lsyscall_return
FUNCTION_END(x86_64_sysenter32_entry)


/* thread exit stub */
// TODO: build with the x86 compiler
FUNCTION(x86_sysenter32_userspace_thread_exit):
	.byte	0x50		// push %eax
	mov		$SYSCALL_EXIT_THREAD, %eax
	.byte	0x89,0xe1	// mov %esp, %ecx
	sysenter
FUNCTION_END(x86_sysenter32_userspace_thread_exit)
SYMBOL(x86_sysenter32_userspace_thread_exit_end):

