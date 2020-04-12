/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * Copyright 2012 Fredrik Modéen, [firstname]@[lastname]
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <bluetooth/bluetooth_error.h>

#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/HCI/btHCI_event.h>

#include <bluetooth/DeviceClass.h>
#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/LocalDevice.h>
#include <bluetooth/RemoteDevice.h>

#include <bluetooth/bdaddrUtils.h>
#include <bluetoothserver_p.h>
#include <CommandManager.h>

#include <new>

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

	if (messenger->SendMessage(request, &reply) == B_OK
		&& reply.FindInt32("hci_id", &hid) == B_OK ) {

		if (hid >= 0)
			lDevice = new (std::nothrow)LocalDevice(hid);
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
	// TODO: Study a singleton here
	return new (std::nothrow)DiscoveryAgent(this);
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

	request.AddInt32("hci_id", fHid);
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
	if (fMessenger == NULL)
		return -1;
	
	size_t	size;
	void* command = buildReadScan(&size);
	if (command == NULL)
		return -1;

	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	request.AddInt32("hci_id", fHid);
	request.AddData("raw command", B_ANY_TYPE, command, size);
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND,
		OCF_READ_SCAN_ENABLE));

	int8 discoverable;
	BMessage reply;
	if (fMessenger->SendMessage(&request, &reply) == B_OK
		&& reply.FindInt8("scan_enable", &discoverable) == B_OK)
		return discoverable;
	
	return -1;
}


status_t
LocalDevice::SetDiscoverable(int mode)
{
	if (fMessenger == NULL)
		return B_ERROR;

	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	size_t size;
	int8 bt_status = BT_ERROR;

	request.AddInt32("hci_id", fHid);


	void* command = buildWriteScan(mode, &size);

	if (command == NULL) {
		return B_NO_MEMORY;
	}

	request.AddData("raw command", B_ANY_TYPE, command, size);
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND,
		OCF_WRITE_SCAN_ENABLE));

	if (fMessenger->SendMessage(&request, &reply) == B_OK) {
		if (reply.FindInt8("status", &bt_status ) == B_OK ) {
			return bt_status;

		}
	}

	return B_ERROR;
}


struct authentication_t {
	uint8 param;
};


status_t
LocalDevice::SetAuthentication(bool authentication)
{
	return SingleParameterCommandRequest<struct authentication_t, uint8>
		(OGF_CONTROL_BASEBAND, OCF_WRITE_AUTH_ENABLE, authentication,
		NULL, fHid, fMessenger);
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

	request.AddInt32("hci_id", fHid);
	request.AddData("raw command", B_ANY_TYPE, command, size);
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_INFORMATIONAL_PARAM,
		OCF_READ_BD_ADDR));

	if (fMessenger->SendMessage(&request, &reply) == B_OK
		&& reply.FindData("bdaddr", B_ANY_TYPE, 0,
			(const void**)&bdaddr, &ssize) == B_OK)
			return *bdaddr;

	return bdaddrUtils::LocalAddress();
}


hci_id
LocalDevice::ID(void) const
{
	return fHid;
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


	request.AddInt32("hci_id", fHid);
	request.AddData("raw command", B_ANY_TYPE, command, size);
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND,
		OCF_READ_LOCAL_NAME));

	if (fMessenger->SendMessage(&request, &reply) == B_OK
		&& reply.FindString("friendlyname", &friendlyname) == B_OK)
		return friendlyname;

	return BString("Unknown|ServerFailed");
}


status_t
LocalDevice::SetFriendlyName(BString& name)
{
	int8 btStatus = BT_ERROR;

	if (fMessenger == NULL)
		return btStatus;

	BluetoothCommand<typed_command(hci_write_local_name)>
		writeName(OGF_CONTROL_BASEBAND, OCF_WRITE_LOCAL_NAME);

	strcpy(writeName->local_name, name.String());

	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	request.AddInt32("hci_id", fHid);
	request.AddData("raw command", B_ANY_TYPE,
		writeName.Data(), writeName.Size());
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND,
		OCF_WRITE_LOCAL_NAME));

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindInt8("status", &btStatus);

	return btStatus;
}


