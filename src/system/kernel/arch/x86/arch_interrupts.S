/*
 * Copyright 2002-2011, The Haiku Team. All rights reserved.
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
#include <asm_defs.h>
#include <commpage_defs.h>
#include <thread_types.h>

#include "tracing_config.h"

#include "asm_offsets.h"
#include "syscall_numbers.h"
#include "syscall_table.h"


#define LOCK_THREAD_TIME()									\
	lea		THREAD_time_lock(%edi), %eax;					\
	pushl	%eax;											\
	call	acquire_spinlock;
		/* leave spinlock address on stack for UNLOCK_THREAD_TIME() */

#define UNLOCK_THREAD_TIME()								\
	/* spinlock address still on stack from */				\
	/* LOCK_THREAD_TIME() */								\
	call	release_spinlock;								\
	addl	$4, %esp;

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
	movb	$1, THREAD_in_kernel(%edi);

#define UPDATE_THREAD_USER_TIME()							\
	LOCK_THREAD_TIME()										\
	call	system_time;									\
	UPDATE_THREAD_USER_TIME_COMMON()						\
	UNLOCK_THREAD_TIME()

#define UPDATE_THREAD_USER_TIME_PUSH_TIME()					\
	call	system_time;									\
	push	%edx;											\
	push	%eax;											\
															\
	LOCK_THREAD_TIME()										\
															\
	/* recover the system time, note that */				\
	/* LOCK_THREAD_TIME() leaves an address on the stack */	\
	movl	4(%esp), %eax;									\
	movl	8(%esp), %edx;									\
															\
	UPDATE_THREAD_USER_TIME_COMMON()						\
															\
	UNLOCK_THREAD_TIME()

#define UPDATE_THREAD_KERNEL_TIME()							\
	LOCK_THREAD_TIME()										\
															\
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
	movb	$0, THREAD_in_kernel(%edi);						\
															\
	UNLOCK_THREAD_TIME()									\

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

#define STOP_USER_DEBUGGING()											\
	testl	$(THREAD_FLAGS_BREAKPOINTS_INSTALLED						\
				| THREAD_FLAGS_SINGLE_STEP), THREAD_flags(%edi);		\
	jz		1f;															\
	call	x86_exit_user_debug_at_kernel_entry;						\
  1:

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
.align 8; \
FUNCTION(name): \
	pushl	$vector; \
	pushl	%edx; \
	pushl	%eax; \
	jmp		int_bottom;	\
FUNCTION_END(name)

#define TRAP(name, vector) \
.align 8; \
FUNCTION(name): \
	pushl	$0; \
	pushl	$vector; \
	pushl	%edx; \
	pushl	%eax; \
	jmp		int_bottom; \
FUNCTION_END(name)

TRAP(trap0, 0)
TRAP(trap1, 1)
TRAP(trap2, 2)
TRAP(trap3, 3)
TRAP(trap4, 4)
TRAP(trap5, 5)
TRAP(trap6, 6)
TRAP(trap7, 7)

.align 8;
FUNCTION(double_fault):
	pushl	$-1;	// user-ss
	pushl	$-1;	// user-esp
	pushl	$-1;	// flags
	pushl	$KERNEL_CODE_SEG;	// cs
	pushl	$-1;	// eip
	pushl	$0;		// error-code
	pushl	$8;
	pushl	$-1;
	pushl	$-1;

	PUSH_IFRAME_BOTTOM(IFRAME_TYPE_OTHER)

	movl	%esp, %ebp		// frame pointer is the iframe

	pushl	%ebp
	call	x86_double_fault_exception

	// Well, there's no returning from a double fault, but maybe a real hacker
	// can repair things in KDL.
	POP_IFRAME_AND_RETURN()
FUNCTION_END(double_fault)


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

// legacy or ioapic interrupts
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

// additional ioapic interrupts
TRAP(trap48, 48)
TRAP(trap49, 49)
TRAP(trap50, 50)
TRAP(trap51, 51)
TRAP(trap52, 52)
TRAP(trap53, 53)
TRAP(trap54, 54)
TRAP(trap55, 55)

