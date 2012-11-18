/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


// The kernel follows the AMD64 ABI for parameter passing (first 6 arguments in
// registers and the remaining ones in on the stack), except that RCX (argument
// 4) is overwritten by the SYSCALL instruction, so it is moved to R10. Syscall
// number goes in RAX.


#define _SYSCALL_NO_RCX(name, n)	\
	.align 8;						\
	FUNCTION(name):					\
		movq	$n, %rax;			\
		syscall;					\
		ret;						\
	FUNCTION_END(name)

#define _SYSCALL_RCX(name, n)		\
	.align 8;						\
	FUNCTION(name):					\
		movq	%rcx, %r10;			\
		movq	$n, %rax;			\
		syscall;					\
		ret;						\
	FUNCTION_END(name)


#define SYSCALL0(name, n)	_SYSCALL_NO_RCX(name, n)
#define SYSCALL1(name, n)	_SYSCALL_NO_RCX(name, n)
#define SYSCALL2(name, n)	_SYSCALL_NO_RCX(name, n)
#define SYSCALL3(name, n)	_SYSCALL_NO_RCX(name, n)
#define SYSCALL4(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL5(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL6(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL7(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL8(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL9(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL10(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL11(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL12(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL13(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL14(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL15(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL16(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL17(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL18(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL19(name, n)	_SYSCALL_RCX(name, n)
#define SYSCALL20(name, n)	_SYSCALL_RCX(name, n)

