/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
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
	trapa	#(n); \
	nop; \
	rts; \
	nop;

#define SYSCALL1(name, n) \
FUNC(name) \
	trapa	#(n); \
	rts; \
	nop;

#define SYSCALL2(name, n) \
FUNC(name) \
	trapa	#(n); \
	nop; \
	rts; \
	nop;

#define SYSCALL3(name, n) \
FUNC(name) \
	trapa	#(n); \
	nop; \
	rts; \
	nop;

#define SYSCALL4(name, n) \
FUNC(name) \
	trapa	#(n); \
	nop; \
	rts; \
	nop; \

#define SYSCALL5(name, n) \
FUNC(name) \
	mov.l	@r15,r0; \
	trapa	#(n); \
	nop; \
	rts; \
	nop;

#define SYSCALL6(name, n) \
FUNC(name) \
	mov.l	@(0,r15),r0; \
	mov.l	@(4,r15),r1; \
	trapa	#(n); \
	nop; \
	rts; \
	nop;
#define SYSCALL7(name, n) \
FUNC(name) \
	mov.l	@(0,r15),r0; \
	mov.l	@(4,r15),r1; \
	mov.l	@(8,r15),r2; \
	trapa	#(n); \
	nop; \
	rts; \
	nop;

#define SYSCALL8(name, n) \
FUNC(name) \
	mov.l	@(0,r15),r0; \
	mov.l	@(4,r15),r1; \
	mov.l	@(8,r15),r2; \
	mov.l	@(12,r15),r3; \
	trapa	#(n); \
	nop; \
	rts; \
	nop;

#define SYSCALL9(name, n) \
FUNC(name) \
	mov.l	@(0,r15),r0; \
	mov.l	@(4,r15),r1; \
	mov.l	@(8,r15),r2; \
	mov.l	@(12,r15),r3; \
	mov.l	r8,@-r15; \
	mov.l	@(20,r15),r8; \
	trapa	#(n); \
	nop; \
	rts; \
	mov.l	@r15+,r8

