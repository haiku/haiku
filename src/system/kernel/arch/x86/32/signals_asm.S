/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>
#include <commpage_defs.h>

#include "asm_offsets.h"
#include "syscall_numbers.h"


/*!	\fn void x86_signal_frame_function_beos(signal_frame_data* frameData)
	\brief Wrapper function for BeOS-style signal handler functions.
	\param frameData The signal frame data.
*/
FUNCTION(x86_signal_frame_function_beos):
	// set up a stack frame
	push	%ebp
	mov		%esp, %ebp

	// Move our parameter to %esi, so we can conveniently work with it. Note
	// that we're free to use non-scratch registers without saving them, since
	// we don't have any caller to save them for. The caller will restore the
	// interrupted environment anyway.
	mov		8(%ebp), %esi

	// push the parameters for the handler function

	// make space for the vregs parameter
	lea		-VREGS_sizeof(%esp), %esp
	mov		%esp, %edi

	// copy the vregs via memcpy()
	pushl	$VREGS_sizeof
	lea		SIGNAL_FRAME_DATA_context + UCONTEXT_T_uc_mcontext(%esi), %eax
	push	%eax
	push	%edi
	movl	USER_COMMPAGE_ADDR + 4 * COMMPAGE_ENTRY_X86_MEMCPY, %eax
	call	*%eax
	addl	$12, %esp

	// the vregs are on the stack -- push user data and signal number
	movl	SIGNAL_FRAME_DATA_user_data(%esi), %eax
	push	%eax
	movl	SIGNAL_FRAME_DATA_info+SIGINFO_T_si_signo(%esi), %eax
	push	%eax

	// call the signal handler
	movl	SIGNAL_FRAME_DATA_handler(%esi), %eax
	call	*%eax
	addl	$8, %esp	// pop only signal number and user data arguments

	// copy the vregs back to the frameData structure
	pushl	$VREGS_sizeof
	push	%edi
	lea		SIGNAL_FRAME_DATA_context + UCONTEXT_T_uc_mcontext(%esi), %eax
	push	%eax
	movl	USER_COMMPAGE_ADDR + 4 * COMMPAGE_ENTRY_X86_MEMCPY, %eax
	call	*%eax
	addl	$12 + VREGS_sizeof, %esp

	// call the _kern_restore_signal_frame() syscall -- does not return (here)
	pushl	%esi
	pushl	$0	// dummy return value
	movl	$SYSCALL_RESTORE_SIGNAL_FRAME, %eax
	int		$99

	// never gets here
FUNCTION_END(x86_signal_frame_function_beos)
