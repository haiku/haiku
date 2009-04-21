/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _LOCAL_DEVICE_H
#define _LOCAL_DEVICE_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/DeviceClass.h>
#include <bluetooth/BluetoothDevice.h>

#include <bluetooth/HCI/btHCI.h>

#include <Messenger.h>
#include <Message.h>

#include <String.h>


namespace Bluetooth {

class DiscoveryAgent;

class LocalDevice : public BluetoothDevice {

public:
	/* Possible throwing */
	static	LocalDevice*	GetLocalDevice();
	static	uint32			GetLocalDeviceCount();

	static	LocalDevice*	GetLocalDevice(const hci_id hid);
	static	LocalDevice*	GetLocalDevice(const bdaddr_t bdaddr);

			DiscoveryAgent*	GetDiscoveryAgent();
			BString			GetFriendlyName();
			DeviceClass		GetDeviceClass();
	/* Possible throwing */
			status_t		SetDiscoverable(int mode);

			BString			GetProperty(const char* property);			
			status_t 		GetProperty(const char* property, uint32* value);

			int				GetDiscoverable();
			bdaddr_t		GetBluetoothAddress();
	/*
			ServiceRecord getRecord(Connection notifier);
			void updateRecord(ServiceRecord srvRecord);
	*/
			hci_id			GetID(void) {return hid;}
private:
			LocalDevice(hci_id hid);
			virtual	~LocalDevice();
			
			status_t		ReadLocalVersion();
			status_t		ReadBufferSize();
			status_t		Reset();
			
	static	LocalDevice*	RequestLocalDeviceID(BMessage* request);

			BMessenger*		fMessenger;

			hci_id			hid;

	friend class DiscoveryAgent;
	friend class RemoteDevice;
	friend class PincodeWindow;

};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::LocalDevice;
#endif

#endif // _LOCAL_DEVICE_H
