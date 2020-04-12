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
		// Check if the name is already retrieved
		// TODO: Check if It is known from a KnownDevicesList
		return BString(B_TRANSLATE("Not implemented"));
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
				return name;
			} else {
				return BString(""); // should not happen
			}

		} else {
			// seems we got a negative event
			return BString(B_TRANSLATE("#CommandFailed#Not Valid name"));
		}
	}

	return BString(B_TRANSLATE("#NotCompletedRequest#Not Valid name"));
}


BString
RemoteDevice::GetFriendlyName()
{
	CALLED();
	return GetFriendlyName(true);
}


bdaddr_t
RemoteDevice::GetBluetoothAddress()
{
	CALLED();
	return fBdaddr;
}


bool
RemoteDevice::Equals(RemoteDevice* obj)
{
	CALLED();
	return bdaddrUtils::Compare(fBdaddr, obj->GetBluetoothAddress());
}


//  static RemoteDevice* GetRemoteDevice(Connection conn);


bool
RemoteDevice::Authenticate()
{
	CALLED();
	int8 btStatus = BT_ERROR;

	if (fMessenger == NULL || fDiscovererLocalDevice == NULL)
		return false;

	BluetoothCommand<typed_command(hci_cp_create_conn)>
		createConnection(OGF_LINK_CONTROL, OCF_CREATE_CONN);

	bdaddrUtils::Copy(createConnection->bdaddr, fBdaddr);
	createConnection->pscan_rep_mode = fPageRepetitionMode;
	createConnection->pscan_mode = fScanMode; // Reserved in spec 2.1
	createConnection->clock_offset = fClockOffset | 0x8000; // substract!

	uint32 roleSwitch;
	fDiscovererLocalDevice->GetProperty("role_switch_capable", &roleSwitch);
	createConnection->role_switch = (uint8)roleSwitch;

	uint32 packetType;
	fDiscovererLocalDevice->GetProperty("packet_type", &packetType);
	createConnection->pkt_type = (uint16)packetType;

	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	request.AddInt32("hci_id", fDiscovererLocalDevice->ID());
	request.AddData("raw command", B_ANY_TYPE,
		createConnection.Data(), createConnection.Size());

	// First we get the status about the starting of the connection
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_STATUS);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL,
		OCF_CREATE_CONN));

	// if authentication needed, we will send any of these commands
	// to accept or deny the LINK KEY [a]
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL,
		OCF_LINK_KEY_REPLY));

	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL,
		OCF_LINK_KEY_NEG_REPLY));

	// in negative case, a pincode will be replied [b]
	// this request will be handled by sepatated by the pincode window
	// request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	// request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL,
	//	OCF_PIN_CODE_REPLY));

	// [a] this is expected of authentication required
	request.AddInt16("eventExpected", HCI_EVENT_LINK_KEY_REQ);
	// [b] If we deny the key an authentication will be requested
	// but this request will be handled by sepatated by the pincode
	// window
	// request.AddInt16("eventExpected", HCI_EVENT_PIN_CODE_REQ);

	// this almost involves already the happy end
	request.AddInt16("eventExpected",  HCI_EVENT_LINK_KEY_NOTIFY);

	request.AddInt16("eventExpected", HCI_EVENT_CONN_COMPLETE);

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindInt8("status", &btStatus);

	if (btStatus == BT_OK) {
		reply.FindInt16("handle", (int16*)&fHandle);
		return true;
	} else
		return false;
}


status_t
RemoteDevice::Disconnect(int8 reason)
{
	CALLED();
	if (fHandle != invalidConnectionHandle) {

		int8 btStatus = BT_ERROR;

		if (fMessenger == NULL || fDiscovererLocalDevice == NULL)
			return false;

		BluetoothCommand<typed_command(struct hci_disconnect)>
			disconnect(OGF_LINK_CONTROL, OCF_DISCONNECT);

		disconnect->reason = reason;
		disconnect->handle = fHandle;

		BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
		BMessage reply;


		request.AddInt32("hci_id", fDiscovererLocalDevice->ID());
		request.AddData("raw command", B_ANY_TYPE,
			disconnect.Data(), disconnect.Size());

		request.AddInt16("eventExpected",  HCI_EVENT_CMD_STATUS);
		request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL,
			OCF_DISCONNECT));

		request.AddInt16("eventExpected",  HCI_EVENT_DISCONNECTION_COMPLETE);

		if (fMessenger->SendMessage(&request, &reply) == B_OK)
			reply.FindInt8("status", &btStatus);

		if (btStatus == BT_OK)
			fHandle = invalidConnectionHandle;

		return btStatus;

	}

	return B_ERROR;
}


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
	fDiscovererLocalDevice(NULL),
	fHandle(invalidConnectionHandle)
{
	CALLED();
	fBdaddr = address;
	fDeviceClass.SetRecord(record);
	fMessenger = _RetrieveBluetoothMessenger();
}


RemoteDevice::RemoteDevice(const BString& address)
	:
	BluetoothDevice(),
	fDiscovererLocalDevice(NULL),
	fHandle(invalidConnectionHandle)
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
