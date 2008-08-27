/*
 * Copyright 2002-2008, The Haiku Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Copyright 2002, Michael Noisternig. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <arch/user_debugger.h>
#include <arch/x86/arch_cpu.h>
#include <arch/x86/arch_kernel.h>
#include <arch/x86/descriptors.h>
#include <commpage_defs.h>
#include <thread_types.h>

#include "tracing_config.h"

#include "asm_offsets.h"
#include "syscall_numbers.h"
#include "syscall_table.h"


#define FUNCTION(x) .global x; .type x,@function; x

#define UPDATE_THREAD_USER_TIME_COMMON()					\
	movl	%eax, %ebx;				/* save for later */	\
	movl	%edx, %ecx;										\
															\
	/* thread->user_time += now - thread->last_time; */		\
	sub		THREAD_last_time(%edi), %eax;					\
	sbb		(THREAD_last_time + 4)(%edi), %edx;				\
	add		%eax, THREAD_user_time(%edi);					\
	adc		%edx, (THREAD_user_time + 4)(%edi);				\
															\
	/* thread->last_time = now; */							\
	movl	%ebx, THREAD_last_time(%edi);					\
	movl	%ecx, (THREAD_last_time + 4)(%edi);				\
															\
	/* thread->in_kernel = true; */							\
	movb	$1, THREAD_in_kernel(%edi)

#define UPDATE_THREAD_USER_TIME()							\
	call	system_time;									\
	UPDATE_THREAD_USER_TIME_COMMON()

#define UPDATE_THREAD_USER_TIME_PUSH_TIME()					\
	call	system_time;									\
	push	%edx;											\
	push	%eax;											\
	UPDATE_THREAD_USER_TIME_COMMON()

#define UPDATE_THREAD_KERNEL_TIME()							\
	call	system_time;									\
															\
	movl	%eax, %ebx;				/* save for later */	\
	movl	%edx, %ecx;										\
															\
	/* thread->kernel_time += now - thread->last_time; */	\
	sub		THREAD_last_time(%edi), %eax;					\
	sbb		(THREAD_last_time + 4)(%edi), %edx;				\
	add		%eax, THREAD_kernel_time(%edi);					\
	adc		%edx, (THREAD_kernel_time + 4)(%edi);			\
															\
	/* thread->last_time = now; */							\
	movl	%ebx, THREAD_last_time(%edi);					\
	movl	%ecx, (THREAD_last_time + 4)(%edi);				\
															\
	/* thread->in_kernel = false; */						\
	movb	$0, THREAD_in_kernel(%edi)

#define PUSH_IFRAME_BOTTOM(iframeType)	\
	pusha;								\
	push	%ds;						\
	push	%es;						\
	push	%fs;						\
	push	%gs;						\
	pushl	$iframeType

#define PUSH_IFRAME_BOTTOM_SYSCALL()	\
	pushl	$0;							\
	pushl	$99;						\
	pushl	%edx;						\
	pushl	%eax;						\
	PUSH_IFRAME_BOTTOM(IFRAME_TYPE_SYSCALL)

#define POP_IFRAME_AND_RETURN()										\
	/* skip iframe type */											\
	lea		4(%ebp), %esp;											\
																	\
	pop		%gs;													\
	addl	$4, %esp;	/* we skip %fs, as this contains the CPU	\
						   dependent TLS segment */					\
	pop		%es;													\
	pop		%ds;													\
																	\
	popa;															\
	addl	$16,%esp;	/* ignore the vector, error code, and		\
						   original eax/edx values */				\
	iret

#if KERNEL_BREAKPOINTS
#	define DISABLE_BREAKPOINTS()											\
		call	x86_exit_user_debug_at_kernel_entry;
#else
#	define DISABLE_BREAKPOINTS()											\
		testl	$THREAD_FLAGS_BREAKPOINTS_INSTALLED, THREAD_flags(%edi);	\
		jz		1f;															\
		call	x86_exit_user_debug_at_kernel_entry;						\
	  1:
#endif	// KERNEL_BREAKPOINTS

#define COPY_SYSCALL_PARAMETERS()										\
	/* make room for the syscall params */								\
	subl	$80, %esp;													\
																		\
	/* get the address of the syscall parameters */						\
	movl	IFRAME_user_esp(%ebp), %esi;								\
	addl	$4, %esi;													\
	cmp		$KERNEL_BASE, %esi;		/* must not be a kernel address */	\
	jae		bad_syscall_params;											\
																		\
	/* set the fault handler */											\
	movl	$bad_syscall_params, THREAD_fault_handler(%edi);			\
																		\
	/* target address is our stack */									\
	movl	%esp, %edi;													\
																		\
	/* number of syscall parameter words */								\
	movl	SYSCALL_INFO_parameter_size(%edx), %ecx;					\
	shrl	$2, %ecx;													\
																		\
	/* copy */															\
	cld;																\
	rep movsl;															\
																		\
	/* restore pointers and clear fault handler */						\
	movl	%edx, %esi;				/* syscall info pointer */			\
	movl	%dr3, %edi;				/* thread pointer */				\
	movl	$0, THREAD_fault_handler(%edi)

