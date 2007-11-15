/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#define FUNC(name) .align 2 ; .globl name ; .type name,@function ; name:

/* lure the preprocessor to not substitute the syscall as string */
#define IMM #

#define SYSCALL0(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL1(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL2(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL3(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL4(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL5(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL6(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL7(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL8(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL9(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL10(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL11(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL12(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

#define SYSCALL13(name, n) \
FUNC(name) \
	move.l	IMM n,%d0		;  \
	trap	#0		;  \
	rts

