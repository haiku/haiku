/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
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

/* TODO: remove me */
#include <stdio.h>

namespace Bluetooth {

BMessenger*  LocalDevice::sfMessenger = NULL;


LocalDevice*
LocalDevice::RequestLocalDeviceID(BMessage* request)
{    
    BMessage reply;
    hci_id hid;
    
    if (sfMessenger->SendMessage(request, &reply) == B_OK &&
        reply.FindInt32("hci_id", &hid) == B_OK ){		
	    
	    if (hid >= 0) {
		    return new LocalDevice(hid);
		}
    }

    return NULL;	            
}


#if 0
#pragma -
#endif


LocalDevice*
LocalDevice::GetLocalDevice()
{          
    if ((sfMessenger = _RetrieveBluetoothMessenger()) == NULL)
        return NULL;
 
    BMessage request(BT_MSG_ACQUIRE_LOCAL_DEVICE);
        
    return RequestLocalDeviceID(&request);
}


LocalDevice*
LocalDevice::GetLocalDevice(hci_id hid)
{
    if ((sfMessenger = _RetrieveBluetoothMessenger()) == NULL)
        return NULL;
 
    BMessage request(BT_MSG_ACQUIRE_LOCAL_DEVICE);    
    request.AddInt32("hci_id", hid);
       
    return RequestLocalDeviceID(&request);

}


LocalDevice*
LocalDevice::GetLocalDevice(bdaddr_t bdaddr)
{
    if ((sfMessenger = _RetrieveBluetoothMessenger()) == NULL)
        return NULL;
 
    BMessage request(BT_MSG_ACQUIRE_LOCAL_DEVICE);    
    request.AddData("bdaddr", B_ANY_TYPE, &bdaddr, sizeof(bdaddr_t));
    
       
    return RequestLocalDeviceID(&request);
}


uint32
LocalDevice::GetLocalDeviceCount()
{
    if ((sfMessenger = _RetrieveBluetoothMessenger()) == NULL)
        return 0;

	BMessage request(BT_MSG_COUNT_LOCAL_DEVICES);	
	BMessage reply;

	if (sfMessenger->SendMessage(&request, &reply) == B_OK)
		return reply.FindInt32("count");
    else
        return 0;
		
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


void 
LocalDevice::GetProperty(const char* property, uint32* value)
{

	*value = 0;
}


int 
LocalDevice::GetDiscoverable()
{

	return 0;
}


status_t 
LocalDevice::SetDiscoverable(int mode)
{
    if ((fMessenger = _RetrieveBluetoothMessenger()) == NULL)
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
    if ((fMessenger = _RetrieveBluetoothMessenger()) == NULL)
        return bdaddrUtils::NullAddress();            
 
    const bdaddr_t* bdaddr;
    BMessage request(BT_MSG_GET_ADDRESS);
    BMessage reply;
 	ssize_t	size;   

    /* ADD ID */
    request.AddInt32("hci_id", hid);
    
    if (fMessenger->SendMessage(&request, &reply) == B_OK) {
    
    	if (reply.FindData("bdaddr", B_ANY_TYPE, 0, (const void**)&bdaddr, &size) == B_OK ){
			
	    	return *bdaddr;
	    
	    } else {
	    	return bdaddrUtils::NullAddress();	    
	    }
	    
    }

	return bdaddrUtils::NullAddress();
}


BString 
LocalDevice::GetFriendlyName()
{
    if ((fMessenger = _RetrieveBluetoothMessenger()) == NULL)
        return NULL;            
 
    BString friendlyname;
    BMessage request(BT_MSG_GET_FRIENDLY_NAME);
    BMessage reply;
    
    /* ADD ID */
    request.AddInt32("hci_id", hid);
    
    if (fMessenger->SendMessage(&request, &reply) == B_OK &&
        reply.FindString("friendlyname", &friendlyname) == B_OK ){		
	    
	    return friendlyname;
    }      
	
	return BString("Unknown");
}


DeviceClass 
LocalDevice::GetDeviceClass()
{

	return DeviceClass(0);

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

}


}
