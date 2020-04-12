/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <bluetooth/bluetooth.h>
#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/debug.h>

#include <bluetoothserver_p.h>

#include "KitSupport.h"


BMessenger*
_RetrieveBluetoothMessenger(void)
{
	CALLED();
	// Fix/review: leaking memory here
	BMessenger* fMessenger = new BMessenger(BLUETOOTH_SIGNATURE);

	if (fMessenger == NULL || !fMessenger->IsValid()) {
		delete fMessenger;
		return NULL;
	} else
		return fMessenger;
}


uint8
GetInquiryTime()
{
	CALLED();
	return BT_DEFAULT_INQUIRY_TIME;
}


void
SetInquiryTime(uint8 time)
{
	CALLED();
	((void)(time));
}
