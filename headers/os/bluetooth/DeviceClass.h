/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _DEVICE_CLASS_H
#define _DEVICE_CLASS_H

#include <String.h>

namespace Bluetooth {

class DeviceClass {

	public:

		DeviceClass(int record)
		{
			this->record = record;
		}

		DeviceClass()
		{
			this->record = 0;
		}


		int GetServiceClasses()
		{
			return (record & 0x00FFE000) >> 13;
		}


		int GetMajorDeviceClass()
		{
			return (record & 0x00001F00) >> 8;
		}


		void GetMajorDeviceClass(BString* str)
		{

		}


		int GetMinorDeviceClass()
		{
			return (record & 0x000000FF) >> 2;
		}


		void GetMinorDeviceClass(BString* str)
		{

		}
		
	private:
		int record;
};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::DeviceClass;
#endif

#endif
