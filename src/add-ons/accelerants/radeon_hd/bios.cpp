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
#   define TRACE(x) _sPrintf x
#else
#   define TRACE(x) ;
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
			TRACE(("%s: CD_SUCCESS : success\n", __FUNCTION__));
			return B_OK;
			break;
		case CD_CALL_TABLE:
			TRACE(("%s: CD_CALL_TABLE : success\n", __FUNCTION__));
			return B_OK;
			break;
		case CD_COMPLETED:
			TRACE(("%s: CD_COMPLETED : success\n", __FUNCTION__));
			return B_OK;
			break;
		default:
			TRACE(("%s: UNKNOWN ERROR\n", __FUNCTION__));
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
	return malloc(size);
}


VOID
CailReleaseMemory(VOID *CAIL, VOID *addr)
{
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
	// TODO : CailReadATIRegister

	UINT32 ret = 0;
	// ret = RHDRegRead(((atomBiosHandlePtr)CAIL), idx << 2);
	return ret;
}


VOID
CailWriteATIRegister(VOID *CAIL, UINT32 idx, UINT32 data)
{
	// TODO : CailWriteATIRegister

	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterMMIO, idx << 2);
	// RHDRegWrite(((atomBiosHandlePtr)CAIL), idx << 2a, data);
}


VOID
CailReadPCIConfigData(VOID *CAIL, VOID* ret, UINT32 idx, UINT16 size)
{
	// TODO : CailReadPCIConfigData

	// pci_device_cfg_read(RHDPTRI((atomBiosHandlePtr)CAIL)->PciInfo,
	//	ret, idx << 2 , size >> 3, NULL);
}


VOID
CailWritePCIConfigData(VOID *CAIL, VOID *src, UINT32 idx, UINT16 size)
{
	// TODO : CailWritePCIConfigData

	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterPCICFG, idx << 2);
	// pci_device_cfg_write(RHDPTRI((atomBiosHandlePtr)CAIL)->PciInfo,
	//	src, idx << 2, size >> 3, NULL);
}


ULONG
CailReadPLL(VOID *CAIL, ULONG Address)
{
	// TODO : CailReadPLL

	ULONG ret = 0;
	// ret = _RHDReadPLL(((atomBiosHandlePtr)CAIL)->scrnIndex, Address);
	return ret;
}


VOID
CailWritePLL(VOID *CAIL, ULONG Address, ULONG Data)
{
	// TODO : CailWritePLL

	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterPLL, Address);
	// _RHDWritePLL(((atomBiosHandlePtr)CAIL)->scrnIndex, Address, Data);
}


ULONG
CailReadMC(VOID *CAIL, ULONG Address)
{
	// TODO : CailReadMC

	ULONG ret = 0;

	// ret = RHDReadMC(((atomBiosHandlePtr)CAIL), Address | MC_IND_ALL);
	return ret;
}


VOID
CailWriteMC(VOID *CAIL, ULONG Address, ULONG data)
{
	// TODO : CailWriteMC

	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterMC, Address);
	// RHDWriteMC(((atomBiosHandlePtr)CAIL),
	//	Address | MC_IND_ALL | MC_IND_WR_EN, data);
}


UINT32
CailReadFBData(VOID* CAIL, UINT32 idx)
{
	// TODO : CailReadFBData

	UINT32 ret = 0;
	/*
	if (((atomBiosHandlePtr)CAIL)->fbBase) {
		CARD8 *FBBase = (CARD8*)
			RHDPTRI((atomBiosHandlePtr)CAIL)->FbBase;
		ret = *((CARD32*)(FBBase + (((atomBiosHandlePtr)CAIL)->fbBase)
			+ idx));
		TRACE(("%s(%x) = %x\n", __func__, idx, ret));
	} else if (((atomBiosHandlePtr)CAIL)->scratchBase) {
		ret = *(CARD32*)((CARD8*)(((atomBiosHandlePtr)CAIL)->scratchBase)
			+ idx);
		TRACE(("%s(%x) = %x\n", __func__, idx, ret));
	} else {
		TRACE(("%s: no fbbase set\n", __func__));
		return 0;
	}
	*/
	return ret;
}


VOID
CailWriteFBData(VOID *CAIL, UINT32 idx, UINT32 data)
{
	// TODO : CailReadFBData

	TRACE(("%s(%x) = %x\n", __func__, idx, data));
	/*
	if (((atomBiosHandlePtr)CAIL)->fbBase) {
		CARD8 *FBBase = (CARD8*)
			RHDPTRI((atomBiosHandlePtr)CAIL)->FbBase;
		*((CARD32*)(FBBase + (((atomBiosHandlePtr)CAIL)->fbBase) + idx)) = data;
	} else if (((atomBiosHandlePtr)CAIL)->scratchBase) {
		*(CARD32*)((CARD8*)(((atomBiosHandlePtr)CAIL)->scratchBase) + idx) = data;
	} else
		TRACE(("%s: no fbbase set\n", __func__));
	*/
}


} // end extern "C"
