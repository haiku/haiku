/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#include <bluetooth/bluetooth_error.h>

#include <bluetooth/LocalDevice.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/DeviceClass.h>
#include <bluetooth/DiscoveryAgent.h>

#include <bluetooth/bdaddrUtils.h>

#include "bluetoothserver_p.h"

/* TODO: remove me */
#include <stdio.h>

namespace Bluetooth {

BMessenger*  LocalDevice::sfMessenger = NULL;

status_t
LocalDevice::SRetrieveBluetoothMessenger(void)
{
    if (sfMessenger == NULL || !sfMessenger->IsValid() )
        sfMessenger = new BMessenger(BLUETOOTH_SIGNATURE);

    return (sfMessenger != NULL)?B_OK:B_ERROR;
}

status_t LocalDevice::RetrieveBluetoothMessenger(void)
{
    if (fMessenger == NULL || !fMessenger->IsValid())
        fMessenger = new BMessenger(BLUETOOTH_SIGNATURE);

    return (fMessenger != NULL)?B_OK:B_ERROR;
}

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
    if (SRetrieveBluetoothMessenger() != B_OK)
        return NULL;
 
    BMessage request(BT_MSG_ACQUIRE_LOCAL_DEVICE);
        
    return RequestLocalDeviceID(&request);
}


LocalDevice*
LocalDevice::GetLocalDevice(hci_id hid)
{
    if (SRetrieveBluetoothMessenger() != B_OK)
        return NULL;
 
    BMessage request(BT_MSG_ACQUIRE_LOCAL_DEVICE);    
    request.AddInt32("hci_id", hid);
       
    return RequestLocalDeviceID(&request);

}


LocalDevice*
LocalDevice::GetLocalDevice(bdaddr_t bdaddr)
{
    if (SRetrieveBluetoothMessenger() != B_OK)
        return NULL;
 
    BMessage request(BT_MSG_ACQUIRE_LOCAL_DEVICE);    
    request.AddData("bdaddr", B_ANY_TYPE, &bdaddr, sizeof(bdaddr_t));
    
       
    return RequestLocalDeviceID(&request);
}


uint32
LocalDevice::GetLocalDeviceCount()
{
    if (SRetrieveBluetoothMessenger() != B_OK)
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

	return NULL;
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
    if (RetrieveBluetoothMessenger() != B_OK)
        return B_ERROR;            


    BMessage request(BT_MSG_HANDLE_SIMPLE_REQUEST);
    BMessage reply;
	int8	 bt_status = BT_ERROR;
	
    
    request.AddInt32("hci_id", hid);
	
	// TODO: Add the raw command to the BMessage + expected event

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
    if (RetrieveBluetoothMessenger() != B_OK)
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
    if (RetrieveBluetoothMessenger() != B_OK)
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
