/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */

#include "SmallResourceData.h"

#include <stdlib.h>

//#define TRACE_SMALLRESOURCEDATA
#ifdef TRACE_SMALLRESOURCEDATA
#	define TRACE(x...) dprintf("Small Resource Data: "x)
#else
#	define TRACE(x...)
#endif


void
io_port::Print()
{
	dprintf("io_port:\n");
	int i = (deviceAddresses16Bit ? 1 : 0);
	dprintf("deviceAddresses16Bit %i\n", i);
	dprintf("minimumBase %i\n", minimumBase);
	dprintf("maximumBase %i\n", maximumBase);
	dprintf("minimumBaseAlignment %i\n", minimumBaseAlignment);
	dprintf("contigiuousIOPorts %i\n", contigiuousIOPorts);
}


SmallResourceData::SmallResourceData(acpi_device_module_info* acpi,
	acpi_device acpiCookie, const char* method)
{
	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;
	
	fStatus = acpi->evaluate_method(acpiCookie, method, NULL, &buffer);
	
	if (fStatus != B_OK)
		return;

	fBuffer = (acpi_object_type*)buffer.pointer;
	
	if (fBuffer[0].object_type != ACPI_TYPE_BUFFER) {
		fStatus = B_ERROR;
		return;
	}

	fResourcePointer = (int8*)fBuffer[0].data.buffer.buffer;
	fBufferSize = fBuffer[0].data.buffer.length;
	fRemainingBufferSize = fBufferSize;
	
	// ToDo: Check checksum of the endtag. The sum of all databytes + checksum
	// is zero. See section 6.4.2.8.
}


SmallResourceData::~SmallResourceData()
{
	if (InitCheck() == B_OK)
		free(fBuffer);
}


status_t
SmallResourceData::InitCheck()
{
	return fStatus;	
}
	

int8
SmallResourceData::GetType()
{
	return *fResourcePointer;
}


status_t
SmallResourceData::ReadIOPort(io_port* ioPort)
{
	const size_t packageSize = 8;
	
	if (fRemainingBufferSize < packageSize)
		return B_ERROR;
	if (fResourcePointer[0] != kIOPort)
		return B_ERROR;

	ioPort->deviceAddresses16Bit = (fResourcePointer[1] == 1);
	
	int16 tmp;
	tmp = fResourcePointer[3];
	tmp = tmp << 8;
	tmp |= fResourcePointer[2];
	ioPort->minimumBase = tmp;
		
	tmp = fResourcePointer[5];
	tmp = tmp << 8;
	tmp |= fResourcePointer[4];
	ioPort->maximumBase = tmp;
	
	ioPort->minimumBaseAlignment = fResourcePointer[6];
	ioPort->contigiuousIOPorts = fResourcePointer[7];
	
	fResourcePointer += packageSize;
	fRemainingBufferSize -= packageSize;
	TRACE("SmallResourceData: remaining buffer size %i\n",
		int(fRemainingBufferSize));
	return B_OK;
}
