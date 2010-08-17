/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */
#include <bluetooth/bluetooth.h>
#include <bluetooth/DiscoveryAgent.h>

#include <bluetoothserver_p.h>

#include "KitSupport.h"


namespace BPrivate {

const bdaddr_t kBdNullAddress 		= {{0, 0, 0, 0, 0, 0}};
const bdaddr_t kBdLocalAddress 		= {{0, 0, 0, 0xff, 0xff, 0xff}};
const bdaddr_t kBdBroadcastAddress	= {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};

}


BMessenger* _RetrieveBluetoothMessenger(void)
{
	// Fix/review: leaking memory here
	BMessenger* fMessenger = new BMessenger(BLUETOOTH_SIGNATURE);

	if (fMessenger == NULL || !fMessenger->IsValid()) {
		delete fMessenger;
		return NULL;
	} else
		return fMessenger;
}


uint8 GetInquiryTime()
{
	return BT_DEFAULT_INQUIRY_TIME;
}


void SetInquiryTime(uint8 time)
{
	((void)(time));
}

