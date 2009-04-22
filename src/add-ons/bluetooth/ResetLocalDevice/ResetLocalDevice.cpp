#include "ResetLocalDevice.h"

#include <Messenger.h>

#include <bluetooth/bluetooth_error.h>
#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/HCI/btHCI_event.h>

#include <bluetoothserver_p.h>
#include <CommandManager.h>


ResetLocalDeviceAddOn::ResetLocalDeviceAddOn()
{

}


const char*
ResetLocalDeviceAddOn::GetName()
{
	return "Reset LocalDevice";
}


status_t
ResetLocalDeviceAddOn::InitCheck(LocalDevice* lDevice)
{
	// you can perform a Reset in all Devices
	fCheck = B_OK;
	return fCheck;
}


const char*
ResetLocalDeviceAddOn::GetActionDescription()
{
	return "Perform a Reset command to the LocalDevice";
}


status_t
ResetLocalDeviceAddOn::TakeAction(LocalDevice* lDevice)
{
	int8	btStatus = BT_ERROR;

	BMessenger* fMessenger = new BMessenger(BLUETOOTH_SIGNATURE);

	if (fMessenger == NULL || !fMessenger->IsValid())
		return B_ERROR;
	
	BluetoothCommand<> Reset(OGF_CONTROL_BASEBAND, OCF_RESET);
	
	BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
	BMessage reply;
	
	request.AddInt32("hci_id", lDevice->ID());
	request.AddData("raw command", B_ANY_TYPE, Reset.Data(), Reset.Size());
	request.AddInt16("eventExpected",  HCI_EVENT_CMD_COMPLETE);
	request.AddInt16("opcodeExpected", PACK_OPCODE(OGF_CONTROL_BASEBAND, OCF_RESET));

	if (fMessenger->SendMessage(&request, &reply) == B_OK)
		reply.FindInt8("status", &btStatus);

	return btStatus;
}


const char*
ResetLocalDeviceAddOn::GetActionOnRemoteDescription()
{
	return NULL;
}


status_t
ResetLocalDeviceAddOn::TakeActionOnRemote(LocalDevice* lDevice, RemoteDevice* rDevice)
{
	return B_NOT_SUPPORTED;
}


const char*
ResetLocalDeviceAddOn::GetOverridenPropertiesDescription()
{
	// Example usage:
	//return "Replace the max count of SCO packets";
	return NULL;
}


BMessage*
ResetLocalDeviceAddOn::OverridenProperties(LocalDevice* lDevice, const char* property)
{
	// Example usage:
	//BMessage* newProperties = new BMessage();
	//newProperties->AddInt8("max_sco", 10);
	//return newProperties;
	
	return NULL;
}

INSTANTIATE_LOCAL_DEVICE_ADDON(ResetLocalDeviceAddOn);
EXPORT_LOCAL_DEVICE_ADDON;
