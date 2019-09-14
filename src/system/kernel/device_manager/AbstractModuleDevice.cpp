/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "AbstractModuleDevice.h"


AbstractModuleDevice::AbstractModuleDevice()
	:
	fNode(NULL),
	fInitialized(0),
	fDeviceModule(NULL),
	fDeviceData(NULL)
{
}


AbstractModuleDevice::~AbstractModuleDevice()
{
}


bool
AbstractModuleDevice::HasSelect() const
{
	return Module()->select != NULL;
}


bool
AbstractModuleDevice::HasDeselect() const
{
	return Module()->deselect != NULL;
}


bool
AbstractModuleDevice::HasRead() const
{
	return Module()->read != NULL;
}


bool
AbstractModuleDevice::HasWrite() const
{
	return Module()->write != NULL;
}


bool
AbstractModuleDevice::HasIO() const
{
	return Module()->io != NULL;
}


status_t
AbstractModuleDevice::Open(const char* path, int openMode, void** _cookie)
{
	return Module()->open(Data(), path, openMode, _cookie);
}


status_t
AbstractModuleDevice::Read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	if (Module()->read == NULL)
		return BaseDevice::Read(cookie, pos, buffer, _length);
	return Module()->read(cookie, pos, buffer, _length);
}


status_t
AbstractModuleDevice::Write(void* cookie, off_t pos, const void* buffer, size_t* _length)
{
	if (Module()->write == NULL)
		return BaseDevice::Write(cookie, pos, buffer, _length);
	return Module()->write(cookie, pos, buffer, _length);
}


status_t
AbstractModuleDevice::IO(void* cookie, io_request* request)
{
	if (Module()->io == NULL)
		return BaseDevice::IO(cookie, request);
	return Module()->io(cookie, request);
}


status_t
AbstractModuleDevice::Control(void* cookie, int32 op, void* buffer, size_t length)
{
	if (Module()->control == NULL)
		return BaseDevice::Control(cookie, op, buffer, length);
	return Module()->control(cookie, op, buffer, length);
}


status_t
AbstractModuleDevice::Select(void* cookie, uint8 event, selectsync* sync)
{
	if (Module()->select == NULL)
		return BaseDevice::Select(cookie, event, sync);
	return Module()->select(cookie, event, sync);
}


status_t
AbstractModuleDevice::Deselect(void* cookie, uint8 event, selectsync* sync)
{
	if (Module()->deselect == NULL)
		return BaseDevice::Deselect(cookie, event, sync);
	return Module()->deselect(cookie, event, sync);
}


status_t
AbstractModuleDevice::Close(void* cookie)
{
	return Module()->close(cookie);
}


status_t
AbstractModuleDevice::Free(void* cookie)
{
	return Module()->free(cookie);
}
