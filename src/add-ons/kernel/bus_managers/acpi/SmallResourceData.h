/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */

#ifndef SMALLRESOURCEDATA_H
#define SMALLRESOURCEDATA_H

#include <ACPI.h>
#include <Errors.h>


enum resource_type
{
	kIOPort = 0x47,
	kEndTag = 0x78
};


struct io_port
{
	void	Print();
	//! The logical device decodes 16-bit addresses.
	bool	deviceAddresses16Bit;
	
	uint16	minimumBase;
	uint16	maximumBase;
	uint8	minimumBaseAlignment;
	uint8	contigiuousIOPorts;	
};


/*! ToDo: implement also the other resource data, see acpi section 6.2.4 */
class SmallResourceData
{
public:
						SmallResourceData(acpi_device_module_info* acpi,
							acpi_device acpiCookie, const char* method);
						~SmallResourceData();

	status_t			InitCheck();
	
	int8				GetType();
	/*! Get resource data and jump to the next resource. */				
	status_t			ReadIOPort(io_port* ioPort);

private:
	acpi_object_type*	fBuffer;
	size_t				fBufferSize;
	size_t				fRemainingBufferSize;
	
	int8*				fResourcePointer;

	status_t			fStatus;
};


#endif
