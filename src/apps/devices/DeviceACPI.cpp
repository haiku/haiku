/*
 * Copyright 2008-2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rob Gill <rrobgill@protonmail.com>
 *		Alexander von Gluck (kallisti5)
 */


#include "DeviceACPI.h"

#include <sstream>
#include <stdlib.h>

#include <Catalog.h>

extern "C" {
#include "acpipnpids.h"
}

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceACPI"


void
acpi_get_vendor_info(const char* vendorID, const char **vendorName)
{
	for (size_t i = 0; i < ACPIPNP_DEVTABLE_LEN; i++) {
		if (strncmp(acpipnp_devids[i].VenId, vendorID, strlen(acpipnp_devids[i].VenId)) == 0) {
			*vendorName = acpipnp_devids[i].VenName;
			return;
		}
	}
	*vendorName = NULL;
}


DeviceACPI::DeviceACPI(Device* parent)
	:
	Device(parent)
{
}


DeviceACPI::~DeviceACPI()
{
}


void
DeviceACPI::InitFromAttributes()
{
	BString nodeACPIPath;
	BString rootACPIPath;
	BString nodeACPIHid;
	BString deviceName;

	rootACPIPath = nodeACPIPath = GetAttribute("acpi/path").fValue;
	nodeACPIHid = GetAttribute("acpi/hid").fValue;

	// Grab just the root node info
	// We grab 6 characters to not identify sub nodes of root node
	rootACPIPath.Truncate(6);
	// Grab node leaf name
	nodeACPIPath.Remove(0, nodeACPIPath.FindLast(".") + 1);

	fCategory = (Category)CAT_ACPI;

	// Identify Predefined root namespaces (ACPI Spec 4.0a, p162)
	if (rootACPIPath == "\\_SB_") {
		deviceName = B_TRANSLATE("ACPI System Bus");
	} else if (rootACPIPath == "\\_TZ_") {
		deviceName = B_TRANSLATE("ACPI Thermal Zone");
	} else if (rootACPIPath == "\\_PR_.") {
		// This allows to localize apostrophes, too
		BString string(B_TRANSLATE("ACPI Processor Namespace '%2'"));
		string.ReplaceFirst("%2", nodeACPIPath);
		// each CPU node is considered a root node
		deviceName << string.String();
	} else if (rootACPIPath == "\\_SI_") {
		deviceName = B_TRANSLATE("ACPI System Indicator");
	} else if (nodeACPIPath != "") {
		// This allows to localize apostrophes, too
		BString string(B_TRANSLATE("ACPI node '%1'"));
		string.ReplaceFirst("%1", nodeACPIPath);
		deviceName << string.String();
	} else if (nodeACPIPath == "" && nodeACPIHid != "") {
		// Handle ACPI HID entries that do not return a path
		nodeACPIHid.Remove(0, nodeACPIHid.FindLast("_") + 1);
		BString string(B_TRANSLATE("ACPI Button '%1'")); 
		string.ReplaceFirst("%1", nodeACPIHid); 
		deviceName << string.String();
	} else {
		BString string(B_TRANSLATE("ACPI <unknown>"));
		deviceName << string.String();
	}

	// Fetch ManufacturerName
	const char* vendorName = NULL;
	BString manufacturerLabel;
	if (nodeACPIHid != "")
		acpi_get_vendor_info(nodeACPIHid, &vendorName);
	if (vendorName == NULL) {
		manufacturerLabel << B_TRANSLATE("Unknown");
	} else {
		manufacturerLabel << vendorName;
	};

	SetAttribute(B_TRANSLATE("Device name"), deviceName.String());
	SetAttribute(B_TRANSLATE("Manufacturer"), manufacturerLabel);
	BString outlineName;
	if (vendorName != NULL)
		outlineName << vendorName << " ";
	outlineName << deviceName;
	SetText(outlineName.String());
}