#if SYSCALL_TRACING
#	define TRACE_PRE_SYSCALL()	\
		movl	%esp, %eax;						/* syscall parameters */	\
		push	%eax;														\
		movl	IFRAME_orig_eax(%ebp), %eax;	/* syscall number */		\
		push	%eax;														\
		call	trace_pre_syscall;											\
		addl	$8, %esp;

#	define TRACE_POST_SYSCALL()	\
		testl	$THREAD_FLAGS_64_BIT_SYSCALL_RETURN, THREAD_flags(%edi);	\
		jnz		1f;															\
		xor		%edx, %edx;													\
1:																			\
		push	%edx;							/* syscall return value */	\
		push	%eax;														\
		movl	IFRAME_orig_eax(%ebp), %eax;	/* syscall number */		\
		push	%eax;														\
		call	trace_post_syscall;											\
		addl	$12, %esp
#else
#	define TRACE_PRE_SYSCALL()
#	define TRACE_POST_SYSCALL()
#endif


.text

#define TRAP_ERRC(name, vector) \
.globl name; \
.align 8; \
name: \
	pushl	$vector; \
	pushl	$-1; \
	pushl	$-1; \
	jmp		int_bottom

#define TRAP(name, vector) \
.globl name; \
.align 8; \
name: \
	pushl	$0; \
	pushl	$vector; \
	pushl	%edx; \
	pushl	%eax; \
	jmp		int_bottom

TRAP(trap0, 0)
TRAP(trap1, 1)
TRAP(trap2, 2)
TRAP(trap3, 3)
TRAP(trap4, 4)
TRAP(trap5, 5)
TRAP(trap6, 6)
TRAP(trap7, 7)

.globl double_fault;
.align 8;
double_fault:
	pushl	$-1;	// user-ss
	pushl	$-1;	// user-esp
	pushl	$-1;	// flags
	pushl	$KERNEL_CODE_SEG;	// cs
	pushl	$-1;	// eip
	pushl	$0;		// error-code
	pushl	$8;
	pushl	$-1;
	pushl	$-1;
	jmp		int_bottom

TRAP(trap9, 9)
TRAP_ERRC(trap10, 10)
TRAP_ERRC(trap11, 11)
TRAP_ERRC(trap12, 12)
TRAP_ERRC(trap13, 13)
TRAP_ERRC(trap14, 14)
/*TRAP(trap15, 15)*/
TRAP(trap16, 16)
TRAP_ERRC(trap17, 17)
TRAP(trap18, 18)
TRAP(trap19, 19)

TRAP(trap32, 32)
TRAP(trap33, 33)
TRAP(trap34, 34)
TRAP(trap35, 35)
TRAP(trap36, 36)
TRAP(trap37, 37)
TRAP(trap38, 38)
TRAP(trap39, 39)
TRAP(trap40, 40)
TRAP(trap41, 41)
TRAP(trap42, 42)
TRAP(trap43, 43)
TRAP(trap44, 44)
TRAP(trap45, 45)
TRAP(trap46, 46)
TRAP(trap47, 47)
TRAP(trap48, 48)
TRAP(trap49, 49)
TRAP(trap50, 50)
TRAP(trap51, 51)
TRAP(trap52, 52)
TRAP(trap53, 53)
TRAP(trap54, 54)
TRAP(trap55, 55)

TRAP(trap251, 251)
TRAP(trap252, 252)
TRAP(trap253, 253)
TRAP(trap254, 254)
TRAP(trap255, 255)


.align 16
.globl int_bottom
int_bottom:
	PUSH_IFRAME_BOTTOM(IFRAME_TYPE_OTHER)

	movl	%esp, %ebp		// frame pointer is the iframe

	testl	$0x20000, IFRAME_flags(%ebp)	// VM86 mode
	jnz		int_bottom_vm86
	cmp		$USER_CODE_SEG, IFRAME_cs(%ebp)	// user mode
	je		int_bottom_user

	// We need to recheck user mode using the thread's in_kernel flag, since
	// sysexit introduces a raced condition: It doesn't reenable interrupts,
	// so that we have to do it in the instruction before, thus opening a
	// window for an interrupt while still being in the kernel, but having set
	// up everything for userland already.
	movl	%dr3, %edi						// thread pointer
	cmpb	$0, THREAD_in_kernel(%edi)
	je		int_bottom_user

	// disable interrupts -- the handler will enable them, if necessary
	cli

	pushl	%ebp
	movl	IFRAME_vector(%ebp), %eax
	call	*gInterruptHandlerTable(, %eax, 4)

	POP_IFRAME_AND_RETURN()


