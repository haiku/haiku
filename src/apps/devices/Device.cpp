/*
 * Copyright 2008-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pieter Panman
 */


#include "Device.h"

#include <iostream>


// This list comes from the pciid list, except for the last one
const char* kCategoryString[] = {
	"Unclassified device", 					// 0x00
	"Mass storage controller",				// 0x01
	"Network controller",					// 0x02
	"Display controller", 					// 0x03
	"Multimedia controller", 				// 0x04
	"Memory controller",  					// 0x05
	"Bridge", 								// 0x06
	"Communication controller",  			// 0x07
	"Generic system peripheral",  			// 0x08
	"Input device controller",  			// 0x09
	"Docking station",  					// 0x0a
	"Processor",  							// 0x0b
	"Serial bus controller",  				// 0x0c
	"Wireless controller",  				// 0x0d
	"Intelligent controller",  				// 0x0e
	"Satellite communications controller",  // 0x0f
	"Encryption controller",  				// 0x10
	"Signal processing controller",			// 0x11
	"Computer",								// 0x12 (added later)
	"ACPI controller"			// 0x13 (added later)
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
	SetAttribute("Device name", name);
	SetAttribute("Manufacturer", manufacturer);
	SetAttribute("Driver used", driverUsed);
	SetAttribute("Device paths", devPathsPublished);
}


Device::~Device()
{
}


void
Device::SetAttribute(const BString& name, const BString& value)
{
	if (name == "Device name") {
		SetText(value.String());
	}
	fAttributeMap[name] = value;
}


Attributes
Device::GetBasicAttributes()
{
	Attributes attributes;
	attributes.push_back(Attribute("Device name:", GetName()));
	attributes.push_back(Attribute("Manufacturer:", GetManufacturer()));
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
	BString str;
	str << "Device Name\t\t\t\t: " << GetName() << "\n";
	str << "Manufacturer\t\t\t: " << GetManufacturer() << "\n";
	str << "Driver used\t\t\t\t: " << GetDriverUsed() << "\n";
	str << "Device paths\t: " << GetDevPathsPublished();
	return str;
}

BString
Device::GetBusStrings()
{
	return "None";
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
