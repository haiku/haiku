/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>


.text

/* user space half of the syscall mechanism, to be copied into the commpage */

// int 99 fallback
FUNCTION(x86_user_syscall_int):
	int	$99
	ret
FUNCTION_END(x86_user_syscall_int)
SYMBOL(x86_user_syscall_int_end):


// Intel sysenter/sysexit
FUNCTION(x86_user_syscall_sysenter):
	// sysexit forces us to trash edx (-> eip) and ecx (-> esp), but they are
	// scratch registers anyway. We use ecx right away to store esp.
	movl	%esp, %ecx
	sysenter
	ret
FUNCTION_END(x86_user_syscall_sysenter)
SYMBOL(x86_user_syscall_sysenter_end):


