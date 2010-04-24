/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BLUETOOTH_DEVICE_H
#define _BLUETOOTH_DEVICE_H

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/DeviceClass.h>

#include <Message.h>
#include <Messenger.h>
#include <String.h>


namespace Bluetooth {

class BluetoothDevice {

public:
	BluetoothDevice()
		:
		fBdaddr(bdaddrUtils::NullAddress()),
		fDeviceClass()
	{

	}

	virtual BString 	GetFriendlyName()=0;
	virtual DeviceClass GetDeviceClass()=0;

	virtual BString 	GetProperty(const char* property)=0;
	virtual status_t 	GetProperty(const char* property, uint32* value)=0;

	virtual bdaddr_t 	GetBluetoothAddress()=0;

protected:
	bdaddr_t 			fBdaddr;
	DeviceClass 		fDeviceClass;
};

}


#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::BluetoothDevice;
#endif

#endif // _BLUETOOTH_DEVICE_H
