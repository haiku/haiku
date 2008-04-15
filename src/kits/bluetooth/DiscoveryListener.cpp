/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <bluetooth/DiscoveryListener.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/DeviceClass.h>

#include <bluetooth/HCI/btHCI_event.h>

#include <bluetoothserver_p.h>

#include <Message.h>

namespace Bluetooth {


/* hooks */
void 
DiscoveryListener::DeviceDiscovered(RemoteDevice* btDevice, DeviceClass cod)
{    

}


void 
DiscoveryListener::InquiryStarted(status_t status)
{

}


void 
DiscoveryListener::InquiryCompleted(int discType)
{

}


/* private */            

/* A LocalDevice is always referenced in any request to the
   Bluetooth server therefore is going to be needed in any */
void
DiscoveryListener::SetLocalDeviceOwner(LocalDevice* ld)
{
    fLocalDevice = ld;
}


RemoteDevicesList
DiscoveryListener::GetRemoteDevicesList(void)
{
	return fRemoteDevicesList;
}


void
DiscoveryListener::MessageReceived(BMessage* message)
{
    int8 status;

    switch (message->what)
    {
        case BT_MSG_INQUIRY_DEVICE:
        {
			const struct inquiry_info* inquiryInfo;
		 	ssize_t	size;   

			if (message->FindData("info", B_ANY_TYPE, 0, (const void**)&inquiryInfo, &size) == B_OK )
			{			
	            RemoteDevice* rd = new RemoteDevice(inquiryInfo->bdaddr);
	            //  DeviceClass(inquiryInfo->dev_class[0] | inquiryInfo->dev_class[1]<<8 | inquiryInfo->dev_class[2]<<16 )
				//  fRemoteDevicesList.AddItem(rd);
	            rd->SetLocalDeviceOwner(fLocalDevice);	      
	            DeviceDiscovered( rd, DeviceClass(inquiryInfo->dev_class[0] | inquiryInfo->dev_class[1]<<8 | inquiryInfo->dev_class[2]<<16 ));

  			}
        }
        break;

        case BT_MSG_INQUIRY_STARTED:
            if (message->FindInt8("status", &status) == B_OK){
	            InquiryStarted(status);
            }
            
        break;

        case BT_MSG_INQUIRY_COMPLETED:

            InquiryCompleted(BT_INQUIRY_COMPLETED);

        break;
        case BT_MSG_INQUIRY_TERMINATED: /* inquiry was cancelled */

            InquiryCompleted(BT_INQUIRY_TERMINATED);

        break;
        case BT_MSG_INQUIRY_ERROR:

            InquiryCompleted(BT_INQUIRY_ERROR);

        break;

        default:

            BLooper::MessageReceived(message);

        break;

    }

}


DiscoveryListener::DiscoveryListener() : BLooper()
{
	// TODO: Make a better handling of the running not running state
	Run();
}


}
