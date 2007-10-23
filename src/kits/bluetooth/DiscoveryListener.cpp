/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <bluetooth/DiscoveryListener.h>
#include <bluetooth/RemoteDevice.h>
#include <bluetooth/DeviceClass.h>

#include <Message.h>

namespace Bluetooth {


/* Move to some private header */
#define B_BT_INQUIRY_COMPLETED_MSG  'IqCM'
#define B_BT_INQUIRY_TERMINATED_MSG 'IqTR'
#define B_BT_INQUIRY_ERROR_MSG      'IqER'
#define B_BT_INQUIRY_DEVICE_MSG     'IqDE'

/* hooks */

void 
DiscoveryListener::DeviceDiscovered(RemoteDevice btDevice, DeviceClass cod)
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
        case B_BT_INQUIRY_DEVICE_MSG:
            
            /* TODO: Extract info from BMessage to create a 
               proper RemoteDevice, ¿¿¿message should be passed from Agent??? */
               
            /* - Instance to be stored/Registered in the Agent? */            
            //DeviceDiscovered( RemoteDevice(BString("00:00:00:00:00:00")), DeviceClass(0));

        break;
        
        case B_BT_INQUIRY_COMPLETED_MSG:

            InquiryCompleted(B_BT_INQUIRY_COMPLETED);

        break;
        case B_BT_INQUIRY_TERMINATED_MSG:

            InquiryCompleted(B_BT_INQUIRY_TERMINATED);

        break;
        case B_BT_INQUIRY_ERROR_MSG:

            InquiryCompleted(B_BT_INQUIRY_ERROR);            

        break;
        
        default:

            BLooper::MessageReceived(message);

        break;
        
    }        
    
}

}
