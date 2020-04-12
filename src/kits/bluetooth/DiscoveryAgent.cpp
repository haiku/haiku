/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <bluetooth/bluetooth_error.h>
#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/DiscoveryListener.h>
#include <bluetooth/LocalDevice.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/debug.h>

#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/HCI/btHCI_event.h>

#include <bluetoothserver_p.h>
#include <CommandManager.h>

#include "KitSupport.h"


namespace Bluetooth {


RemoteDevicesList
DiscoveryAgent::RetrieveDevices(int option)
{
	CALLED();
    // No inquiry process initiated
    if (fLastUsedListener == NULL)
        return RemoteDevicesList();

    return fLastUsedListener->GetRemoteDevicesList();
}


status_t
DiscoveryAgent::StartInquiry(int accessCode, DiscoveryListener* listener)
{
	CALLED();
    return StartInquiry(accessCode, listener, GetInquiryTime());
}


status_t
DiscoveryAgent::StartInquiry(uint32 accessCode, DiscoveryListener* listener,
	bigtime_t secs)
{
	CALLED();
    size_t size;

	if (fMessenger == NULL)
		return B_ERROR;

	if (secs < 1 || secs > 61 )
		return B_TIMED_OUT;

    void*  startInquiryCommand = NULL;

    // keep the listener whats the current listener for our inquiry state
    fLastUsedListener = listener;

    // Inform the listener who is gonna be its owner LocalDevice
    // and its discovered devices
    listener->SetLocalDeviceOwner(fLocalDevice);

    /* Issue inquiry command */
    BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
    BMessage reply;

    request.AddInt32("hci_id", fLocalDevice->ID());

    startInquiryCommand = buildInquiry(accessCode, secs, BT_MAX_RESPONSES,
		&size);

    // For stating the inquiry
    request.AddData("raw command", B_ANY_TYPE, startInquiryCommand, size);
    request.AddInt16("eventExpected", HCI_EVENT_CMD_STATUS);
    request.AddInt16("opcodeExpected",
		PACK_OPCODE(OGF_LINK_CONTROL, OCF_INQUIRY));

	// For getting each discovered message
    request.AddInt16("eventExpected",  HCI_EVENT_INQUIRY_RESULT);

	// For finishing each discovered message
    request.AddInt16("eventExpected",  HCI_EVENT_INQUIRY_COMPLETE);


    if (fMessenger->SendMessage(&request, listener) == B_OK)
    {
    	return B_OK;
    }
	
	return B_ERROR;

}


status_t
DiscoveryAgent::CancelInquiry(DiscoveryListener* listener)
{
	CALLED();
    size_t size;

    if (fMessenger == NULL)
    	return B_ERROR;

    void* cancelInquiryCommand = NULL;
    int8  bt_status = BT_ERROR;

    /* Issue inquiry command */
    BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
    BMessage reply;

    request.AddInt32("hci_id", fLocalDevice->ID());

    cancelInquiryCommand = buildInquiryCancel(&size);
    request.AddData("raw command", B_ANY_TYPE, cancelInquiryCommand, size);
    request.AddInt16("eventExpected",  HCI_EVENT_CMD_STATUS);
    request.AddInt16("opcodeExpected",
		PACK_OPCODE(OGF_LINK_CONTROL, OCF_INQUIRY_CANCEL));

    if (fMessenger->SendMessage(&request, &reply) == B_OK) {
        if (reply.FindInt8("status", &bt_status ) == B_OK ) {
			return bt_status;
		}
    }

    return B_ERROR;
}


void
DiscoveryAgent::SetLocalDeviceOwner(LocalDevice* ld)
{
	CALLED();
    fLocalDevice = ld;
}


DiscoveryAgent::DiscoveryAgent(LocalDevice* ld)
{
	CALLED();
	fLocalDevice = ld;
	fMessenger = _RetrieveBluetoothMessenger();
}


DiscoveryAgent::~DiscoveryAgent()
{
	CALLED();
	delete fMessenger;
}


} /* End namespace Bluetooth */
