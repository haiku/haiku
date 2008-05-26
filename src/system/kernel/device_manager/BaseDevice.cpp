/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "BaseDevice.h"


BaseDevice::BaseDevice()
	:
	fNode(NULL),
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
