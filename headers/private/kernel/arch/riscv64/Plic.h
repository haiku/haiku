/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _PLIC_H_
#define _PLIC_H_

#include <SupportDefs.h>


// platform-level interrupt controller
struct PlicRegs
{
	// context = hart * 2 + mode, mode: 0 - machine, 1 - supervisor
	uint32 priority[1024];           // 0x000000
	uint32 pending[1024 / 32];       // 0x001000, bitfield
	uint8  unused1[0x2000 - 0x1080];
	uint32 enable[15872][1024 / 32]; // 0x002000, bitfield, [context][enable]
	uint8  unused2[0xe000];
	struct {
		uint32 priorityThreshold;
		uint32 claimAndComplete;
		uint8  unused[0xff8];
	} contexts[15872];               // 0x200000
};

extern PlicRegs* volatile gPlicRegs;


#endif	// _PLIC_H_
