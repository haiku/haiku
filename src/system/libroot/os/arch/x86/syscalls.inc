/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/*
 * syscall interface works as such:
 * eax has syscall #
 * esp + 4 points to the syscall parameters
 * branch to the syscall vector in the commpage
 */

#include <os/kernel/arch/x86/commpage.h>

#define _SYSCALL(name, n) \
.globl name; \
.type name,@function; \
.align 8; \
name: \
	movl	$n,%eax; \
	jmp	*(USER_COMMPAGE_ADDR + COMMPAGE_ENTRY_SYSCALL * 4)

#define SYSCALL0(name, n) _SYSCALL(name, n)
#define SYSCALL1(name, n) _SYSCALL(name, n)
#define SYSCALL2(name, n) _SYSCALL(name, n)
#define SYSCALL3(name, n) _SYSCALL(name, n)
#define SYSCALL4(name, n) _SYSCALL(name, n)
#define SYSCALL5(name, n) _SYSCALL(name, n)
#define SYSCALL6(name, n) _SYSCALL(name, n)
#define SYSCALL7(name, n) _SYSCALL(name, n)
#define SYSCALL8(name, n) _SYSCALL(name, n)
#define SYSCALL9(name, n) _SYSCALL(name, n)
#define SYSCALL10(name, n) _SYSCALL(name, n)
#define SYSCALL11(name, n) _SYSCALL(name, n)
#define SYSCALL12(name, n) _SYSCALL(name, n)
#define SYSCALL13(name, n) _SYSCALL(name, n)
#define SYSCALL14(name, n) _SYSCALL(name, n)
#define SYSCALL15(name, n) _SYSCALL(name, n)
#define SYSCALL16(name, n) _SYSCALL(name, n)

