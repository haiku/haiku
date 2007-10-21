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
            static   LocalDevice* getLocalDevice();
            static   LocalDevice* getLocalDevice(uint8 dev);
            static   LocalDevice* getLocalDevice(bdaddr_t bdaddr);
            
                     DiscoveryAgent* getDiscoveryAgent();
                     BString getFriendlyName();
                     DeviceClass getDeviceClass();
            /* Possible throwing */
                     bool setDiscoverable(int mode);
                     
                     BString getProperty(const char* property);                     
                     void getProperty(const char* property, uint32* value);
                     
                     int getDiscoverable();
                     BString getBluetoothAddress();
/*                     
                     ServiceRecord getRecord(Connection notifier);
                     void updateRecord(ServiceRecord srvRecord);
*/                 
        private:
            LocalDevice();
    };
    
}

#endif
