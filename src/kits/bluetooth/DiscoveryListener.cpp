/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <bluetooth/DiscoveryListener.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/DeviceClass.h>

#include <bluetoothserver_p.h>

#include <Message.h>

namespace Bluetooth {


/* hooks */
void 
DiscoveryListener::DeviceDiscovered(RemoteDevice btDevice, DeviceClass cod)
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
DiscoveryListener::DiscoveryListener()
{

}


void 
DiscoveryListener::MessageReceived(BMessage* message)
{
    switch (message->what) 
    {
        case BT_MSG_INQUIRY_DEVICE:
            
            /* TODO: Extract info from BMessage to create a 
               proper RemoteDevice, message should be passed from Agent??? */
               
            /* - Instance to be stored/Registered in the Agent? */            
            //DeviceDiscovered( RemoteDevice(BString("00:00:00:00:00:00")), DeviceClass(0));

        break;
        
        case BT_MSG_INQUIRY_COMPLETED:

            InquiryCompleted(B_BT_INQUIRY_COMPLETED);

        break;
        case BT_MSG_INQUIRY_TERMINATED:

            InquiryCompleted(B_BT_INQUIRY_TERMINATED);

        break;
        case BT_MSG_INQUIRY_ERROR:

            InquiryCompleted(B_BT_INQUIRY_ERROR);            

        break;
        
        default:

            BLooper::MessageReceived(message);

        break;
        
    }        
    
}

}