int_bottom_user:
	movl	$KERNEL_DATA_SEG,%eax
	cld
	movl	%eax,%ds
	movl	%eax,%es

	// disable breakpoints, if installed
	movl	%dr3, %edi				// thread pointer
	cli								// disable interrupts
	DISABLE_BREAKPOINTS()

	// update the thread's user time
	UPDATE_THREAD_USER_TIME()

	// leave interrupts disabled -- the handler will enable them, if
	// necessary

	pushl	%ebp
	movl	IFRAME_vector(%ebp), %eax
	call	*gInterruptHandlerTable(, %eax, 4)

	// Don't do any kernel exit work, if we actually came from the kernel (but
	// were already/still prepared for userland), since the iframe in this case
	// will be a kernel iframe and e.g. trying to set up a signal stack will not
	// be a very healthy endeavor.
	cmp		$USER_CODE_SEG, IFRAME_cs(%ebp)
	jne		1f

#if KERNEL_BREAKPOINTS
	jmp		kernel_exit_work
#else
	testl	$(THREAD_FLAGS_DEBUGGER_INSTALLED | THREAD_FLAGS_SIGNALS_PENDING \
			| THREAD_FLAGS_DEBUG_THREAD | THREAD_FLAGS_BREAKPOINTS_DEFINED) \
			, THREAD_flags(%edi)
	jnz		kernel_exit_work
#endif
1:

	cli								// disable interrupts

	// update the thread's kernel time and return
	UPDATE_THREAD_KERNEL_TIME()
	POP_IFRAME_AND_RETURN()


int_bottom_vm86:
	movl	$KERNEL_DATA_SEG,%eax
	cld
	movl	%eax,%ds
	movl	%eax,%es

	// update the thread's user time
	movl	%dr3, %edi				// thread pointer
	cli
	UPDATE_THREAD_USER_TIME()

	// leave interrupts disabled -- the handler will enable them, if
	// necessary

	pushl	%ebp
	movl	IFRAME_vector(%ebp), %eax
	call	*gInterruptHandlerTable(, %eax, 4)

	// update the thread's kernel time and return
	cli
	UPDATE_THREAD_KERNEL_TIME()
	lea		20(%ebp), %esp;
	popa;
	addl	$16,%esp;
	iret


// test interrupt handler for performance measurements
.align 16
.globl trap98
trap98:
	iret


.align 16
.globl trap99
trap99:
	// push error, vector, orig_edx, orig_eax, and other registers
	PUSH_IFRAME_BOTTOM_SYSCALL()

	call	handle_syscall

	POP_IFRAME_AND_RETURN()


handle_syscall:
	// save %eax, the number of the syscall
	movl	%eax, %esi

	movl	$KERNEL_DATA_SEG,%eax
	cld
	movl	%eax,%ds
	movl	%eax,%es

	lea		4(%esp), %ebp			// skipping the return address, the stack
									// frame pointer is the iframe
	movl	%dr3, %edi				// thread pointer

	// disable breakpoints, if installed
	cli								// disable interrupts
	DISABLE_BREAKPOINTS()

	// update the thread's user time
	UPDATE_THREAD_USER_TIME_PUSH_TIME()
		// leave the time on the stack (needed for post syscall debugging)

	sti								// enable interrupts

	cmp		$SYSCALL_COUNT, %esi	// check syscall number
	jae		bad_syscall_number
	movl	$kSyscallInfos, %eax	// get syscall info
	lea		(%eax, %esi, SYSCALL_INFO_sizeof), %edx

	// copy parameters onto this stack
	COPY_SYSCALL_PARAMETERS()

	// pre syscall debugging
	TRACE_PRE_SYSCALL()
	testl	$THREAD_FLAGS_DEBUGGER_INSTALLED, THREAD_flags(%edi)
	jnz		do_pre_syscall_debug
  pre_syscall_debug_done:

	// call the syscall function
	call	*SYSCALL_INFO_function(%esi)

	// overwrite the values of %eax and %edx on the stack (the syscall return
	// value)
	movl	%edx, IFRAME_edx(%ebp)
	movl	%eax, IFRAME_eax(%ebp)

	TRACE_POST_SYSCALL()

