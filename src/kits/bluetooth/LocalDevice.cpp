/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#include <bluetooth/LocalDevice.h>
#include <bluetooth/DeviceClass.h>
#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/RemoteDevice.h>

#include <bluetooth/bdaddrUtils.h>

namespace Bluetooth {


	LocalDevice*
	LocalDevice::GetLocalDevice()
	{
	
		return NULL;
	}
	
	
	LocalDevice*
	LocalDevice::GetLocalDevice(uint8 dev)
	{
	
		return NULL;
	
	}
	
	
	LocalDevice*
	LocalDevice::GetLocalDevice(bdaddr_t bdaddr)
	{
	
		return NULL;
		
	}
	
	
	DiscoveryAgent*
	LocalDevice::GetDiscoveryAgent()
	{
	
		return NULL;
	}
	
	
	BString 
	LocalDevice::GetFriendlyName()
	{
	
		return NULL;
	}
	
	
	DeviceClass 
	LocalDevice::GetDeviceClass()
	{
	
		return NULL;
	}
	
	
	bool 
	LocalDevice::SetDiscoverable(int mode)
	{
	
		return false;
	}
	
	
	BString
	LocalDevice::GetProperty(const char* property)
	{
	
		return NULL;
	
	}
	
	
	void 
	LocalDevice::GetProperty(const char* property, uint32* value)
	{
	
		*value = 0;
	}
	
	
	int 
	LocalDevice::GetDiscoverable()
	{
	
		return 0;
	}
	
	
	BString 
	LocalDevice::GetBluetoothAddress()
	{
	
		return NULL;
	}
	
	
	/*
	ServiceRecord 
	LocalDevice::getRecord(Connection notifier) {
	
	}
	
	void 
	LocalDevice::updateRecord(ServiceRecord srvRecord) {
	
	}
	*/
	LocalDevice::LocalDevice()
	{
	
	}


}
