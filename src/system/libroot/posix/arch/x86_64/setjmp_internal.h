/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETJMP_INTERNAL_H
#define SETJMP_INTERNAL_H


#define JMP_REGS_IP		0
#define JMP_REGS_SP		8
#define JMP_REGS_BP		16
#define JMP_REGS_BX		24
#define JMP_REGS_R12	32
#define JMP_REGS_R13	40
#define JMP_REGS_R14	48
#define JMP_REGS_R15	56

#include <asm_defs.h>

#endif	/* SETJMP_INTERNAL_H */
