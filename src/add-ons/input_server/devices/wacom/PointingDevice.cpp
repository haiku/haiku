/*
 * Copyright 2005-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#include <malloc.h>
#include <string.h>

#include "DeviceReader.h"

#include "PointingDevice.h"

// constructor
PointingDevice::PointingDevice(MasterServerDevice* parent,
							   DeviceReader* reader)
	: fParent(parent),
	  fReader(reader),
	  fActive(false)
{
}

// destructor
PointingDevice::~PointingDevice()
{
	delete fReader;
}

// InitCheck
status_t
PointingDevice::InitCheck()
{
	return fParent && fReader ? fReader->InitCheck() : B_NO_INIT;
}

// SetActive
void
PointingDevice::SetActive(bool active)
{
	fActive = active;
}

// IsActive
bool
PointingDevice::IsActive() const
{
	return fActive;
}

// DevicePath
const char*
PointingDevice::DevicePath() const
{
	if (fReader) {
		return fReader->DevicePath();
	}
	return NULL;
}

// DisablePS2
bool
PointingDevice::DisablePS2() const
{
	return false;
}

// VendorID
uint16
PointingDevice::VendorID() const
{
	if (fReader)
		return fReader->VendorID();
	return 0;
}

// ProductID
uint16
PointingDevice::ProductID() const
{
	if (fReader)
		return fReader->ProductID();
	return 0;
}






