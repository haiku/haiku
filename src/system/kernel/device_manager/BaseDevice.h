/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BASE_DEVICE_H
#define BASE_DEVICE_H


#include <device_manager.h>


namespace BPrivate {


class BaseDevice {
public:
							BaseDevice();
	virtual					~BaseDevice();

			void			SetID(ino_t id)	{ fID = id; }
			ino_t			ID() const		{ return fID; }

	virtual	status_t		InitDevice();
	virtual	void			UninitDevice();

	virtual void			Removed();

	virtual	bool			HasSelect() const;
	virtual	bool			HasDeselect() const;
	virtual	bool			HasRead() const;
	virtual	bool			HasWrite() const;
	virtual	bool			HasIO() const;

	virtual	status_t		Open(const char* path, int openMode,
								void** _cookie) = 0;
	virtual	status_t		Read(void* cookie, off_t pos, void* buffer,
								size_t* _length);
	virtual	status_t		Write(void* cookie, off_t pos, const void* buffer,
								size_t* _length);
	virtual	status_t		IO(void* cookie, io_request* request);
	virtual	status_t		Control(void* cookie, int32 op, void* buffer,
								size_t length);
	virtual	status_t		Select(void* cookie, uint8 event, selectsync* sync);
	virtual	status_t		Deselect(void* cookie, uint8 event,
								selectsync* sync);

	virtual	status_t		Close(void* cookie) = 0;
	virtual	status_t		Free(void* cookie) = 0;

protected:
	ino_t					fID;
};


} // namespace BPrivate


using BPrivate::BaseDevice;


#endif	// BASE_DEVICE_H
