/*
 * Copyright 2008-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pieter Panman
 */


#include "Device.h"

#include <iostream>

#include <Catalog.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Device"

// This list comes from the pciid list, except for the last one
const char* kCategoryString[] = {
	B_TRANSLATE("Unclassified device"), 				// 0x00
	B_TRANSLATE("Mass storage controller"),				// 0x01
	B_TRANSLATE("Network controller"),					// 0x02
	B_TRANSLATE("Display controller"), 					// 0x03
	B_TRANSLATE("Multimedia controller"), 				// 0x04
	B_TRANSLATE("Memory controller"),  					// 0x05
	B_TRANSLATE("Bridge"), 								// 0x06
	B_TRANSLATE("Communication controller"),  			// 0x07
	B_TRANSLATE("Generic system peripheral"),  			// 0x08
	B_TRANSLATE("Input device controller"),  			// 0x09
	B_TRANSLATE("Docking station"),  					// 0x0a
	B_TRANSLATE("Processor"),  							// 0x0b
	B_TRANSLATE("Serial bus controller"),  				// 0x0c
	B_TRANSLATE("Wireless controller"),  				// 0x0d
	B_TRANSLATE("Intelligent controller"),  			// 0x0e
	B_TRANSLATE("Satellite communications controller"), // 0x0f
	B_TRANSLATE("Encryption controller"),  				// 0x10
	B_TRANSLATE("Signal processing controller"),		// 0x11
	B_TRANSLATE("Computer"),							// 0x12 (added later)
	B_TRANSLATE("ACPI controller")						// 0x13 (added later)
};

// This list is only used to translate Device properties
static const char* kTranslateMarkString[] = {
	B_TRANSLATE_MARK("unknown"),
	B_TRANSLATE_MARK("Device"),
	B_TRANSLATE_MARK("Computer"),
	B_TRANSLATE_MARK("ACPI bus"),
	B_TRANSLATE_MARK("PCI bus"),
	B_TRANSLATE_MARK("ISA bus"),
	B_TRANSLATE_MARK("Unknown device")
};


Device::Device(Device* physicalParent, BusType busType, Category category,
			const BString& name, const BString& manufacturer,
			const BString& driverUsed, const BString& devPathsPublished)
	:
	BStringItem(name.String()),
	fBusType(busType),
	fCategory(category),
	fPhysicalParent(physicalParent)
{
	SetAttribute(B_TRANSLATE("Device name"), B_TRANSLATE(name));
	SetAttribute(B_TRANSLATE("Manufacturer"), B_TRANSLATE(manufacturer));
	SetAttribute(B_TRANSLATE("Driver used"), B_TRANSLATE(driverUsed));
	SetAttribute(B_TRANSLATE("Device paths"), B_TRANSLATE(devPathsPublished));
}


Device::~Device()
{
}


BString
Device::GetName()
{
	return fAttributeMap[B_TRANSLATE("Device name")];
}


BString
Device::GetManufacturer()
{
	return fAttributeMap[B_TRANSLATE("Manufacturer")];
}


BString
Device::GetDriverUsed()
{
	return fAttributeMap[B_TRANSLATE("Driver used")];
}


BString
Device::GetDevPathsPublished()
{
	return fAttributeMap[B_TRANSLATE("Device paths")];
}


void
Device::SetAttribute(const BString& name, const BString& value)
{
	if (name == B_TRANSLATE("Device name")) {
		SetText(value.String());
	}
	fAttributeMap[name] = value;
}


Attributes
Device::GetBasicAttributes()
{
	Attributes attributes;
	attributes.push_back(Attribute(B_TRANSLATE("Device name:"), GetName()));
	attributes.push_back(Attribute(B_TRANSLATE("Manufacturer:"),
		GetManufacturer()));
	return attributes;
}


Attributes
Device::GetBusAttributes()
{
	Attributes attributes;
	attributes.push_back(Attribute("None", ""));
	return attributes;
}


Attributes
Device::GetAllAttributes()
{
	Attributes attributes;
	AttributeMapIterator iter;
	for (iter = fAttributeMap.begin(); iter != fAttributeMap.end(); iter++) {
		attributes.push_back(Attribute(iter->first, iter->second));
	}
	return attributes;
}


BString
Device::GetBasicStrings()
{
	BString str(B_TRANSLATE("Device Name\t\t\t\t: %Name%\n"
							"Manufacturer\t\t\t: %Manufacturer%\n"
							"Driver used\t\t\t\t: %DriverUsed%\n"
							"Device paths\t: %DevicePaths%"));
	
	str.ReplaceFirst("%Name%", GetName()); 
	str.ReplaceFirst("%Manufacturer%", GetManufacturer());
	str.ReplaceFirst("%DriverUsed%", GetDriverUsed());
	str.ReplaceFirst("%DevicePaths%", GetDevPathsPublished());

	return str;
}

BString
Device::GetBusStrings()
{
	return B_TRANSLATE("None");
}


BString
Device::GetBusTabName()
{ 
	return B_TRANSLATE("Bus Information"); 
}


BString
Device::GetAllStrings()
{
	BString str;
	AttributeMapIterator iter;
	for (iter = fAttributeMap.begin(); iter != fAttributeMap.end(); iter++) {
		str << iter->first << " : " << iter->second << "\n";
	}
	return str;
}
