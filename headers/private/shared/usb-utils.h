/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	Jérôme Duval
 */

#include <stdio.h>

#include "usbdevs.h"
#include "usbdevs_data.h"

void
usb_get_vendor_info(uint16 vendorID, const char **vendorName)
{
	int i;
	for (i = 0; usb_knowndevs[i].vendor != 0; i++) {
		if (usb_knowndevs[i].vendor == vendorID) {
			*vendorName = usb_knowndevs[i].vendorname;
			return;
		}
	}
	*vendorName = NULL;
}


void
usb_get_vendor_device_info(uint16 vendorID, uint16 deviceID, const char **vendorName, const char **deviceName)
{
	int i;
	for (i = 0; usb_knowndevs[i].vendor != 0; i++) {
		if (usb_knowndevs[i].vendor == vendorID
			&& usb_knowndevs[i].product == deviceID
			&& usb_knowndevs[i].flags != USB_KNOWNDEV_NOPROD) {
			*deviceName = usb_knowndevs[i].productname;
			*vendorName = usb_knowndevs[i].vendorname;
			return;
		}
	}
	*vendorName = NULL;
	*deviceName = NULL;
}
