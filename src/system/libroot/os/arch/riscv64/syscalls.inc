/*
** Copyright 2001-2007, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#define _SYSCALL(name, n) \
.globl name; \
.type name,%function; \
.align 4; \
name: \
	li t0, n; \
	ecall; \
	ret; \
.size	name, .-name

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
#define SYSCALL17(name, n) _SYSCALL(name, n)
#define SYSCALL18(name, n) _SYSCALL(name, n)
#define SYSCALL19(name, n) _SYSCALL(name, n)
#define SYSCALL20(name, n) _SYSCALL(name, n)
