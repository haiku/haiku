/* 
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights
 * reserved. Distributed under the terms of the MIT License.
 */
#ifndef SETJMP_INTERNAL_H
#define SETJMP_INTERNAL_H

/*	M68K function call ABI register use:
	d0		- return value
	d1		- volatile (return value?)
	d2-d7		- local vars
	a0		- return value
	a1		- volatile (return value?)
	a2-a6		- local vars
	a6		- (stack frame ?)
	a7		- stack pointer
*/

/* These are the fields of the __jmp_regs structure */
#define JMP_REGS_D2     0
#define JMP_REGS_PC     (4*(6+6))
#define JMP_REGS_CCR    (4*(6+6)+4)

#define FUNCTION(x) .global x; .type x,@function; x

#endif	/* SETJMP_INTERNAL_H */