#if KERNEL_BREAKPOINTS
	jmp		post_syscall_work
#else
	testl	$(THREAD_FLAGS_DEBUGGER_INSTALLED | THREAD_FLAGS_SIGNALS_PENDING \
			| THREAD_FLAGS_DEBUG_THREAD | THREAD_FLAGS_BREAKPOINTS_DEFINED \
			| THREAD_FLAGS_64_BIT_SYSCALL_RETURN \
			| THREAD_FLAGS_RESTART_SYSCALL | THREAD_FLAGS_SYSCALL_RESTARTED) \
			, THREAD_flags(%edi)
	jnz		post_syscall_work
#endif

	cli								// disable interrupts

	// update the thread's kernel time
	UPDATE_THREAD_KERNEL_TIME()

	lea		-4(%ebp), %esp			// remove all parameters from the stack

	ret

  do_pre_syscall_debug:
	movl	%esp, %eax				// syscall parameters
	push	%eax
	movl	IFRAME_orig_eax(%ebp), %eax		// syscall number
	push	%eax
	call	user_debug_pre_syscall
	addl	$8, %esp
	jmp		pre_syscall_debug_done

  post_syscall_work:
	// clear the 64 bit return value and syscall restarted bits
	testl	$(THREAD_FLAGS_64_BIT_SYSCALL_RETURN \
				| THREAD_FLAGS_SYSCALL_RESTARTED), THREAD_flags(%edi)
	jz		2f
  1:
    movl	THREAD_flags(%edi), %eax
	movl	%eax, %edx
    andl	$~(THREAD_FLAGS_64_BIT_SYSCALL_RETURN \
				| THREAD_FLAGS_SYSCALL_RESTARTED), %edx
    lock
    cmpxchgl	%edx, THREAD_flags(%edi)
    jnz		1b
  2:

	// post syscall debugging
	testl	$THREAD_FLAGS_DEBUGGER_INSTALLED, THREAD_flags(%edi)
	jz		1f
	pushl	-8(%ebp)				// syscall start time
	pushl	-12(%ebp)
	movl	IFRAME_edx(%ebp), %edx	// syscall return value
	movl	IFRAME_eax(%ebp), %eax
	push	%edx
	push	%eax
	lea		16(%esp), %eax			// syscall parameters
	push	%eax
	movl	IFRAME_orig_eax(%ebp), %eax		// syscall number
	push	%eax
	call	user_debug_post_syscall
	addl	$24, %esp
  1:

  bad_syscall_number:
  kernel_exit_work:
	// if no signals are pending and the thread shall not be debugged, we can
	// use the quick kernel exit function
	testl	$(THREAD_FLAGS_SIGNALS_PENDING | THREAD_FLAGS_DEBUG_THREAD) \
			, THREAD_flags(%edi)
	jnz		kernel_exit_handle_signals
	cli								// disable interrupts
	call	thread_at_kernel_exit_no_signals
  kernel_exit_work_done:

	// syscall restart
	// TODO: this only needs to be done for syscalls!
	testl	$THREAD_FLAGS_RESTART_SYSCALL, THREAD_flags(%edi)
	jz		1f
	push	%ebp
	call	x86_restart_syscall
	addl	$4, %esp
  1:

	// install breakpoints, if defined
#if !KERNEL_BREAKPOINTS
	testl	$THREAD_FLAGS_BREAKPOINTS_DEFINED, THREAD_flags(%edi)
	jz		1f
#endif
	push	%ebp
	call	x86_init_user_debug_at_kernel_exit
  1:
	POP_IFRAME_AND_RETURN()

  kernel_exit_handle_signals:
	// make sure interrupts are enabled (they are, when coming from a syscall
	// but otherwise they might be disabled)
	sti
	call	thread_at_kernel_exit	// also disables interrupts
	jmp		kernel_exit_work_done

  bad_syscall_params:
	// clear the fault handler and exit normally
	movl	%dr3, %edi
	movl	$0, THREAD_fault_handler(%edi)
	jmp		kernel_exit_work


