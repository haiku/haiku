/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include <Debug.h>

#include "bios.h"

#include "accelerant.h"
#include "accelerant_protos.h"


#undef TRACE

#define TRACE_ATOM
#ifdef TRACE_ATOM
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


status_t
AtomParser(void *parameterSpace, uint8_t index, void *handle, void *biosBase)
{
	DEVICE_DATA deviceData;

	deviceData.pParameterSpace = (UINT32*)parameterSpace;
	deviceData.CAIL = handle;
	deviceData.pBIOS_Image = (UINT8*)biosBase;
	deviceData.format = TABLE_FORMAT_BIOS;

	switch (ParseTable(&deviceData, index)) {
		case CD_SUCCESS:
			TRACE("%s: CD_SUCCESS : success\n", __func__);
			return B_OK;
			break;
		case CD_CALL_TABLE:
			TRACE("%s: CD_CALL_TABLE : success\n", __func__);
			return B_OK;
			break;
		case CD_COMPLETED:
			TRACE("%s: CD_COMPLETED : success\n", __func__);
			return B_OK;
			break;
		default:
			TRACE("%s: UNKNOWN ERROR\n", __func__);
	}
	return B_ERROR;
}


/*	Begin AtomBIOS OS callbacks
	These functions are used by AtomBios to access
	functions and data provided by the accelerant
*/
extern "C" {


VOID*
CailAllocateMemory(VOID *CAIL, UINT16 size)
{
	TRACE("AtomBios callback %s, size = %d\n", __func__, size);
	return malloc(size);
}


VOID
CailReleaseMemory(VOID *CAIL, VOID *addr)
{
	TRACE("AtomBios callback %s\n", __func__);
	free(addr);
}


VOID
CailDelayMicroSeconds(VOID *CAIL, UINT32 delay)
{
	usleep(delay);
}


UINT32
CailReadATIRegister(VOID* CAIL, UINT32 idx)
{
	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx << 2);
	return Read32(OUT, idx << 2);
}


VOID
CailWriteATIRegister(VOID *CAIL, UINT32 idx, UINT32 data)
{
	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx << 2);

	// TODO : save MMIO via atomSaveRegisters in CailWriteATIRegister
	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterMMIO, idx << 2);
	Write32(OUT, idx << 2, data);
}


VOID
CailReadPCIConfigData(VOID *CAIL, VOID* ret, UINT32 idx, UINT16 size)
{
	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx);
	// TODO : CailReadPCIConfigData

	// pci_device_cfg_read(RHDPTRI((atomBiosHandlePtr)CAIL)->PciInfo,
	//	ret, idx << 2 , size >> 3, NULL);
}


VOID
CailWritePCIConfigData(VOID *CAIL, VOID *src, UINT32 idx, UINT16 size)
{
	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx);
	// TODO : CailWritePCIConfigData

	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterPCICFG, idx << 2);
	// pci_device_cfg_write(RHDPTRI((atomBiosHandlePtr)CAIL)->PciInfo,
	//	src, idx << 2, size >> 3, NULL);
}


ULONG
CailReadPLL(VOID *CAIL, ULONG address)
{
	TRACE("AtomBios callback %s, addr (0x%X)\n", __func__, address);
	return Read32(PLL, address);
}


VOID
CailWritePLL(VOID *CAIL, ULONG address, ULONG data)
{
	TRACE("AtomBios callback %s, addr (0x%X)\n", __func__, address);

	// TODO : save PLL registers
	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterPLL, address);
	// TODO : Assumed screen index 0
	Write32(PLL, address, data);
}


ULONG
CailReadMC(VOID *CAIL, ULONG address)
{
	TRACE("AtomBios callback %s, addr (0x%X)\n", __func__, address);

	return Read32(MC, address | MC_IND_ALL);
}


VOID
CailWriteMC(VOID *CAIL, ULONG address, ULONG data)
{
	TRACE("AtomBios callback %s, addr (0x%X)\n", __func__, address);

	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterMC, address);
	Write32(MC, address | MC_IND_ALL | MC_IND_WR_EN, data);
}


UINT32
CailReadFBData(VOID* CAIL, UINT32 idx)
{
	// TODO : This should work only in theory and needs tested

	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx);

	UINT32 ret = 0;

	uint32_t fbLocation
		= gInfo->shared_info->frame_buffer_phys & 0xffffffff;

	// If we have a physical offset for our frame buffer, use it
	if (fbLocation > 0)
		ret = Read32(PLL, fbLocation + idx);
	else {
		TRACE("%s: ERROR: Frame Buffer offset not defined\n",
			__func__);
		return 0;
	}

	return ret;
}


VOID
CailWriteFBData(VOID *CAIL, UINT32 idx, UINT32 data)
{
	// TODO : This should work only in theory and needs tested

	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx);

	uint32_t fbLocation
		= gInfo->shared_info->frame_buffer_phys & 0xffffffff;

	// If we have a physical offset for our frame buffer, use it
	if (fbLocation > 0)
		Write32(OUT, fbLocation + idx, data);
	else
		TRACE("%s: ERROR: Frame Buffer offset not defined\n",
			__func__);
}


} // end extern "C"
