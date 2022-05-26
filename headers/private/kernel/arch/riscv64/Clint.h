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
	uint32 msip    [4096]; // machine software interrupt pending, per CPU core
	uint64 mtimecmp[4095]; // per CPU core
	uint64 mtime;          // @0xBFF8
};

extern ClintRegs* volatile gClintRegs;


#endif	// _CLINT_H_
