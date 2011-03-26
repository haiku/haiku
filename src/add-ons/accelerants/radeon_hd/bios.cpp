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

