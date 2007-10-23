/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _REMOTE_DEVICE_H
#define _REMOTE_DEVICE_H

#include <bluetooth/bluetooth.h>

#include <String.h>

#define B_BT_WAIT 0x00
#define B_BT_SUCCEEDED 0x01


namespace Bluetooth {

class Connection;

class RemoteDevice {

    public:
                
        static const int WAIT = B_BT_WAIT;
        static const int SUCCEEDED = B_BT_SUCCEEDED;
                
        bool IsTrustedDevice();
        BString GetFriendlyName(bool alwaysAsk); /* Throwing */
        BString GetBluetoothAddress();
        bool Equals(RemoteDevice* obj);
        
        static RemoteDevice* GetRemoteDevice(Connection conn);  /* Throwing */
        bool Authenticate(); /* Throwing */
        bool Authorize(Connection conn); /* Throwing */
        bool Encrypt(Connection conn, bool on); /* Throwing */
        bool IsAuthenticated(); /* Throwing */
        bool IsAuthorized(Connection conn); /* Throwing */
        bool IsEncrypted(); /* Throwing */
                
    protected:
        RemoteDevice(BString address);
    
    /* Instances of this class only will be done by Discovery[Listener|Agent] TODO */
    friend class DiscoveryListener;
        
};

}

#endif
