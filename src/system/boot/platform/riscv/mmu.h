/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef MMU_H
#define MMU_H


#include <SupportDefs.h>

#include <boot/platform.h>
#include <util/FixedWidthPointer.h>


extern uint8* gMemBase;
extern size_t gTotalMem;


void mmu_init();
void mmu_init_for_kernel(addr_t& satp);


inline addr_t
fix_address(addr_t address)
{
	addr_t result;
	if (platform_bootloader_address_to_kernel_address((void *)address, &result)
		!= B_OK)
		return address;

	return result;
}


template<typename Type>
inline void
fix_address(FixedWidthPointer<Type>& p)
{
	if (p != NULL)
		p.SetTo(fix_address(p.Get()));
}


#endif	/* MMU_H */
