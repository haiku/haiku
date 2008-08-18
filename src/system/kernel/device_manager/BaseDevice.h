/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BASE_DEVICE_H
#define BASE_DEVICE_H


#include <device_manager.h>


class BaseDevice {
public:
							BaseDevice();
	virtual					~BaseDevice();

			void			SetID(ino_t id) { fID = id; }
			ino_t			ID() const { return fID; }

			device_node*	Node() const { return fNode; }

	virtual	status_t		InitDevice();
	virtual	void			UninitDevice();

			device_module_info* Module() const { return fDeviceModule; }
			void*			Data() const { return fDeviceData; }

			bool			HasSelect() const
								{ return Module()->select != NULL; }
			bool			HasDeselect() const
								{ return Module()->deselect != NULL; }
			bool			HasRead() const
								{ return Module()->read != NULL; }
			bool			HasWrite() const
								{ return Module()->write != NULL; }
			bool			HasIO() const
								{ return Module()->io != NULL; }

	virtual	status_t		Open(const char* path, int openMode,
								void** _cookie);
			status_t		Read(void* cookie, off_t pos, void* buffer,
								size_t* _length);
			status_t		Write(void* cookie, off_t pos, const void* buffer,
								size_t* _length);
			status_t		IO(void* cookie, io_request* request);
			status_t		Control(void* cookie, int32 op, void* buffer,
								size_t length);
	virtual	status_t		Select(void* cookie, uint8 event, selectsync* sync);
			status_t		Deselect(void* cookie, uint8 event,
								selectsync* sync);

			status_t		Close(void* cookie);
			status_t		Free(void* cookie);

protected:
	ino_t					fID;
	device_node*			fNode;
	int32					fInitialized;
	device_module_info*		fDeviceModule;
	void*					fDeviceData;
};


inline status_t
BaseDevice::Open(const char* path, int openMode, void** _cookie)
{
	return Module()->open(Data(), path, openMode, _cookie);
}


inline status_t
BaseDevice::Read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	return Module()->read(cookie, pos, buffer, _length);
}


inline status_t
BaseDevice::Write(void* cookie, off_t pos, const void* buffer, size_t* _length)
{
	return Module()->write(cookie, pos, buffer, _length);
}


inline status_t
BaseDevice::IO(void* cookie, io_request* request)
{
	return Module()->io(cookie, request);
}


inline status_t
BaseDevice::Control(void* cookie, int32 op, void* buffer, size_t length)
{
	return Module()->control(cookie, op, buffer, length);
}


inline status_t
BaseDevice::Select(void* cookie, uint8 event, selectsync* sync)
{
	return Module()->select(cookie, event, sync);
}


inline status_t
BaseDevice::Deselect(void* cookie, uint8 event, selectsync* sync)
{
	return Module()->deselect(cookie, event, sync);
}


inline status_t
BaseDevice::Close(void* cookie)
{
	return Module()->close(cookie);
}


inline status_t
BaseDevice::Free(void* cookie)
{
	return Module()->free(cookie);
}

#endif	// BASE_DEVICE_H
