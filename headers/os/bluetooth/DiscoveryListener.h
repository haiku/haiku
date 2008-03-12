/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _DISCOVERY_LISTENER_H
#define _DISCOVERY_LISTENER_H

#include <Looper.h>


#define B_BT_INQUIRY_COMPLETED 0x01  // HCI_EVENT_X check specs
#define B_BT_INQUIRY_TERMINATED 0x02 // HCI_EVENT_X
#define B_BT_INQUIRY_ERROR 0x03      // HCI_EVENT_X


namespace Bluetooth {

class RemoteDevice;
class DeviceClass;


class DiscoveryListener : BLooper {

    public:

        static const int INQUIRY_COMPLETED = B_BT_INQUIRY_COMPLETED;
        static const int INQUIRY_TERMINATED = B_BT_INQUIRY_TERMINATED;
        static const int INQUIRY_ERROR = B_BT_INQUIRY_ERROR;
         
        static const int SERVICE_SEARCH_COMPLETED = 0x01;
        static const int SERVICE_SEARCH_TERMINATED = 0x02;
        static const int SERVICE_SEARCH_ERROR = 0x03;
        static const int SERVICE_SEARCH_NO_RECORDS = 0x04;
        static const int SERVICE_SEARCH_DEVICE_NOT_REACHABLE = 0x06;
        
        virtual void DeviceDiscovered(RemoteDevice btDevice, DeviceClass cod);
        /*
        virtual void servicesDiscovered(int transID, ServiceRecord[] servRecord);
        virtual void serviceSearchCompleted(int transID, int respCode);
        */
        virtual void InquiryCompleted(int discType);

		/* JSR82 non-defined methods */
		virtual void InquiryStarted(status_t status);
        
    private:                
        DiscoveryListener();
        void MessageReceived(BMessage* msg);
        
};

}

#endif
