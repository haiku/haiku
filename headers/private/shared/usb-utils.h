/*
 * Copyright 2008-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	Jérôme Duval
 */


#include <stdio.h>

#include "usbhdr.h"


static void
usb_get_class_info(uint8 usb_class_base_id, uint8 usb_class_sub_id, uint8 usb_class_proto_id,
	char *classInfo, size_t size)
{
	USB_CLASSCODETABLE *foundItem = NULL;
	int i;
	for (i = 0; i < (int)USB_CLASSCODETABLE_LEN; i++) {
		if ((usb_class_base_id == UsbClassCodeTable[i].BaseClass)
			&& (usb_class_sub_id == UsbClassCodeTable[i].SubClass)) {
			foundItem = &UsbClassCodeTable[i];
			if (usb_class_proto_id == UsbClassCodeTable[i].Protocol)
				break;
		}
	}
	if (foundItem) {
		snprintf(classInfo, size, "%s (%s%s%s)", foundItem->BaseDesc, foundItem->SubDesc,
			(foundItem->ProtocolDesc && strcmp("", foundItem->ProtocolDesc)) ? ", " : "",
			foundItem->ProtocolDesc);
	} else
		snprintf(classInfo, size, "%s (%u:%u:%u)", "(Unknown)",
			usb_class_base_id, usb_class_sub_id, usb_class_proto_id);
}


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
