/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <bluetooth/bluetooth_error.h>

#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/HCI/btHCI_event.h>

#include <bluetooth/LocalDevice.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/DeviceClass.h>
#include <bluetooth/DiscoveryAgent.h>

#include <bluetooth/bdaddrUtils.h>
#include <bluetoothserver_p.h>
#include <CommandManager.h>

#include "KitSupport.h"


namespace Bluetooth {


LocalDevice*
LocalDevice::RequestLocalDeviceID(BMessage* request)
{
	BMessage reply;
	hci_id hid;
	LocalDevice* lDevice = NULL;

	BMessenger* messenger = _RetrieveBluetoothMessenger();

	if (messenger == NULL)
		return NULL;
	
	if (messenger->SendMessage(request, &reply) == B_OK &&
		reply.FindInt32("hci_id", &hid) == B_OK ) {
	
	    if (hid >= 0)
		    lDevice = new LocalDevice(hid);
    }
	
	delete messenger;
    return lDevice;
}


#if 0
#pragma -
#endif


LocalDevice*
LocalDevice::GetLocalDevice()
{
	BMessage request(BT_MSG_ACQUIRE_LOCAL_DEVICE);

	return RequestLocalDeviceID(&request);
}


LocalDevice*
LocalDevice::GetLocalDevice(const hci_id hid)
{ 
	BMessage request(BT_MSG_ACQUIRE_LOCAL_DEVICE);
	request.AddInt32("hci_id", hid);
	
	return RequestLocalDeviceID(&request);
}


LocalDevice*
LocalDevice::GetLocalDevice(const bdaddr_t bdaddr)
{

	BMessage request(BT_MSG_ACQUIRE_LOCAL_DEVICE);    
	request.AddData("bdaddr", B_ANY_TYPE, &bdaddr, sizeof(bdaddr_t));

	return RequestLocalDeviceID(&request);
}


uint32
LocalDevice::GetLocalDeviceCount()
{
	BMessenger* messenger = _RetrieveBluetoothMessenger();
	uint32 count = 0;	

	if (messenger != NULL) {

		BMessage request(BT_MSG_COUNT_LOCAL_DEVICES);	
		BMessage reply;

		if (messenger->SendMessage(&request, &reply) == B_OK)
			count = reply.FindInt32("count");

		delete messenger;
	}

	return count;
		
}


DiscoveryAgent*
LocalDevice::GetDiscoveryAgent()
{
	/* TODO: Study a singleton here */
	return new DiscoveryAgent(this);
}


BString
LocalDevice::GetProperty(const char* property)
{

	return NULL;

}


status_t
LocalDevice::GetProperty(const char* property, uint32* value)
{
	if (fMessenger == NULL)
		return B_ERROR;
	
	BMessage request(BT_MSG_GET_PROPERTY);
	BMessage reply;
	
	request.AddInt32("hci_id", hid);	
	request.AddString("property", property);
	
	if (fMessenger->SendMessage(&request, &reply) == B_OK) {
		if (reply.FindInt32("result", (int32*)value ) == B_OK ) {
			return B_OK;
			
		}		
	}

	return B_ERROR;
}


int 
LocalDevice::GetDiscoverable()
{

	return 0;
}


status_t 
LocalDevice::SetDiscoverable(int mode)
{
	if (fMessenger == NULL)
		return B_ERROR;
	
	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;
	
	size_t   size;
	int8	 bt_status = BT_ERROR;
	
	
	request.AddInt32("hci_id", hid);
	
	
	void* command = buildWriteScan(mode, &size);
	
	if (command == NULL) {
		return B_NO_MEMORY;
	}
	
	request.AddData("raw command", B_ANY_TYPE, command, size);
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_WRITE_SCAN_ENABLE));
	
	if (fMessenger->SendMessage(&request, &reply) == B_OK) {
		if (reply.FindInt8("status", &bt_status ) == B_OK ) {
			return bt_status;
			
		}
		
	}
	
	return B_ERROR;
}


bdaddr_t 
LocalDevice::GetBluetoothAddress()
{
	if (fMessenger == NULL)
		return bdaddrUtils::LocalAddress();

	size_t	size;
	void* command = buildReadBdAddr(&size);
	
	if (command == NULL)
		return bdaddrUtils::LocalAddress();

	const bdaddr_t* bdaddr;
	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;
	ssize_t	ssize;
	
	/* ADD ID */
	request.AddInt32("hci_id", hid);
	request.AddData("raw command", B_ANY_TYPE, command, size);
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_BD_ADDR));
	
	if (fMessenger->SendMessage(&request, &reply) == B_OK) {
		if (reply.FindData("bdaddr", B_ANY_TYPE, 0, (const void**)&bdaddr, &ssize) == B_OK )
			return *bdaddr;
	}

	return bdaddrUtils::LocalAddress();
}


BString 
LocalDevice::GetFriendlyName()
{
	if (fMessenger == NULL)
		return BString("Unknown|Messenger");

	size_t	size;
	void* command = buildReadLocalName(&size);
	if (command == NULL)
		return BString("Unknown|NoMemory");

	BString friendlyname;
	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;
	

	request.AddInt32("hci_id", hid);
	request.AddData("raw command", B_ANY_TYPE, command, size);
    request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
    request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_LOCAL_NAME));

	if (fMessenger->SendMessage(&request, &reply) == B_OK &&
		reply.FindString("friendlyname", &friendlyname) == B_OK){		
		
		return friendlyname;
	}
	
	return BString("Unknown|ServerFailed");
}


DeviceClass 
LocalDevice::GetDeviceClass()
{

	if (fDeviceClass.IsUnknownDeviceClass()) {

		if (fMessenger == NULL)
			return fDeviceClass;
	
		size_t	size;
		void* command = buildReadClassOfDevice(&size);
		if (command == NULL)
			return fDeviceClass;
	
		BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
		BMessage reply;
		const uint8*	record;
		ssize_t	ssize;
	
		request.AddInt32("hci_id", hid);
		request.AddData("raw command", B_ANY_TYPE, command, size);
	    request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	    request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_READ_CLASS_OF_DEV));
	
		if (fMessenger->SendMessage(&request, &reply) == B_OK &&
			reply.FindData("devclass", B_ANY_TYPE, 0, (const void**)&record, &ssize) == B_OK) {

			fDeviceClass.SetRecord(*record);
		}
	}

	return fDeviceClass;

}


status_t 
LocalDevice::ReadLocalVersion()
{
	size_t	size;
	int8	 bt_status = BT_ERROR;
	
	BluetoothCommand<> localVersion(OGF_INFORMATIONAL_PARAM,OCF_READ_LOCAL_VERSION,&size);
	
	
	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;
	
	request.AddInt32("hci_id", hid);
	request.AddData("raw command", B_ANY_TYPE, localVersion.Data(), size);
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_INFORMATIONAL_PARAM, OCF_READ_LOCAL_VERSION));
	
	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindInt8("status", &bt_status);

	return bt_status;
}


/*
ServiceRecord 
LocalDevice::getRecord(Connection notifier) {

}

void 
LocalDevice::updateRecord(ServiceRecord srvRecord) {

}
*/


LocalDevice::LocalDevice(hci_id hid) : hid(hid)
{
	fMessenger = _RetrieveBluetoothMessenger();
	ReadLocalVersion();
}


LocalDevice::~LocalDevice()
{
	if (fMessenger)
		delete fMessenger;
}


}
