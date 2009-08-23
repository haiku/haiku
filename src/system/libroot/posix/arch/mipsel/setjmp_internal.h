/* 
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2005 Ingo Weinhold, bonefish@cs.tu-berlin.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SETJMP_INTERNAL_H
#define SETJMP_INTERNAL_H

/* These are the fields of the __jmp_regs structure */
#define JMP_REGS_R1		0
#define JMP_REGS_R2		4
#define JMP_REGS_R3		8


#warning DEFINE JMP_REGS


#define FUNCTION(x) .global x; .type x,@function; x

#endif	/* SETJMP_INTERNAL_H */
