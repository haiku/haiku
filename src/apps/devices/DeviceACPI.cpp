/*
 * Copyright 2008-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck (kallisti5)
 */


#include "DeviceACPI.h"

#include <sstream>
#include <stdlib.h>

#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceACPI"


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
	BString outlineName;
	BString nodeACPIPath;
	BString rootACPIPath;

	rootACPIPath = nodeACPIPath = GetAttribute("acpi/path").fValue;

	// Grab just the root node info
	// We grab 6 characters to not identify sub nodes of root node
	rootACPIPath.Truncate(6);
	// Grab node leaf name
	nodeACPIPath.Remove(0, nodeACPIPath.FindLast(".") + 1);

	fCategory = (Category)CAT_ACPI;

	// Identify Predefined root namespaces (ACPI Spec 4.0a, p162)
	if (rootACPIPath == "\\_SB_") {
		outlineName = B_TRANSLATE("ACPI System Bus");
	} else if (rootACPIPath == "\\_TZ_") {
		outlineName = B_TRANSLATE("ACPI Thermal Zone");
	} else if (rootACPIPath == "\\_PR_.") {
		// This allows to localize apostrophes, too
		BString string(B_TRANSLATE("ACPI Processor Namespace '%2'"));
		string.ReplaceFirst("%2", nodeACPIPath);
		// each CPU node is considered a root node
		outlineName << string.String();
	} else if (rootACPIPath == "\\_SI_") {
		outlineName = B_TRANSLATE("ACPI System Indicator");
	} else {
		// This allows to localize apostrophes, too
		BString string(B_TRANSLATE("ACPI node '%1'"));
		string.ReplaceFirst("%1", nodeACPIPath);
		outlineName << string.String();
	}

	SetAttribute(B_TRANSLATE("Device name"), outlineName.String());
	SetAttribute(B_TRANSLATE("Manufacturer"), B_TRANSLATE("Not implemented"));

	SetText(outlineName.String());
}


Attributes
DeviceACPI::GetBusAttributes()
{
	// Push back things that matter for ACPI
	Attributes attributes;
	attributes.push_back(GetAttribute("device/bus"));
	attributes.push_back(GetAttribute("acpi/path"));
	attributes.push_back(GetAttribute("acpi/type"));
	return attributes;
}


BString
DeviceACPI::GetBusStrings()
{
	BString str(B_TRANSLATE("Class Info:\t\t\t\t: %classInfo%"));
	str.ReplaceFirst("%classInfo%", fAttributeMap["Class Info"]);
	
	return str;
}
	

BString
DeviceACPI::GetBusTabName()
{
	return B_TRANSLATE("ACPI Information");
}

