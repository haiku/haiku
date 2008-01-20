/*
 * Copyright 2005-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#include "PointingDeviceFactory.h"

#include "DeviceReader.h"
#include "TabletDevice.h"


static const uint16 kVendorWacom = 0x056a;


/*static*/ PointingDevice*
PointingDeviceFactory::DeviceFor(MasterServerDevice* parent, const char* path)
{
	PointingDevice* device = NULL;
	DeviceReader* reader = new DeviceReader();
	if (reader->SetTo(path) >= B_OK) {
		switch (reader->VendorID()) {
			case kVendorWacom:
				device = new TabletDevice(parent, reader);
				break;
			default:
				delete reader;
				break;
		}
	} else {
		delete reader;
	}
	return device;
}

// forbidden:
PointingDeviceFactory::PointingDeviceFactory() {}
PointingDeviceFactory::~PointingDeviceFactory() {}


