/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _DISCOVERY_AGENT_H
#define _DISCOVERY_AGENT_H

#include <bluetooth/bluetooth.h>

#define B_BT_CACHED 0x00
#define B_BT_PREKNOWN 0x01
#define B_BT_NOT_DISCOVERABLE 0x01

#define B_BT_GIAC 0x9E8B33
#define B_BT_LIAC 0x9E8B00

namespace Bluetooth {

class DiscoveryListener;
class RemoteDevice;

class DiscoveryAgent {

    public:
                
        static const int GIAC = B_BT_GIAC;
        static const int LIAC = B_BT_LIAC;
        
        static const int PREKNOWN = B_BT_PREKNOWN;
        static const int CACHED = B_BT_CACHED;
        static const int NOT_DISCOVERABLE = B_BT_NOT_DISCOVERABLE;
        
        RemoteDevice** RetrieveDevices(int option); /* TODO */
        bool StartInquiry(int accessCode, DiscoveryListener listener); /* Throwing */
        bool CancelInquiry(DiscoveryListener listener);
        
        /*
        int searchServices(int[] attrSet,
                               UUID[] uuidSet,
                               RemoteDevice btDev,
                               DiscoveryListener discListener);
                               
        bool cancelServiceSearch(int transID);                               
        BString selectService(UUID uuid, int security, boolean master);
        */
        
    private:
        DiscoveryAgent();
        
};

}

#endif
