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




PXE_STRUCT *gPxeData;


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


void 
pxe_undi_init()
{
	TRACE("pxe_undi_init\n");

	gPxeData = pxe_undi_find_data();
	if (!gPxeData)
		panic("can't find !PXE structure");

	TRACE("entrypoint at %p\n", *(uint32 *)&gPxeData->EntryPointSP);


	PXENV_UNDI_GET_INFORMATION get_info;

	memset(&get_info, 0, sizeof(get_info));

	TRACE("PXENV_UNDI_GET_INFORMATION at %p\n", &get_info);

	uint16 res = call_pxe_bios(gPxeData, UNDI_GET_INFORMATION, &get_info);

	TRACE("res = %04x\n", res);

	TRACE("Status = %x\n", get_info.Status);
	TRACE("BaseIo = %x\n", get_info.BaseIo);
	TRACE("IntNumber = %x\n", get_info.IntNumber);
	TRACE("MaxTranUnit = %x\n", get_info.MaxTranUnit);
	TRACE("HwType = %x\n", get_info.HwType);
	TRACE("HwAddrLen = %x\n", get_info.HwAddrLen);
	TRACE("MAC = %02x:%02x:%02x:%02x:%02x:%02x\n", get_info.CurrentNodeAddress[0], get_info.CurrentNodeAddress[1], 
												   get_info.CurrentNodeAddress[2], get_info.CurrentNodeAddress[3], 
												   get_info.CurrentNodeAddress[4], get_info.CurrentNodeAddress[5] );
	TRACE("RxBufCt = %x\n", get_info.RxBufCt);
	TRACE("TxBufCt = %x\n", get_info.TxBufCt);
}