DeviceClass
LocalDevice::GetDeviceClass()
{

//	if (fDeviceClass.IsUnknownDeviceClass()) {

		if (fMessenger == NULL)
			return fDeviceClass;

		size_t	size;
		void* command = buildReadClassOfDevice(&size);
		if (command == NULL)
			return fDeviceClass;

		BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
		BMessage reply;
		const uint8* bufferRecord;
		ssize_t	ssize;

		request.AddInt32("hci_id", fHid);
		request.AddData("raw command", B_ANY_TYPE, command, size);
		request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
		request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND,
			OCF_READ_CLASS_OF_DEV));

		if (fMessenger->SendMessage(&request, &reply) == B_OK
			&& reply.FindData("devclass", B_ANY_TYPE, 0, (const void**)&bufferRecord,
			&ssize) == B_OK) {
			uint8 record[3] = { bufferRecord[0], bufferRecord[1], bufferRecord[2] };
			fDeviceClass.SetRecord(record);
		}
//	}

	return fDeviceClass;

}


status_t
LocalDevice::SetDeviceClass(DeviceClass deviceClass)
{
	int8 bt_status = BT_ERROR;

	if (fMessenger == NULL)
		return bt_status;

	BluetoothCommand<typed_command(hci_write_dev_class)>
		setDeviceClass(OGF_CONTROL_BASEBAND, OCF_WRITE_CLASS_OF_DEV);

	setDeviceClass->dev_class[0] = deviceClass.Record() & 0xFF;
	setDeviceClass->dev_class[1] = (deviceClass.Record() & 0xFF00) >> 8;
	setDeviceClass->dev_class[2] = (deviceClass.Record() & 0xFF0000) >> 16;

	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	request.AddInt32("hci_id", fHid);
	request.AddData("raw command", B_ANY_TYPE,
		setDeviceClass.Data(), setDeviceClass.Size());
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND,
		OCF_WRITE_CLASS_OF_DEV));

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindInt8("status", &bt_status);

	return bt_status;

}


status_t
LocalDevice::_ReadLocalVersion()
{
	int8 bt_status = BT_ERROR;

	BluetoothCommand<> localVersion(OGF_INFORMATIONAL_PARAM,
		OCF_READ_LOCAL_VERSION);

	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	request.AddInt32("hci_id", fHid);
	request.AddData("raw command", B_ANY_TYPE,
		localVersion.Data(), localVersion.Size());
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_INFORMATIONAL_PARAM,
		OCF_READ_LOCAL_VERSION));

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindInt8("status", &bt_status);

	return bt_status;
}


status_t
LocalDevice::_ReadBufferSize()
{
	int8 bt_status = BT_ERROR;

	BluetoothCommand<> BufferSize(OGF_INFORMATIONAL_PARAM,
		OCF_READ_BUFFER_SIZE);


	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	request.AddInt32("hci_id", fHid);
	request.AddData("raw command", B_ANY_TYPE,
		BufferSize.Data(), BufferSize.Size());
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_INFORMATIONAL_PARAM,
		OCF_READ_BUFFER_SIZE));

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindInt8("status", &bt_status);

	return bt_status;
}


status_t
LocalDevice::_ReadLocalFeatures()
{
	int8 bt_status = BT_ERROR;

	BluetoothCommand<> LocalFeatures(OGF_INFORMATIONAL_PARAM,
		OCF_READ_LOCAL_FEATURES);

	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	request.AddInt32("hci_id", fHid);
	request.AddData("raw command", B_ANY_TYPE,
		LocalFeatures.Data(), LocalFeatures.Size());
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_INFORMATIONAL_PARAM,
		OCF_READ_LOCAL_FEATURES));

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindInt8("status", &bt_status);

	return bt_status;
}


