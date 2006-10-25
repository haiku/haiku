/*
 * Copyright 2006, Marcus Overhagen <marcus@overhagen.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <KernelExport.h>

#include "pxe_undi.h"

#define TRACE_UNDI
#ifdef TRACE_UNDI
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


struct pxe_struct 
{
	uint32	signature;
};

pxe_struct *gPxeData;


pxe_struct *
pxe_undi_find_data()
{
	pxe_struct *data = NULL;
	for (char *addr = (char *)0x8D000; addr < (char *)0xA0000; addr += 16) {
		if (*(uint32 *)addr == 'EXP!' /* '!PXE' */) {
			TRACE("found !PXE at %p\n", addr);
			data = (pxe_struct *)addr;
			break;
		}
	}
	return data;
}


void 
pxe_undi_init()
{
	TRACE("pxe_undi_init\n");

	gPxeData = pxe_undi_find_data();
	if (!gPxeData)
		panic("can't find !PXE structure");


}
