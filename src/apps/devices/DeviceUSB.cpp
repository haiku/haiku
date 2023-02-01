/*
 * Copyright 2008-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pieter Panman
 */


#include "DeviceUSB.h"

#include <sstream>
#include <stdlib.h>

#include <Catalog.h>
#include <bus/USB.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceUSB"

extern "C" {
#include "dm_wrapper.h"
#include "usb-utils.h"
}


DeviceUSB::DeviceUSB(Device* parent)
	:
	Device(parent),
	fVendorId(0),
	fDeviceId(0)
{
}


DeviceUSB::~DeviceUSB()
{
}


static BString ToHex(uint16 num)
{
	std::stringstream ss;
	ss.flags(std::ios::hex | std::ios::showbase);
	ss << num;
	return BString(ss.str().c_str());
}


void
DeviceUSB::InitFromAttributes()
{
	// Process the attributes
	fClassBaseId = atoi(fAttributeMap[USB_DEVICE_CLASS].String());
	fClassSubId = atoi(fAttributeMap[USB_DEVICE_SUBCLASS].String());
	fClassProtoId = atoi(fAttributeMap[USB_DEVICE_PROTOCOL].String());
	fVendorId = atoi(fAttributeMap[B_DEVICE_VENDOR_ID].String());
	fDeviceId = atoi(fAttributeMap[B_DEVICE_ID].String());

	// Looks better in Hex, so rewrite
	fAttributeMap[USB_DEVICE_CLASS] = ToHex(fClassBaseId);
	fAttributeMap[USB_DEVICE_SUBCLASS] = ToHex(fClassSubId);
	fAttributeMap[USB_DEVICE_PROTOCOL] = ToHex(fClassProtoId);
	fAttributeMap[B_DEVICE_VENDOR_ID] = ToHex(fVendorId);
	fAttributeMap[B_DEVICE_ID] = ToHex(fDeviceId);

	// Fetch ClassInfo
	char classInfo[128];
	usb_get_class_info(fClassBaseId, fClassSubId, fClassProtoId, classInfo,
		sizeof(classInfo));

	// Fetch ManufacturerName
	const char* vendorName = NULL;
	const char* deviceName = NULL;
	BString deviceLabel;
	BString manufacturerLabel;
	usb_get_vendor_info(fVendorId, &vendorName);
	usb_get_device_info(fVendorId, fDeviceId, &deviceName);
	if (vendorName == NULL) {
		manufacturerLabel << B_TRANSLATE("Unknown");
	} else {
		manufacturerLabel << vendorName;
	};

	// Fetch DeviceName
	if (deviceName == NULL) {
		deviceLabel << B_TRANSLATE("Unknown");
	} else {
		deviceLabel << deviceName;
	}

	SetAttribute(B_TRANSLATE("Device name"), deviceLabel);
	SetAttribute(B_TRANSLATE("Manufacturer"), manufacturerLabel);
#if 0
	// These are a source of confusion for users, leading them to think there
	// is no driver for their device. Until we can display something useful,
	// let's not show the lines at all.
	SetAttribute(B_TRANSLATE("Driver used"), B_TRANSLATE("Not implemented"));
	SetAttribute(B_TRANSLATE("Device paths"), B_TRANSLATE("Not implemented"));
#endif
	SetAttribute(B_TRANSLATE("Class info"), classInfo);
	switch (fClassBaseId) {
		case 0x1:
			fCategory = CAT_MULTIMEDIA; break;
		case 0x2:
			fCategory = CAT_COMM; break;
		case 0x3:
			fCategory = CAT_INPUT; break;
		case 0x6:	// Imaging
			fCategory = CAT_MULTIMEDIA; break;
		case 0x7:	// Printer
			fCategory = CAT_MULTIMEDIA; break;
		case 0x8:
			fCategory = CAT_MASS; break;
		case 0x9:
			fCategory = CAT_GENERIC; break;
		case 0xa:
			fCategory = CAT_COMM; break;
		case 0xe:	// Webcam
			fCategory = CAT_MULTIMEDIA; break;
		case 0xe0:
			fCategory = CAT_WIRELESS; break;
	}
	BString outlineName;
	outlineName << manufacturerLabel << " " << deviceLabel;
	SetText(outlineName.String());
}
