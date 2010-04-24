/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _REMOTE_DEVICE_H
#define _REMOTE_DEVICE_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/bluetooth_error.h>
#include <bluetooth/BluetoothDevice.h>

#include <String.h>

#define B_BT_WAIT 0x00
#define B_BT_SUCCEEDED 0x01


namespace Bluetooth {

class Connection;
class LocalDevice;

class RemoteDevice : public BluetoothDevice {

public:
	static const int WAIT = B_BT_WAIT;
	static const int SUCCEEDED = B_BT_SUCCEEDED;

	virtual ~RemoteDevice();

	bool IsTrustedDevice();
	BString GetFriendlyName(bool alwaysAsk); /* Throwing */
	BString GetFriendlyName(void); /* Throwing */
	bdaddr_t GetBluetoothAddress();
	DeviceClass GetDeviceClass();

	bool Equals(RemoteDevice* obj);

	/*static RemoteDevice* GetRemoteDevice(Connection conn);   Throwing */
	bool		Authenticate(); /* Throwing */
	status_t	Disconnect(int8 reason = BT_REMOTE_USER_ENDED_CONNECTION);
	/* bool Authorize(Connection conn);  Throwing */
	/*bool Encrypt(Connection conn, bool on);  Throwing */
	bool IsAuthenticated(); /* Throwing */
	/*bool IsAuthorized(Connection conn);  Throwing */
	bool IsEncrypted(); /* Throwing */

	BString GetProperty(const char* property); /* Throwing */
	status_t GetProperty(const char* property, uint32* value); /* Throwing */

	LocalDevice* GetLocalDeviceOwner();

	RemoteDevice(const BString& address);
	RemoteDevice(const bdaddr_t address, uint8 record[3]);

protected:
	/* called by Discovery[Listener|Agent] */
	void SetLocalDeviceOwner(LocalDevice* ld);
	friend class DiscoveryListener;

private:

	LocalDevice* fDiscovererLocalDevice;
	BMessenger*	 fMessenger;

	uint16		fHandle;
	uint8		fPageRepetitionMode;
	uint8		fScanPeriodMode;
	uint8		fScanMode;
	uint16		fClockOffset;

};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::RemoteDevice;
#endif

#endif // _REMOTE_DEVICE_H
