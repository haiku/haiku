/* 
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights
 * reserved. Distributed under the terms of the MIT License.
 */
#ifndef SETJMP_INTERNAL_H
#define SETJMP_INTERNAL_H

/*	PPC function call ABI register use:
	r0		- volatile
	r1		- stack frame
	r2		- reserved
	r3-r4	- param passing, return values
	r5-r10	- param passing
	r11-r12	- volatile
	r13		- small data pointer
	r14-r30	- local vars
	r31		- local vars/environment
*/

/* These are the fields of the __jmp_regs structure */
#define JMP_REGS_R1		0
#define JMP_REGS_R2		4
#define JMP_REGS_R13	8
#define JMP_REGS_R14	12
#define JMP_REGS_R15	16
#define JMP_REGS_R16	20
#define JMP_REGS_R17	24
#define JMP_REGS_R18	28
#define JMP_REGS_R19	32
#define JMP_REGS_R20	36
#define JMP_REGS_R21	40
#define JMP_REGS_R22	44
#define JMP_REGS_R23	48
#define JMP_REGS_R24	52
#define JMP_REGS_R25	56
#define JMP_REGS_R26	60
#define JMP_REGS_R27	64
#define JMP_REGS_R28	68
#define JMP_REGS_R29	72
#define JMP_REGS_R30	76
#define JMP_REGS_R31	80
#define JMP_REGS_LR		84
#define JMP_REGS_CR		88

#define FUNCTION(x) .global x; .type x,@function; x

#endif	/* SETJMP_INTERNAL_H */
