/*
 * Copyright 2006, Marcus Overhagen <marcus@overhagen.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <KernelExport.h>
#include <string.h>

#include "pxe_undi.h"

#define TRACE_UNDI
#ifdef TRACE_UNDI
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


PXE_STRUCT *
pxe_undi_find_data()
{
	PXE_STRUCT *data = NULL;
	for (char *addr = (char *)0x8D000; addr < (char *)0xA0000; addr += 16) {
		if (*(uint32 *)addr == 'EXP!' /* '!PXE' */) {
			TRACE("found !PXE at %p\n", addr);
			data = (PXE_STRUCT *)addr;
			break;
		}
	}
	return data;
}
