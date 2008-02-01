/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <bluetooth/LocalDevice.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/DeviceClass.h>
#include <bluetooth/DiscoveryAgent.h>

#include <bluetooth/bdaddrUtils.h>

#include "bluetoothserver_p.h"

namespace Bluetooth {

BMessenger*  LocalDevice::sfMessenger = NULL;

status_t
LocalDevice::SRetrieveBluetoothMessenger(void)
{
    if (sfMessenger == NULL || !sfMessenger->IsValid() )
        sfMessenger = new BMessenger();

    return (sfMessenger != NULL)?B_OK:B_ERROR;
}

status_t LocalDevice::RetrieveBluetoothMessenger(void)
{
    if (fMessenger == NULL || !fMessenger->IsValid())
        fMessenger = new BMessenger();

    return (fMessenger != NULL)?B_OK:B_ERROR;
}

LocalDevice*
LocalDevice::RequestLocalDeviceID(BMessage* request)
{    
    BMessage reply;
    hci_id hid;
    
    if (sfMessenger->SendMessage(request, &reply) == B_OK &&
        reply.FindInt32("hci_id", &hid) == B_OK ){		
	    
	    if (hid > 0)	        
		    return new LocalDevice(hid);
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
 
    BMessage request(BT_MSG_ADQUIRE_LOCAL_DEVICE);
        
    return RequestLocalDeviceID(&request);
}


LocalDevice*
LocalDevice::GetLocalDevice(hci_id hid)
{
    if (SRetrieveBluetoothMessenger() != B_OK)
        return NULL;
 
    BMessage request(BT_MSG_ADQUIRE_LOCAL_DEVICE);    
    request.AddInt32("hci_id", hid);
       
    return RequestLocalDeviceID(&request);

}


LocalDevice*
LocalDevice::GetLocalDevice(bdaddr_t bdaddr)
{
    if (SRetrieveBluetoothMessenger() != B_OK)
        return NULL;
 
    BMessage request(BT_MSG_ADQUIRE_LOCAL_DEVICE);    
    request.AddData("bdaddr", B_ANY_TYPE, &bdaddr, sizeof(bdaddr_t));
    
       
    return RequestLocalDeviceID(&request);
}


uint32
LocalDevice::GetLocalDeviceCount()
{
    if (SRetrieveBluetoothMessenger() != B_OK)
        return NULL;

	BMessage request(BT_MSG_COUNT_LOCAL_DEVICES);	
	BMessage reply;

	if (sfMessenger->SendMessage(&request, &reply) == B_OK)
		return reply.FindInt32("count");
    else
        return B_ERROR;
		
}


DiscoveryAgent*
LocalDevice::GetDiscoveryAgent()
{

	return NULL;
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

	return NULL;
}


bool 
LocalDevice::SetDiscoverable(int mode)
{

	return false;
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


bdaddr_t 
LocalDevice::GetBluetoothAddress()
{
    if (RetrieveBluetoothMessenger() != B_OK)
        return bdaddrUtils::NullAddress();            
 
    bdaddr_t bdaddr;
    BMessage request(BT_MSG_GET_ADDRESS);
    BMessage reply;
 	ssize_t	size;   
    /* ADD ID */
    request.AddInt32("hci_id", hid);
    
    if (fMessenger->SendMessage(&request, &reply) == B_OK &&
        reply.FindData("bdaddr", B_ANY_TYPE, 0, (const void**)&bdaddr, &size) == B_OK ){		
	    
	    return bdaddr;
    }
    
	return bdaddrUtils::NullAddress();
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
