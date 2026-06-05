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
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <USBKit.h>
#include <bus/USB.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceUSB"

extern "C" {
#include "dm_wrapper.h"
#include "usb-utils.h"
}


#define USB_SELF_POWERED	0x40
#define USB_REMOTE_WAKEUP	0x20


static uint8
MajorVersion(uint16 version)
{
	return version >> 8;
}


static uint8
MinorVersion(uint16 version)
{
	return version & 0xff;
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


static BString
FindDevicePath(const char* currentPath, uint16 vendor, uint16 product)
{
	BDirectory dir(currentPath);
	if (dir.InitCheck() != B_OK)
		return "";

	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath pathObj;
		entry.GetPath(&pathObj);

		if (BString(pathObj.Leaf()) == "hub")
			continue;

		if (entry.IsDirectory()) {
			BString result = FindDevicePath(pathObj.Path(), vendor, product);
			if (result != "")
				return result;
		} else {
			BUSBDevice testDevice(pathObj.Path());
			if (testDevice.InitCheck() == B_OK && testDevice.VendorID() == vendor
				&& testDevice.ProductID() == product) {

				return BString(pathObj.Path());
			}
		}
	}

	return "";
}


static void
BuildUSBTree(BUSBDevice& device, DeviceUSB* node)
{
	node->SetAttribute("usb/path", device.Location());
	if (device.ManufacturerString())
		node->SetAttribute("usb/manufacturer", device.ManufacturerString());
	if (device.ProductString())
		node->SetAttribute("usb/product", device.ProductString());
	if (device.SerialNumberString())
		node->SetAttribute("usb/serial_number", device.SerialNumberString());

	BString usbVersion;
	usbVersion.SetToFormat(B_TRANSLATE("%d.%d"),
		MajorVersion(device.USBVersion()),
		MinorVersion(device.USBVersion()));
	node->SetAttribute("usb/version", usbVersion);

	for (uint32 i = 0; i < device.CountConfigurations(); i++) {
		const BUSBConfiguration* config = device.ConfigurationAt(i);
		if (!config)
			continue;

		BString confPath;
		confPath << "usb/config/" << i;

		BString powerValue;
		powerValue.SetToFormat(B_TRANSLATE("%d mA"), config->Descriptor()->max_power * 2);
		node->SetAttribute(BString(confPath) << "/max_power", powerValue);

		uint8 attr = config->Descriptor()->attributes;
		if (attr & USB_SELF_POWERED)
			node->SetAttribute(BString(confPath) << "/self_powered", "true");
		if (attr & USB_REMOTE_WAKEUP)
			node->SetAttribute(BString(confPath) << "/remote_wakeup", "true");

		if (config->ConfigurationString())
			node->SetAttribute(BString(confPath) << "/name", config->ConfigurationString());

		for (uint32 j = 0; j < config->CountInterfaces(); j++) {
			const BUSBInterface* interface = config->InterfaceAt(j);
			if (!interface)
				continue;

			BString intfPath;
			intfPath << confPath << "/interface/" << j;

			for (uint32 k = 0; k < interface->CountAlternates(); k++) {
				const BUSBInterface* alt = interface->AlternateAt(k);
				if (!alt)
					continue;

				BString altPath;
				altPath << intfPath << "/alt/" << k;

				if (k == interface->AlternateIndex())
					node->SetAttribute(BString(altPath) << "/active", "true");

				char classInfo[128];
				usb_get_class_info(alt->Class(), alt->Subclass(), alt->Protocol(), classInfo,
					sizeof(classInfo));

				BString classVal;
				classVal.SetToFormat(B_TRANSLATE("0x%02x, Sub: 0x%02x (%s)"), alt->Class(),
					alt->Subclass(), classInfo);

				node->SetAttribute(BString(altPath) << "/class_info", classVal);

				if (alt->InterfaceString())
					node->SetAttribute(BString(altPath) << "/name", alt->InterfaceString());

				for (uint32 e = 0; e < alt->CountEndpoints(); e++) {
					const BUSBEndpoint* endpoint = alt->EndpointAt(e);
					if (!endpoint)
						continue;

					BString endpointPath;
					endpointPath << altPath << "/endpoint/" << e;

					const usb_endpoint_descriptor* endpointDesc = endpoint->Descriptor();

					BString endpointAddress;
					endpointAddress.SetToFormat(B_TRANSLATE("0x%02x (%s)"),
						endpointDesc->endpoint_address,
						(endpointDesc->endpoint_address & USB_ENDPOINT_ADDR_DIR_MASK)
								== USB_ENDPOINT_ADDR_DIR_IN
							? B_TRANSLATE("IN")
							: B_TRANSLATE("OUT"));
					node->SetAttribute(BString(endpointPath) << "/address", endpointAddress);

					const char* type = B_TRANSLATE("Unknown");
					switch (endpointDesc->attributes & USB_ENDPOINT_ATTR_MASK) {
						case USB_ENDPOINT_ATTR_CONTROL:
							type = B_TRANSLATE("Control");
							break;
						case USB_ENDPOINT_ATTR_ISOCHRONOUS:
							type = B_TRANSLATE("Isochronous");
							break;
						case USB_ENDPOINT_ATTR_BULK:
							type = B_TRANSLATE("Bulk");
							break;
						case USB_ENDPOINT_ATTR_INTERRUPT:
							type = B_TRANSLATE("Interrupt");
							break;
					}
					node->SetAttribute(BString(endpointPath) << "/type", type);

					BString maxPacket;
					maxPacket << endpointDesc->max_packet_size;
					node->SetAttribute(BString(endpointPath) << "/max_packet_size", maxPacket);

					if ((endpointDesc->attributes & USB_ENDPOINT_ATTR_MASK)
						== USB_ENDPOINT_ATTR_INTERRUPT) {
						BString interval;
						interval.SetToFormat(B_TRANSLATE("%u ms"), endpointDesc->interval);
						node->SetAttribute(BString(endpointPath) << "/interval", interval);
					}
				}
			}
		}
	}
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
		manufacturerLabel << B_TRANSLATE("unknown");
	} else {
		manufacturerLabel << vendorName;
	};

	// Fetch DeviceName
	if (deviceName == NULL) {
		deviceLabel << B_TRANSLATE("unknown");
	} else {
		deviceLabel << deviceName;
	}

	SetAttribute(B_TRANSLATE("Device name"), deviceLabel);
	SetAttribute(B_TRANSLATE("Manufacturer"), manufacturerLabel);
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

	// Fetch USB attributes
	BString path = FindDevicePath("/dev/bus/usb/", fVendorId, fDeviceId);

	if (path.Length() > 0) {
		BUSBDevice device(path.String());
		if (device.InitCheck() == B_OK)
			BuildUSBTree(device, this);
	}
}
