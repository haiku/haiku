/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "BaseDevice.h"


BaseDevice::BaseDevice()
	:
	fNode(NULL),
	fInitialized(0),
	fDeviceModule(NULL),
	fDeviceData(NULL)
{
}


BaseDevice::~BaseDevice()
{
}


status_t
BaseDevice::InitDevice()
{
	return B_ERROR;
}


void
BaseDevice::UninitDevice()
{
}


void
BaseDevice::Removed()
{
}
