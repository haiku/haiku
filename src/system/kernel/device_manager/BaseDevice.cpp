/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "BaseDevice.h"


BaseDevice::BaseDevice()
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


bool
BaseDevice::HasSelect() const
{
	return false;
}


bool
BaseDevice::HasDeselect() const
{
	return false;
}


bool
BaseDevice::HasRead() const
{
	return false;
}


bool
BaseDevice::HasWrite() const
{
	return false;
}


bool
BaseDevice::HasIO() const
{
	return false;
}


status_t
BaseDevice::Read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	return B_UNSUPPORTED;
}


status_t
BaseDevice::Write(void* cookie, off_t pos, const void* buffer, size_t* _length)
{
	return B_UNSUPPORTED;
}


status_t
BaseDevice::IO(void* cookie, io_request* request)
{
	return B_UNSUPPORTED;
}


status_t
BaseDevice::Control(void* cookie, int32 op, void* buffer, size_t length)
{
	return B_UNSUPPORTED;
}


status_t
BaseDevice::Select(void* cookie, uint8 event, selectsync* sync)
{
	return B_UNSUPPORTED;
}


status_t
BaseDevice::Deselect(void* cookie, uint8 event, selectsync* sync)
{
	return B_UNSUPPORTED;
}