status_t
LocalDevice::_ReadLinkKeys()
{
	int8 bt_status = BT_ERROR;

	BluetoothCommand<> LocalFeatures(OGF_CONTROL_BASEBAND,
		OCF_READ_STORED_LINK_KEY);

	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	request.AddInt32("hci_id", fHid);
	request.AddData("raw command", B_ANY_TYPE,
		LocalFeatures.Data(), LocalFeatures.Size());
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND,
		OCF_READ_STORED_LINK_KEY));

	request.AddInt16("eventExpected",  HCI_EVENT_RETURN_LINK_KEYS);


	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindInt8("status", &bt_status);

	return bt_status;
}


struct pageTimeout_t {
	uint16 param;
};


status_t
LocalDevice::_ReadTimeouts()
{

	// Read PageTimeout
	NonParameterCommandRequest(OGF_CONTROL_BASEBAND,
		OCF_READ_PG_TIMEOUT, NULL, fHid, fMessenger);

	// Write PageTimeout
	SingleParameterCommandRequest<struct pageTimeout_t, uint16>
		(OGF_CONTROL_BASEBAND, OCF_WRITE_PG_TIMEOUT, 0x8000, NULL,
		fHid, fMessenger);

	// Write PageTimeout
	return SingleParameterCommandRequest<struct pageTimeout_t, uint16>
		(OGF_CONTROL_BASEBAND, OCF_WRITE_CA_TIMEOUT, 0x7d00, NULL,
		fHid, fMessenger);
}


status_t
LocalDevice::Reset()
{
	int8 bt_status = BT_ERROR;

	BluetoothCommand<> Reset(OGF_CONTROL_BASEBAND, OCF_RESET);

	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;

	request.AddInt32("hci_id", fHid);
	request.AddData("raw command", B_ANY_TYPE, Reset.Data(), Reset.Size());
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND,
		OCF_RESET));

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


LocalDevice::LocalDevice(hci_id hid)
	:
	BluetoothDevice(),
	fHid(hid)
{
	fMessenger = _RetrieveBluetoothMessenger();

	_ReadBufferSize();
	_ReadLocalFeatures();
	_ReadLocalVersion();
	_ReadTimeouts();
	_ReadLinkKeys();

	// Uncomment this if you want your device to have a nicer default name
	// BString name("HaikuBluetooth");
	// SetFriendlyName(name);


	uint32 value;

	// HARDCODE -> move this to addons
	if (GetProperty("manufacturer", &value) == B_OK
		&& value == 15) {

		// Uncomment this out if your Broadcom dongle is not working properly
		// Reset();	// Perform a reset to Broadcom buggyland

// Uncomment this out if your Broadcom dongle has a null bdaddr
//#define BT_WRITE_BDADDR_FOR_BCM2035
#ifdef BT_WRITE_BDADDR_FOR_BCM2035
#warning Writting broadcom bdaddr @ init.
		// try write bdaddr to a bcm2035 -> will be moved to an addon
		int8 bt_status = BT_ERROR;

		BluetoothCommand<typed_command(hci_write_bcm2035_bdaddr)>
			writeAddress(OGF_VENDOR_CMD, OCF_WRITE_BCM2035_BDADDR);

		BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
		BMessage reply;
		writeAddress->bdaddr.b[0] = 0x3C;
		writeAddress->bdaddr.b[1] = 0x19;
		writeAddress->bdaddr.b[2] = 0x30;
		writeAddress->bdaddr.b[3] = 0xC9;
		writeAddress->bdaddr.b[4] = 0x03;
		writeAddress->bdaddr.b[5] = 0x00;

		request.AddInt32("hci_id", fHid);
		request.AddData("raw command", B_ANY_TYPE,
			writeAddress.Data(), writeAddress.Size());
		request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
		request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_VENDOR_CMD,
			OCF_WRITE_BCM2035_BDADDR));

		if (fMessenger->SendMessage(&request, &reply) == B_OK)
			reply.FindInt8("status", &bt_status);
#endif
	}
}


LocalDevice::~LocalDevice()
{
	delete fMessenger;
}


} /* end namespace Bluetooth */
