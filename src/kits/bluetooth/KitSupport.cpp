/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#include <bluetooth/bluetooth.h>
#include <bluetoothserver_p.h>

#include "KitSupport.h"


BMessenger* _RetrieveBluetoothMessenger(void)
{
	BMessenger* fMessenger = new BMessenger(BLUETOOTH_SIGNATURE);

    if (fMessenger == NULL || !fMessenger->IsValid())
    	return NULL;
    else
    	return fMessenger;
}
