/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _CLINT_H_
#define _CLINT_H_

#include <SupportDefs.h>


// core local interruptor
struct ClintRegs
{
	uint8  unknown1[0x4000];
	uint64 mTimeCmp[4095]; // per CPU core, but not implemented in temu
	uint64 mTime;          // @0xBFF8
};

extern ClintRegs* volatile gClintRegs;


#endif	// _CLINT_H_
