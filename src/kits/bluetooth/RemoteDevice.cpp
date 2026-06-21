/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <bluetooth/DeviceClass.h>
#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/DiscoveryListener.h>
#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/LocalDevice.h>
#include <bluetooth/RemoteDevice.h>

#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/HCI/btHCI_event.h>

#include <bluetooth/debug.h>
#include <bluetooth/bluetooth_error.h>

#include <Catalog.h>
#include <CommandManager.h>
#include <Locale.h>
#include <bluetoothserver_p.h>

#include "KitSupport.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RemoteDevice"


namespace Bluetooth {


// TODO: Check headers for valid/reserved ranges
static const uint16 invalidConnectionHandle = 0xF000;


bool
RemoteDevice::IsTrustedDevice(void)
{
	CALLED();
	return true;
}


BString
RemoteDevice::GetFriendlyName(bool alwaysAsk)
{
	CALLED();
	if (!alwaysAsk) {
		if (!fFriendlyName.IsEmpty() && fFriendlyNameIsComplete)
			return fFriendlyName;
	}

	if (fDiscovererLocalDevice == NULL)
		return BString(B_TRANSLATE("#NoOwnerError#Not Valid name"));

	if (fMessenger == NULL)
		return BString(B_TRANSLATE("#ServerNotReady#Not Valid name"));

	void* remoteNameCommand = NULL;
	size_t size;

	// Issue inquiry command
	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	request.AddInt32("hci_id", fDiscovererLocalDevice->ID());

	// Fill the request
	remoteNameCommand = buildRemoteNameRequest(fBdaddr, fPageRepetitionMode,
		fClockOffset, &size);

	request.AddData("raw command", B_ANY_TYPE, remoteNameCommand, size);

	request.AddInt16("eventExpected",  HCI_EVENT_CMD_STATUS);
	request.AddInt16("opcodeExpected",
		PACK_OPCODE(OGF_LINK_CONTROL, OCF_REMOTE_NAME_REQUEST));

	request.AddInt16("eventExpected",  HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE);


	if (fMessenger->SendMessage(&request, &reply) == B_OK) {
		BString name;
		int8 status;

		if ((reply.FindInt8("status", &status) == B_OK) && (status == BT_OK)) {

			if ((reply.FindString("friendlyname", &name) == B_OK )) {
				fFriendlyName = name;
				fFriendlyNameIsComplete = true;
				return name;
			} else {
				return BString(""); // should not happen
			}

		} else {
			// seems we got a negative event
			if (!fFriendlyName.IsEmpty())
				return fFriendlyName;

			return BString(B_TRANSLATE("#CommandFailed#Not Valid name"));
		}
	}

	return BString(B_TRANSLATE("#NotCompletedRequest#Not Valid name"));
}


BString
RemoteDevice::GetFriendlyName()
{
	CALLED();
	return GetFriendlyName(false);
}


BString
RemoteDevice::GetCachedFriendlyName()
{
	CALLED();
	return fFriendlyName;
}


bdaddr_t
RemoteDevice::GetBluetoothAddress()
{
	CALLED();
	return fBdaddr;
}


uint8
RemoteDevice::GetClockOffset()
{
	return fClockOffset;
}


uint8
RemoteDevice::GetScanMode()
{
	return fScanMode;
}


uint8
RemoteDevice::GetPageRepetitionMode()
{
	return fPageRepetitionMode;
}


bool
RemoteDevice::Equals(RemoteDevice* obj)
{
	CALLED();
	return bdaddrUtils::Compare(fBdaddr, obj->GetBluetoothAddress());
}


status_t
RemoteDevice::Connect()
{
	CALLED();
	BMessage request(BT_REQ_CREATE_CONN);

	request.AddInt32("hci_id", fDiscovererLocalDevice->ID());

	bdaddr_t bdaddr = GetBluetoothAddress();
	request.AddData("bdaddr", B_ANY_TYPE, &bdaddr, sizeof(bdaddr_t));

	uint16 packetType;
	fDiscovererLocalDevice->GetProperty("packet_type", (uint32*)&packetType);
	request.AddUInt16("packet type", packetType);

	request.AddUInt8("pscan_rep_mode", fPageRepetitionMode);
	request.AddUInt8("pscan_mode", fScanMode);
	request.AddUInt16("clock_offset", fClockOffset);

	uint8 roleSwitch;
	fDiscovererLocalDevice->GetProperty("role_switch_capable", (uint32*)&roleSwitch);
	request.AddUInt8("role_switch", roleSwitch);

	if (fMessenger->SendMessage(&request) == B_OK)
		return B_OK;
	return B_ERROR;
}


status_t
RemoteDevice::CancelConnection()
{
	CALLED();
	BMessage request(BT_REQ_CANCEL_CONN);

	request.AddInt32("hci_id", fDiscovererLocalDevice->ID());

	bdaddr_t bdaddr = GetBluetoothAddress();
	request.AddData("bdaddr", B_ANY_TYPE, &bdaddr, sizeof(bdaddr_t));

	if (fMessenger->SendMessage(&request) == B_OK)
		return B_OK;
	return B_ERROR;
}


status_t
RemoteDevice::Disconnect()
{
	CALLED();
	BMessage request(BT_REQ_DISCONNECT);

	request.AddInt32("hci_id", fDiscovererLocalDevice->ID());

	request.AddUInt16("handle", fHandle);
	request.AddUInt8("reason", BT_REMOTE_USER_ENDED_CONNECTION);

	if (fMessenger->SendMessage(&request) == B_OK)
		return B_OK;
	return B_ERROR;
}


//  static RemoteDevice* GetRemoteDevice(Connection conn);
//  bool Authorize(Connection conn);
//  bool Encrypt(Connection conn, bool on);


bool
RemoteDevice::IsAuthenticated()
{
	CALLED();
	return true;
}


//  bool IsAuthorized(Connection conn);


bool
RemoteDevice::IsEncrypted()
{
	CALLED();
	return true;
}


LocalDevice*
RemoteDevice::GetLocalDeviceOwner()
{
	CALLED();
	return fDiscovererLocalDevice;
}


/* Private */
void
RemoteDevice::SetLocalDeviceOwner(LocalDevice* ld)
{
	CALLED();
	fDiscovererLocalDevice = ld;
}


/* Constructor */
RemoteDevice::RemoteDevice(const bdaddr_t address, uint8 record[3])
	:
	BluetoothDevice(),
	fDiscovererLocalDevice(NULL)
{
	CALLED();
	fBdaddr = address;
	fDeviceClass.SetRecord(record);
	fMessenger = _RetrieveBluetoothMessenger();
}


RemoteDevice::RemoteDevice(const BString& address)
	:
	BluetoothDevice(),
	fDiscovererLocalDevice(NULL)
{
	CALLED();
	fBdaddr = bdaddrUtils::FromString((const char*)address.String());
	fMessenger = _RetrieveBluetoothMessenger();
}


RemoteDevice::~RemoteDevice()
{
	CALLED();
	delete fMessenger;
}


BString
RemoteDevice::GetProperty(const char* property) /* Throwing */
{
	return NULL;
}


status_t
RemoteDevice::GetProperty(const char* property, uint32* value) /* Throwing */
{
	CALLED();
	return B_ERROR;
}


DeviceClass
RemoteDevice::GetDeviceClass()
{
	CALLED();
	return fDeviceClass;
}

} /* end namespace Bluetooth */
