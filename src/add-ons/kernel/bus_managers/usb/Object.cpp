/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "usb_p.h"


Object::Object(BusManager *bus)
	:	fBusManager(bus),
		fStack(bus->GetStack()),
		fUSBID(fStack->GetUSBID(this))
{
}
