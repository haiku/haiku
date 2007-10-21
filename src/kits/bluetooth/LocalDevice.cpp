/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#include <bluetooth/LocalDevice.h>
#include <bluetooth/bdaddrUtils.h>

namespace Bluetooth {


	LocalDevice*
	LocalDevice::getLocalDevice() {
	
		return NULL;
	
	}
	
	
	LocalDevice*
	LocalDevice::getLocalDevice(uint8 dev) {
	
		return NULL;
	
	}
	
	
	LocalDevice*
	LocalDevice::getLocalDevice(bdaddr_t bdaddr){
	
		return NULL;
		
	}
	
	
	DiscoveryAgent*
	LocalDevice::getDiscoveryAgent() {
	
		return NULL;
	}
	
	
	BString 
	LocalDevice::getFriendlyName() {
	
		return NULL;
	}
	
	
	DeviceClass 
	LocalDevice::getDeviceClass() {
	
		return NULL;
	}
	
	
	bool 
	LocalDevice::setDiscoverable(int mode) {
	
		return false;
	}
	
	
	BString
	LocalDevice::getProperty(const char* property) {
	
		return NULL;
	
	}
	
	
	void 
	LocalDevice::getProperty(const char* property, uint32* value) {
	
		*value = 0;
	
	}
	
	
	int 
	LocalDevice::getDiscoverable() {
	
		return 0;
	}
	
	
	BString 
	LocalDevice::getBluetoothAddress() {
	
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
	LocalDevice::LocalDevice() {
	
	}


}
