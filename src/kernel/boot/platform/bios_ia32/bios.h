/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef BIOS_H
#define BIOS_H


#include <SupportDefs.h>


// The values in this structure are passed to the BIOS call, and
// are updated to the register contents after that call.

struct bios_regs {
	uint32	eax;
	uint32	ebx;
	uint32	ecx;
	uint32	edx;
	uint32	esi;
	uint32	edi;
	uint16	es;
	uint16	flags;
};

#define CARRY_FLAG	0x01

extern
#ifdef __cplusplus
"C"
#endif
void call_bios(uint8 num, struct bios_regs *regs);

#endif	/* BIOS_H */
