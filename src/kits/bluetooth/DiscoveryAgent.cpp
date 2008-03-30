/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/DiscoveryListener.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/LocalDevice.h>

#include <bluetooth/bluetooth_error.h>

#include <bluetooth/bluetoothserver_p.h>
#include <bluetooth/CommandManager.h>

#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/HCI/btHCI_event.h>


#include "KitSupport.h"

namespace Bluetooth {


RemoteDevicesList
DiscoveryAgent::RetrieveDevices(int option)
{
    /* No inquiry process initiated */
    if (fLastUsedListener == NULL)
        return NULL;

    return fLastUsedListener->GetRemoteDevicesList();
}


status_t
DiscoveryAgent::StartInquiry(int accessCode, DiscoveryListener* listener)
{
    return StartInquiry(accessCode, listener, BT_DEFAULT_INQUIRY_TIME);
}


status_t
DiscoveryAgent::StartInquiry(uint32 accessCode, DiscoveryListener* listener, bigtime_t secs)
{
    BMessenger* btsm = NULL;
	size_t size;

    if ((btsm = _RetrieveBluetoothMessenger()) == NULL)
    	return B_ERROR;

	if (secs < 1 || secs > 61 )
		return B_TIMED_OUT;

    void*  startInquiryCommand = NULL;
    int8   bt_status = BT_ERROR;

    // keep the listener whats the current listener for our inquiry state
    fLastUsedListener = listener;

    // Inform the listener who is gonna be its owner LocalDevice
    // and its discovered devices
    listener->SetLocalDeviceOwner(fLocalDevice);

    /* Issue inquiry command */
    BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
    BMessage reply;

    request.AddInt32("hci_id", fLocalDevice->GetID());
    
	startInquiryCommand = buildInquiry(accessCode, secs, BT_MAX_RESPONSES, &size);
	request.AddData("raw command", B_ANY_TYPE, startInquiryCommand, size);
    request.AddInt16("eventExpected",  HCI_EVENT_CMD_STATUS);
    request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_LINK_CONTROL, OCF_INQUIRY));

    if (btsm->SendMessage(&request, listener) == B_OK)
    {
    	return B_OK;
    }
	
	return B_ERROR;

}


status_t
DiscoveryAgent::CancelInquiry(DiscoveryListener* listener)
{
    BMessenger* btsm = NULL;

    if ((btsm = _RetrieveBluetoothMessenger()) == NULL)
    	return B_ERROR;

    void* cancelInquiryCommand = NULL;
    int8  bt_status = BT_ERROR;

    /* Issue inquiry command */
    BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
    BMessage reply;

    request.AddInt32("hci_id", fLocalDevice->GetID());
    // TODO: Add the raw command to the BMessage + expected event(s)



    if (btsm->SendMessage(&request, listener) == B_OK) {
        if (reply.FindInt8("status", &bt_status ) == B_OK ) {
            return bt_status;
        }
    }
	
	return B_ERROR;
}

void
DiscoveryAgent::SetLocalDeviceOwner(LocalDevice* ld)
{
    fLocalDevice = ld;
}

DiscoveryAgent::DiscoveryAgent()
{
    fLocalDevice = NULL;
}


}
