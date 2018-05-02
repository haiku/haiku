/*
 * Copyright 2018, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


.text

/* user space half of the syscall mechanism, to be copied into the commpage */


// Intel sysenter/sysexit
FUNCTION(x86_user_syscall_sysenter):
	// sysexit forces us to trash edx (-> eip) and ecx (-> esp), but they are
	// scratch registers anyway. We use ecx right away to store esp.
	movl	%esp, %ecx
	sysenter
	ret
FUNCTION_END(x86_user_syscall_sysenter)
SYMBOL(x86_user_syscall_sysenter_end):


// AMD syscall/sysret
FUNCTION(x86_user_syscall_syscall):
	syscall
	ret
FUNCTION_END(x86_user_syscall_syscall)
SYMBOL(x86_user_syscall_syscall_end):

