/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _LOCAL_DEVICE_H
#define _LOCAL_DEVICE_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/DeviceClass.h>

#include <bluetooth/HCI/btHCI.h>

#include <Messenger.h>
#include <Message.h>

#include <String.h>


namespace Bluetooth {

class DiscoveryAgent;

class LocalDevice {
   
    public:
        /* Possible throwing */
        static   LocalDevice* GetLocalDevice();
        static   uint32       GetLocalDeviceCount();
        
        static   LocalDevice* GetLocalDevice(hci_id hid);
        static   LocalDevice* GetLocalDevice(bdaddr_t bdaddr);
        
                 DiscoveryAgent* GetDiscoveryAgent();
                 BString GetFriendlyName();
                 DeviceClass GetDeviceClass();
        /* Possible throwing */
                 status_t SetDiscoverable(int mode);
                 
                 BString GetProperty(const char* property);                     
                 void GetProperty(const char* property, uint32* value);
                 
                 int GetDiscoverable();
                 bdaddr_t GetBluetoothAddress();
		 /*                     
                 ServiceRecord getRecord(Connection notifier);
                 void updateRecord(ServiceRecord srvRecord);
		 */                 
    private:
        LocalDevice(hci_id hid);
        
        static   LocalDevice*   RequestLocalDeviceID(BMessage* request);
        
        static BMessenger*      sfMessenger;
               BMessenger*      fMessenger;

		hci_id		hid;
};
    
}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::LocalDevice;
#endif

#endif