/*!	Handler called by the sysenter instruction
	ecx - user esp
*/
FUNCTION(x86_sysenter):
	// switch the stack
	movl	%dr3, %edx
	movl	THREAD_kernel_stack_top(%edx), %esp

	// push the iframe
	pushl	$USER_DATA_SEG			// user_ss
	pushl	%ecx					// user_esp
	pushfl							// eflags
	orl		$(1 << 9), (%esp)		// set the IF (interrupts) bit
	pushl	$USER_CODE_SEG			// user cs

	// user_eip
	movl	USER_COMMPAGE_ADDR + 4 * COMMPAGE_ENTRY_X86_SYSCALL, %edx
	addl	$4, %edx				// sysenter is at offset 2, 2 bytes long
	pushl	%edx

	PUSH_IFRAME_BOTTOM_SYSCALL()

	call	handle_syscall

	// pop the bottom of the iframe
	lea		4(%ebp), %esp	// skip iframe type

	pop		%gs
	addl	$4, %esp	/* we skip %fs, as this contains the CPU
						   dependent TLS segment */
	pop		%es
	pop		%ds

	popa

	// ecx already contains the user esp -- load edx with the return address
	movl	16(%esp), %edx

	// pop eflags, which also reenables interrupts
	addl	$24, %esp	// skip, orig_eax/edx, vector, error_code, eip, cs
	popfl

	sysexit


/*!	Is copied to the signal stack call to restore the original frame when
	the signal handler exits.
	The copying code (in arch_thread.c::arch_setup_signal_frame()) copies
	everything between the i386_return_from_signal and i386_end_return_from_signal
	symbols.
*/
FUNCTION(i386_return_from_signal):
	addl	$12, %esp	// Flushes the 3 arguments to sa_handler
	movl	$SYSCALL_RESTORE_SIGNAL_FRAME, %eax
		// This syscall will restore the cpu context to the
		// one existing before calling the signal handler
	movl	$0, %ecx
	lea		4(%esp), %edx
	int		$99
	ret
FUNCTION(i386_end_return_from_signal):


/*!	void i386_restore_frame_from_syscall(struct iframe iframe);
	Pops the regs of the iframe from the stack to make it current and then
	return to userland.
	Interrupts are disabled.
*/
FUNCTION(i386_restore_frame_from_syscall):
	lea		4(%esp), %ebp	// iframe to %ebp

	// check, if any kernel exit work has to be done
	movl	%dr3, %edi
	testl	$(THREAD_FLAGS_DEBUGGER_INSTALLED | THREAD_FLAGS_SIGNALS_PENDING \
			| THREAD_FLAGS_DEBUG_THREAD | THREAD_FLAGS_BREAKPOINTS_DEFINED) \
			, THREAD_flags(%edi)
	jnz		kernel_exit_work

	// update the thread's kernel time and return
	UPDATE_THREAD_KERNEL_TIME()
	POP_IFRAME_AND_RETURN()


/* status_t x86_vm86_enter(struct vm86_iframe *frame) */
FUNCTION(x86_vm86_enter):
	// save critical registers
	pushf
	pushl	%edi
	pushl	%esi
	pushl	%ebp
	pushl	%ebx
	push	%gs
	push	%fs

	// get pointers
	movl	32(%esp), %esi	// vm86 iframe
	push	%esi			// ... store iframe addr for x86_vm86_return
	movl	%dr3, %edi		// struct thread

	// make sure eflags are in right shape
	movl	VM86_IFRAME_flags(%esi), %eax
	andl	$0x200CD5, %eax		// leave ID,OF,DF,SF,ZF,AF,PF,CF flags
	orl		$0x20202, %eax		// set VM and IF flags (+10b)
	movl	%eax, VM86_IFRAME_flags(%esi)

	// update kernel_stack_top and tss.esp0
	pushl	THREAD_kernel_stack_top(%edi)
	movl	%esp, THREAD_kernel_stack_top(%edi)
	pushl	%esp
	call	i386_set_tss_and_kstack

	// go to vm86 mode
	cli
	UPDATE_THREAD_KERNEL_TIME()
	lea		20(%esi), %esp
	popa
	addl	$16, %esp
	iret


/* void x86_vm86_return(struct vm86_iframe *frame, status_t retval) */
FUNCTION(x86_vm86_return):
	// set stack to where x86_vm86_enter was left
	movl 8(%esp), %ebx
	movl 4(%esp), %esi
	cli
	movl %esi, %esp
	addl $VM86_IFRAME_sizeof, %esp

	// save old iframe
	popl	%eax	// old kernel stack top
	popl	%edi
	movl	$(VM86_IFRAME_sizeof >> 2), %ecx
	cld
	rep movsl

	// adjust kernel_stack_top and tss.esp0
	movl	%dr3, %edi
	movl	%eax, THREAD_kernel_stack_top(%edi)
	push	%eax
	call	i386_set_tss_and_kstack
	addl	$4, %esp

	// restore registers
	movl %ebx, %eax
	pop %fs
	pop %gs
	popl %ebx
	popl %ebp
	popl %esi
	popl %edi
	popf
	ret

