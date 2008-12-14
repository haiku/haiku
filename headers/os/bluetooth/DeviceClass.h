/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _DEVICE_CLASS_H
#define _DEVICE_CLASS_H

#include <String.h>

namespace Bluetooth {

#define UNKNOWN_CLASS_OF_DEVICE 0x00


class DeviceClass {

public:
	DeviceClass(uint8 record[3])
	{
		SetRecord(record);
	}


	DeviceClass(uint32 record)
	{
		SetRecord(record);
	}


	DeviceClass(void)
	{
		this->record = UNKNOWN_CLASS_OF_DEVICE;
	}

	void SetRecord(uint8 record[3])
	{
		this->record = record[0]|record[1]<<8|record[2]<<16;
	}

	void SetRecord(uint32 record)
	{
		this->record = record;
	}

	uint GetServiceClass()
	{
		return (record & 0x00FFE000) >> 13;
	}

	uint GetMajorDeviceClass()
	{				
		return (record & 0x00001F00) >> 8;
	}

	uint GetMinorDeviceClass()
	{
		return (record & 0x000000FF) >> 2;
	}

	bool IsUnknownDeviceClass()
	{
		return (record == UNKNOWN_CLASS_OF_DEVICE);
	}

	void GetServiceClass(BString&);
	void GetMajorDeviceClass(BString&);
	void GetMinorDeviceClass(BString&);
	
	void DumpDeviceClass(BString&);

private:
	uint32 record;

};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::DeviceClass;
#endif

#endif // _DEVICE_CLASS_H