// configurable msi or msi-x interrupts
TRAP(trap56, 56)
TRAP(trap57, 57)
TRAP(trap58, 58)
TRAP(trap59, 59)
TRAP(trap60, 60)
TRAP(trap61, 61)
TRAP(trap62, 62)
TRAP(trap63, 63)
TRAP(trap64, 64)
TRAP(trap65, 65)
TRAP(trap66, 66)
TRAP(trap67, 67)
TRAP(trap68, 68)
TRAP(trap69, 69)
TRAP(trap70, 70)
TRAP(trap71, 71)
TRAP(trap72, 72)
TRAP(trap73, 73)
TRAP(trap74, 74)
TRAP(trap75, 75)
TRAP(trap76, 76)
TRAP(trap77, 77)
TRAP(trap78, 78)
TRAP(trap79, 79)
TRAP(trap80, 80)
TRAP(trap81, 81)
TRAP(trap82, 82)
TRAP(trap83, 83)
TRAP(trap84, 84)
TRAP(trap85, 85)
TRAP(trap86, 86)
TRAP(trap87, 87)
TRAP(trap88, 88)
TRAP(trap89, 89)
TRAP(trap90, 90)
TRAP(trap91, 91)
TRAP(trap92, 92)
TRAP(trap93, 93)
TRAP(trap94, 94)
TRAP(trap95, 95)
TRAP(trap96, 96)
TRAP(trap97, 97)
//TRAP(trap98, 98) // performance testing interrupt
//TRAP(trap99, 99) // syscall interrupt
TRAP(trap100, 100)
TRAP(trap101, 101)
TRAP(trap102, 102)
TRAP(trap103, 103)
TRAP(trap104, 104)
TRAP(trap105, 105)
TRAP(trap106, 106)
TRAP(trap107, 107)
TRAP(trap108, 108)
TRAP(trap109, 109)
TRAP(trap110, 110)
TRAP(trap111, 111)
TRAP(trap112, 112)
TRAP(trap113, 113)
TRAP(trap114, 114)
TRAP(trap115, 115)
TRAP(trap116, 116)
TRAP(trap117, 117)
TRAP(trap118, 118)
TRAP(trap119, 119)
TRAP(trap120, 120)
TRAP(trap121, 121)
TRAP(trap122, 122)
TRAP(trap123, 123)
TRAP(trap124, 124)
TRAP(trap125, 125)
TRAP(trap126, 126)
TRAP(trap127, 127)
TRAP(trap128, 128)
TRAP(trap129, 129)
TRAP(trap130, 130)
TRAP(trap131, 131)
TRAP(trap132, 132)
TRAP(trap133, 133)
TRAP(trap134, 134)
TRAP(trap135, 135)
TRAP(trap136, 136)
TRAP(trap137, 137)
TRAP(trap138, 138)
TRAP(trap139, 139)
TRAP(trap140, 140)
TRAP(trap141, 141)
TRAP(trap142, 142)
TRAP(trap143, 143)
TRAP(trap144, 144)
TRAP(trap145, 145)
TRAP(trap146, 146)
TRAP(trap147, 147)
TRAP(trap148, 148)
TRAP(trap149, 149)
TRAP(trap150, 150)
TRAP(trap151, 151)
TRAP(trap152, 152)
TRAP(trap153, 153)
TRAP(trap154, 154)
TRAP(trap155, 155)
TRAP(trap156, 156)
TRAP(trap157, 157)
TRAP(trap158, 158)
TRAP(trap159, 159)
TRAP(trap160, 160)
TRAP(trap161, 161)
TRAP(trap162, 162)
TRAP(trap163, 163)
TRAP(trap164, 164)
TRAP(trap165, 165)
TRAP(trap166, 166)
TRAP(trap167, 167)
TRAP(trap168, 168)
TRAP(trap169, 169)
TRAP(trap170, 170)
TRAP(trap171, 171)
TRAP(trap172, 172)
TRAP(trap173, 173)
TRAP(trap174, 174)
TRAP(trap175, 175)
TRAP(trap176, 176)
TRAP(trap177, 177)
TRAP(trap178, 178)
TRAP(trap179, 179)
TRAP(trap180, 180)
TRAP(trap181, 181)
TRAP(trap182, 182)
TRAP(trap183, 183)
TRAP(trap184, 184)
TRAP(trap185, 185)
TRAP(trap186, 186)
TRAP(trap187, 187)
TRAP(trap188, 188)
TRAP(trap189, 189)
TRAP(trap190, 190)
TRAP(trap191, 191)
TRAP(trap192, 192)
TRAP(trap193, 193)
TRAP(trap194, 194)
TRAP(trap195, 195)
TRAP(trap196, 196)
TRAP(trap197, 197)
TRAP(trap198, 198)
TRAP(trap199, 199)
TRAP(trap200, 200)
TRAP(trap201, 201)
TRAP(trap202, 202)
TRAP(trap203, 203)
TRAP(trap204, 204)
TRAP(trap205, 205)
TRAP(trap206, 206)
TRAP(trap207, 207)
TRAP(trap208, 208)
TRAP(trap209, 209)
TRAP(trap210, 210)
TRAP(trap211, 211)
TRAP(trap212, 212)
TRAP(trap213, 213)
TRAP(trap214, 214)
TRAP(trap215, 215)
TRAP(trap216, 216)
TRAP(trap217, 217)
TRAP(trap218, 218)
TRAP(trap219, 219)
TRAP(trap220, 220)
TRAP(trap221, 221)
TRAP(trap222, 222)
TRAP(trap223, 223)
TRAP(trap224, 224)
TRAP(trap225, 225)
TRAP(trap226, 226)
TRAP(trap227, 227)
TRAP(trap228, 228)
TRAP(trap229, 229)
TRAP(trap230, 230)
TRAP(trap231, 231)
TRAP(trap232, 232)
TRAP(trap233, 233)
TRAP(trap234, 234)
TRAP(trap235, 235)
TRAP(trap236, 236)
TRAP(trap237, 237)
TRAP(trap238, 238)
TRAP(trap239, 239)
TRAP(trap240, 240)
TRAP(trap241, 241)
TRAP(trap242, 242)
TRAP(trap243, 243)
TRAP(trap244, 244)
TRAP(trap245, 245)
TRAP(trap246, 246)
TRAP(trap247, 247)
TRAP(trap248, 248)
TRAP(trap249, 249)
TRAP(trap250, 250)

