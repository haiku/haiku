/*
 * Copyright 2008-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	Jérôme Duval
 */


#include <stdio.h>

#include "usbhdr.h"


void
usb_get_vendor_info(uint16 vendorID, const char **vendorName)
{
	int i;
	for (i = 0; i < (int)USB_VENTABLE_LEN; i++) {
		if (UsbVenTable[i].VenId == vendorID) {
			*vendorName = UsbVenTable[i].VenName && UsbVenTable[i].VenName[0]
				? UsbVenTable[i].VenName : NULL;
			return;
		}
	}
	*vendorName = NULL;
}


void
usb_get_device_info(uint16 vendorID, uint16 deviceID, const char **deviceName)
{
	int i;
	// search for the device
	for (i = 0; i < (int)USB_DEVTABLE_LEN; i++) {
		if (UsbDevTable[i].VenId == vendorID && UsbDevTable[i].DevId == deviceID ) {
			*deviceName = UsbDevTable[i].ChipDesc && UsbDevTable[i].ChipDesc[0]
				? UsbDevTable[i].ChipDesc : NULL;
			return;
		}
	}
	*deviceName = NULL;
}
