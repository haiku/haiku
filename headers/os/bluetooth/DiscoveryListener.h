/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _DISCOVERY_LISTENER_H
#define _DISCOVERY_LISTENER_H

#include <Looper.h>
#include <ObjectList.h>

#include <bluetooth/DeviceClass.h>
#include <bluetooth/RemoteDevice.h>


#define BT_INQUIRY_COMPLETED	0x01 // HCI_EVENT_X check specs
#define BT_INQUIRY_TERMINATED	0x02 // HCI_EVENT_X
#define BT_INQUIRY_ERROR		0x03 // HCI_EVENT_X


namespace Bluetooth {

typedef BObjectList<RemoteDevice> RemoteDevicesList;

class LocalDevice;

class DiscoveryListener : public BLooper {

public:

	static const int INQUIRY_COMPLETED = BT_INQUIRY_COMPLETED;
	static const int INQUIRY_TERMINATED = BT_INQUIRY_TERMINATED;
	static const int INQUIRY_ERROR = BT_INQUIRY_ERROR;

	static const int SERVICE_SEARCH_COMPLETED = 0x01;
	static const int SERVICE_SEARCH_TERMINATED = 0x02;
	static const int SERVICE_SEARCH_ERROR = 0x03;
	static const int SERVICE_SEARCH_NO_RECORDS = 0x04;
	static const int SERVICE_SEARCH_DEVICE_NOT_REACHABLE = 0x06;

	virtual void DeviceDiscovered(RemoteDevice* btDevice, DeviceClass cod);
	/*
	virtual void servicesDiscovered(int transID, ServiceRecord[] servRecord);
	virtual void serviceSearchCompleted(int transID, int respCode);
	*/
	virtual void InquiryCompleted(int discType);

	/* JSR82 non-defined methods */
	virtual void InquiryStarted(status_t status);

private:

	RemoteDevicesList GetRemoteDevicesList(void);

	void MessageReceived(BMessage* msg);

	LocalDevice* fLocalDevice;
	RemoteDevicesList fRemoteDevicesList;

	friend class DiscoveryAgent;

protected:
	DiscoveryListener();
	void SetLocalDeviceOwner(LocalDevice* ld);
};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::DiscoveryListener;
#endif

#endif // _DISCOVERY_LISTENER_H