// smp / apic local interrupts
TRAP(trap251, 251)
TRAP(trap252, 252)
TRAP(trap253, 253)
TRAP(trap254, 254)
TRAP(trap255, 255)


.align 8;
FUNCTION(trap14_double_fault):
	pushl	$14
	pushl	$-1
	pushl	$-1

	PUSH_IFRAME_BOTTOM(IFRAME_TYPE_OTHER)

	movl	%esp, %ebp		// frame pointer is the iframe

	pushl	%ebp
	call	x86_page_fault_exception_double_fault

	POP_IFRAME_AND_RETURN()
FUNCTION_END(trap14_double_fault)


.align 16
STATIC_FUNCTION(int_bottom):
	PUSH_IFRAME_BOTTOM(IFRAME_TYPE_OTHER)

	movl	%esp, %ebp		// frame pointer is the iframe

	// Set the RF (resume flag) in EFLAGS. This prevents an instruction
	// breakpoint on the instruction we're returning to to trigger a debug
	// exception.
	orl		$0x10000, IFRAME_flags(%ebp);

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
FUNCTION_END(int_bottom)


STATIC_FUNCTION(int_bottom_user):
	movl	$KERNEL_DATA_SEG,%eax
	cld
	movl	%eax,%ds
	movl	%eax,%es

	// disable breakpoints, if installed
	movl	%dr3, %edi				// thread pointer
	cli								// disable interrupts
	STOP_USER_DEBUGGING()

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

	testl	$(THREAD_FLAGS_DEBUGGER_INSTALLED | THREAD_FLAGS_SIGNALS_PENDING \
			| THREAD_FLAGS_DEBUG_THREAD | THREAD_FLAGS_BREAKPOINTS_DEFINED) \
			, THREAD_flags(%edi)
	jnz		kernel_exit_work
1:

	cli								// disable interrupts

	// update the thread's kernel time and return
	UPDATE_THREAD_KERNEL_TIME()
	POP_IFRAME_AND_RETURN()
