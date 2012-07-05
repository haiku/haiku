/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef LONG_H
#define LONG_H


#include <SupportDefs.h>


extern "C" void long_enter_kernel(uint32 physPML4, uint32 physGDT,
	uint64 virtGDT, uint64 entry, uint64 stackTop, uint64 kernelArgs,
	int currentCPU);

extern void long_start_kernel();


#endif /* LONG_H */
