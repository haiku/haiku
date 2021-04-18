/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef MMU_H
#define MMU_H


#include <SupportDefs.h>


extern uint8* gMemBase;
extern size_t gTotalMem;


void mmu_init();
void mmu_init_for_kernel();


#endif	/* MMU_H */