FUNCTION_END(int_bottom_user)


STATIC_FUNCTION(int_bottom_vm86):
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
FUNCTION_END(int_bottom_vm86)


// test interrupt handler for performance measurements
.align 16
FUNCTION(trap98):
	iret
FUNCTION_END(trap98)


.align 16
FUNCTION(trap99):
	// push error, vector, orig_edx, orig_eax, and other registers
	PUSH_IFRAME_BOTTOM_SYSCALL()

	call	handle_syscall

	POP_IFRAME_AND_RETURN()
FUNCTION_END(trap99)


STATIC_FUNCTION(handle_syscall):
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
	STOP_USER_DEBUGGING()

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

	testl	$(THREAD_FLAGS_DEBUGGER_INSTALLED | THREAD_FLAGS_SIGNALS_PENDING \
			| THREAD_FLAGS_DEBUG_THREAD | THREAD_FLAGS_BREAKPOINTS_DEFINED \
			| THREAD_FLAGS_64_BIT_SYSCALL_RETURN \
			| THREAD_FLAGS_RESTART_SYSCALL | THREAD_FLAGS_SYSCALL_RESTARTED) \
			, THREAD_flags(%edi)
	jnz		post_syscall_work

	cli								// disable interrupts

	// update the thread's kernel time
	UPDATE_THREAD_KERNEL_TIME()

	lea		-4(%ebp), %esp			// remove all parameters from the stack

	ret
FUNCTION_END(handle_syscall)

  STATIC_FUNCTION(do_pre_syscall_debug):
	movl	%esp, %eax				// syscall parameters
	push	%eax
	movl	IFRAME_orig_eax(%ebp), %eax		// syscall number
	push	%eax
	call	user_debug_pre_syscall
	addl	$8, %esp
	jmp		pre_syscall_debug_done
  FUNCTION_END(do_pre_syscall_debug)

  STATIC_FUNCTION(post_syscall_work):
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
  FUNCTION_END(post_syscall_work)

  bad_syscall_number:
  STATIC_FUNCTION(kernel_exit_work):
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
	testl	$THREAD_FLAGS_BREAKPOINTS_DEFINED, THREAD_flags(%edi)
	jz		1f
	push	%ebp
	call	x86_init_user_debug_at_kernel_exit
  1:
	POP_IFRAME_AND_RETURN()
  FUNCTION_END(kernel_exit_work)

  STATIC_FUNCTION(kernel_exit_handle_signals):
	// make sure interrupts are enabled (they are, when coming from a syscall
	// but otherwise they might be disabled)
	sti
	call	thread_at_kernel_exit	// also disables interrupts
	jmp		kernel_exit_work_done
  FUNCTION_END(kernel_exit_handle_signals)

  STATIC_FUNCTION(bad_syscall_params):
	// clear the fault handler and exit normally
	movl	%dr3, %edi
	movl	$0, THREAD_fault_handler(%edi)
	jmp		kernel_exit_work
  FUNCTION_END(bad_syscall_params)


/*!	Handler called by the sysenter instruction
	ecx - user esp
*/
FUNCTION(x86_sysenter):
	// get the thread
	movl	%dr3, %edx

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
FUNCTION_END(x86_sysenter)


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
	// get the iframe* parameter
	movl	4(%esp), %ebp
	movl	%ebp, %esp

	// check, if any kernel exit work has to be done
	movl	%dr3, %edi
	testl	$(THREAD_FLAGS_DEBUGGER_INSTALLED | THREAD_FLAGS_SIGNALS_PENDING \
			| THREAD_FLAGS_DEBUG_THREAD | THREAD_FLAGS_BREAKPOINTS_DEFINED) \
			, THREAD_flags(%edi)
	jnz		kernel_exit_work

	// update the thread's kernel time and return
	UPDATE_THREAD_KERNEL_TIME()
	POP_IFRAME_AND_RETURN()
FUNCTION_END(x86_return_to_userland)


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
	movl	%dr3, %edi		// Thread*

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
FUNCTION_END(x86_vm86_enter)


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
FUNCTION_END(x86_vm86_return)
