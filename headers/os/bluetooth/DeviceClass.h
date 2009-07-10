/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _DEVICE_CLASS_H
#define _DEVICE_CLASS_H

#include <String.h>
#include <View.h>

namespace Bluetooth {

#define UNKNOWN_CLASS_OF_DEVICE 0x000000

class DeviceClass {

public:

	static const uint8 PixelsForIcon = 32;
	static const uint8 IconInsets = 5; 

	DeviceClass(uint8 record[3])
	{
		SetRecord(record);
	}


	DeviceClass(uint8 major, uint8 minor, uint16 service)
	{
		SetRecord(major, minor, service);
	}


	DeviceClass(void)
	{
		fRecord = UNKNOWN_CLASS_OF_DEVICE;
	}

	void SetRecord(uint8 record[3])
	{
		fRecord = record[0]|record[1]<<8|record[2]<<16;
	}

	void SetRecord(uint8 major, uint8 minor, uint16 service)
	{
		fRecord = (minor & 0x3F) << 2;
		fRecord |= (major & 0x1F) << 8;
		fRecord |= (service & 0x7FF) << 13;
	}


	uint16 ServiceClass()
	{
		return (fRecord & 0x00FFE000) >> 13;
	}

	uint8 MajorDeviceClass()
	{				
		return (fRecord & 0x00001F00) >> 8;
	}

	uint8 MinorDeviceClass()
	{
		return (fRecord & 0x000000FF) >> 2;
	}
	
	uint32 Record()
	{
		return fRecord;
	}

	bool IsUnknownDeviceClass()
	{
		return (fRecord == UNKNOWN_CLASS_OF_DEVICE);
	}

	void GetServiceClass(BString&);
	void GetMajorDeviceClass(BString&);
	void GetMinorDeviceClass(BString&);
	
	void DumpDeviceClass(BString&);
	
	void Draw(BView* view, const BPoint& point);

private:
	uint32 fRecord;

};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::DeviceClass;
#endif

#endif // _DEVICE_CLASS_H
