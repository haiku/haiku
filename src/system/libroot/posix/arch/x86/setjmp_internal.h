/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef SETJMP_INTERNAL_H
#define SETJMP_INTERNAL_H

/* These are the fields of the __jmp_regs structure */

#define JMP_REGS_EBX	0
#define JMP_REGS_ESI	4
#define JMP_REGS_EDI	8
#define JMP_REGS_EBP	12
#define JMP_REGS_ESP	16
#define JMP_REGS_PC		20

#include <asm_defs.h>

#endif	/* SETJMP_INTERNAL_H */
