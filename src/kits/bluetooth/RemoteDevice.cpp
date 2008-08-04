/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/DiscoveryListener.h>
#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/LocalDevice.h>
#include <bluetooth/RemoteDevice.h>

#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/HCI/btHCI_event.h>

#include <bluetooth/bluetooth_error.h>

#include <CommandManager.h>
#include <bluetoothserver_p.h>

#include "KitSupport.h"



namespace Bluetooth {


bool
RemoteDevice::IsTrustedDevice(void)
{
	return true;
}


BString
RemoteDevice::GetFriendlyName(bool alwaysAsk)
{

	if (!alwaysAsk) {
		// Check if the name is already retrieved
		// TODO: Check if It is known from a KnownDevicesList
		return BString("Not implemented");
	}
	
	if (fDiscovererLocalDevice == NULL)
		return BString("#NoOwnerError#Not Valid name");
	
	if (fMessenger == NULL)
		return BString("#ServerNotReady#Not Valid name");
	
	void*  remoteNameCommand = NULL;
	size_t size;
	
	// Issue inquiry command
	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;
	
	request.AddInt32("hci_id", fDiscovererLocalDevice->GetID());
	
	// Fill the request
	remoteNameCommand = buildRemoteNameRequest(fBdaddr, fPageRepetitionMode, fClockOffset, &size); // Fill correctily
	
	request.AddData("raw command", B_ANY_TYPE, remoteNameCommand, size);
	
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_STATUS);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL, OCF_REMOTE_NAME_REQUEST));
	
	request.AddInt16("eventExpected",  HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE);
	
	
	if (fMessenger->SendMessage(&request, &reply) == B_OK)
	{
		BString name;
		int8 status;
	
		if ((reply.FindInt8("status", &status) == B_OK) && (status == BT_OK)) {
			
			if ((reply.FindString("friendlyname", &name) == B_OK ) ) {
				return name;
			} else {
				return BString(""); // should not happen
			}
	
		} else {
			// seems we got a negative event
			return BString("#CommandFailed#Not Valid name");
		}
	}
	
	return BString("#NotCompletedRequest#Not Valid name");
}


BString
RemoteDevice::GetFriendlyName()
{
	return GetFriendlyName(true);
}


bdaddr_t
RemoteDevice::GetBluetoothAddress()
{
	return fBdaddr;
}


bool
RemoteDevice::Equals(RemoteDevice* obj)
{
	bdaddr_t ba = obj->GetBluetoothAddress();

	return bdaddrUtils::Compare(&fBdaddr, &ba);
}

//  static RemoteDevice* GetRemoteDevice(Connection conn);

bool
RemoteDevice::Authenticate()
{
	return true;
}


//  bool Authorize(Connection conn);
//  bool Encrypt(Connection conn, bool on);


bool
RemoteDevice::IsAuthenticated()
{
	return true;
}


//  bool IsAuthorized(Connection conn);


bool
RemoteDevice::IsEncrypted()
{
	return true;
}

LocalDevice*
RemoteDevice::GetLocalDeviceOwner()
{
	return fDiscovererLocalDevice;
}

/* Private */
void
RemoteDevice::SetLocalDeviceOwner(LocalDevice* ld)
{
    fDiscovererLocalDevice = ld;
}


/* Constructor */
RemoteDevice::RemoteDevice(bdaddr_t address)
{
	fBdaddr = address;
	fMessenger = _RetrieveBluetoothMessenger();
}


RemoteDevice::RemoteDevice(const BString& address)
{
	fBdaddr = bdaddrUtils::FromString((const char *)address.String());
	fMessenger = _RetrieveBluetoothMessenger();
}


RemoteDevice::~RemoteDevice()
{
	if (fMessenger)
		delete fMessenger;
}


BString
RemoteDevice::GetProperty(const char* property) /* Throwing */
{
	return NULL;
}


void
RemoteDevice::GetProperty(const char* property, uint32* value) /* Throwing */
{

}


DeviceClass
RemoteDevice::GetDeviceClass()
{
	return fDeviceClass;
}


}
