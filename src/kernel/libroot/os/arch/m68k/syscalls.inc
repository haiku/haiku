/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/*
** arg layout:
** r4-r7:  arg 1 - 4
** r0-r3:  arg 5 - 8
** r8-r13: arg 8 - 13
*/
#define FUNC(name) .align 2 ; .globl _##name ; .type _##name,@function ; _##name:

#define SYSCALL0(name, n) \
FUNC(name) \
	rts

#define SYSCALL1(name, n) \
FUNC(name) \
	rts

#define SYSCALL2(name, n) \
FUNC(name) \
	rts

#define SYSCALL3(name, n) \
FUNC(name) \
	rts

#define SYSCALL4(name, n) \
FUNC(name) \
	rts

#define SYSCALL5(name, n) \
FUNC(name) \
	rts

#define SYSCALL6(name, n) \
FUNC(name) \
	rts

#define SYSCALL7(name, n) \
FUNC(name) \
	rts

#define SYSCALL8(name, n) \
FUNC(name) \
	rts

#define SYSCALL9(name, n) \
FUNC(name) \
	rts

