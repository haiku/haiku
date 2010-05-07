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
			status_t		SetFriendlyName(BString& name);
			DeviceClass		GetDeviceClass();
			status_t		SetDeviceClass(DeviceClass deviceClass);

	/* Possible throwing */
			status_t		SetDiscoverable(int mode);
			status_t		SetAuthentication(bool authentication);

			BString			GetProperty(const char* property);
			status_t 		GetProperty(const char* property, uint32* value);

			int				GetDiscoverable();
			bdaddr_t		GetBluetoothAddress();
	/*
			ServiceRecord getRecord(Connection notifier);
			void updateRecord(ServiceRecord srvRecord);
	*/
			hci_id	ID() const;
private:
			LocalDevice(hci_id hid);
			virtual	~LocalDevice();

			status_t		_ReadLocalVersion();
			status_t		_ReadBufferSize();
			status_t		_ReadLocalFeatures();
			status_t		_ReadTimeouts();
			status_t		_ReadLinkKeys();
			status_t		Reset();

	static	LocalDevice*	RequestLocalDeviceID(BMessage* request);

			BMessenger*		fMessenger;
			hci_id			fHid;

	friend class DiscoveryAgent;
	friend class RemoteDevice;
	friend class PincodeWindow;

};

}


#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::LocalDevice;
#endif

#endif // _LOCAL_DEVICE_H
