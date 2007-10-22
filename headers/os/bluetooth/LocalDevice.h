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

#include <String.h>

namespace Bluetooth {

class DiscoveryAgent;

class LocalDevice {
   
    public:
        /* Possible throwing */
        static   LocalDevice* GetLocalDevice();
        static   LocalDevice* GetLocalDevice(uint8 dev);
        static   LocalDevice* GetLocalDevice(bdaddr_t bdaddr);
        
                 DiscoveryAgent* GetDiscoveryAgent();
                 BString GetFriendlyName();
                 DeviceClass GetDeviceClass();
        /* Possible throwing */
                 bool SetDiscoverable(int mode);
                 
                 BString GetProperty(const char* property);                     
                 void GetProperty(const char* property, uint32* value);
                 
                 int GetDiscoverable();
                 BString GetBluetoothAddress();
				 /*                     
                 ServiceRecord getRecord(Connection notifier);
                 void updateRecord(ServiceRecord srvRecord);
				 */                 
    private:
        LocalDevice();
};
    
}

#endif
