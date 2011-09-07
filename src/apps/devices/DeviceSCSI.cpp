/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "DeviceSCSI.h"

#include <scsi.h>
#include <sstream>
#include <stdlib.h>

#include <Catalog.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "DeviceSCSI"


// standard SCSI device types
const char* SCSITypeMap[] = {
	B_TRANSLATE("Disk Drive"),			// 0x00
	B_TRANSLATE("Tape Drive"),			// 0x01
	B_TRANSLATE("Printer"),				// 0x02
	B_TRANSLATE("Processor"),			// 0x03
	B_TRANSLATE("Worm"),				// 0x04
	B_TRANSLATE("CD-ROM"),				// 0x05
	B_TRANSLATE("Scanner"),				// 0x06
	B_TRANSLATE("Optical Drive"),		// 0x07
	B_TRANSLATE("Changer"),				// 0x08
	B_TRANSLATE("Communications"),		// 0x09
	B_TRANSLATE("Graphics Peripheral"),	// 0x0A
	B_TRANSLATE("Graphics Peripheral"),	// 0x0B
	B_TRANSLATE("Array"),				// 0x0C
	B_TRANSLATE("Enclosure"),			// 0x0D
	B_TRANSLATE("RBC"),					// 0x0E
	B_TRANSLATE("Card Reader"),			// 0x0F
	B_TRANSLATE("Bridge"),				// 0x10
	B_TRANSLATE("Other")				// 0x11
};


DeviceSCSI::DeviceSCSI(Device* parent)
	:
	Device(parent)
{
}


DeviceSCSI::~DeviceSCSI()
{
}


void
DeviceSCSI::InitFromAttributes()
{
	BString nodeVendor(GetAttribute("scsi/vendor").fValue);
	BString nodeProduct(GetAttribute("scsi/product").fValue);

	fCategory = (Category)CAT_MASS;

	uint32 nodeTypeID = atoi(GetAttribute("scsi/type").fValue);

	SetAttribute(B_TRANSLATE("Device name"), nodeProduct.String());
	SetAttribute(B_TRANSLATE("Manufacturer"), nodeVendor.String());
	SetAttribute(B_TRANSLATE("Device class"), SCSITypeMap[nodeTypeID]);

	BString listName;
	listName
		<< "SCSI " << SCSITypeMap[nodeTypeID] << " (" << nodeProduct << ")";

	SetText(listName.String());
}


Attributes
DeviceSCSI::GetBusAttributes()
{
	// Push back things that matter for SCSI devices
	Attributes attributes;
	attributes.push_back(GetAttribute(B_TRANSLATE("Device class")));
	attributes.push_back(GetAttribute(B_TRANSLATE("Device name")));
	attributes.push_back(GetAttribute(B_TRANSLATE("Manufacturer")));
	attributes.push_back(GetAttribute("scsi/revision"));
	attributes.push_back(GetAttribute("scsi/target_id"));
	attributes.push_back(GetAttribute("scsi/target_lun"));
	return attributes;
}


BString
DeviceSCSI::GetBusStrings()
{
	BString str(B_TRANSLATE("Class Info:\t\t\t\t: %classInfo%"));
	str.ReplaceFirst("%classInfo%", fAttributeMap["Class Info"]);

	return str;
}


BString
DeviceSCSI::GetBusTabName()
{
	return B_TRANSLATE("SCSI Information");
}

