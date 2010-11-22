/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_DEVICE_H
#define FILE_DEVICE_H


#include "BaseDevice.h"


namespace BPrivate {


class FileDevice : public BaseDevice {
public:
							FileDevice();
	virtual					~FileDevice();

			status_t		Init(const char* path);

	virtual	status_t		InitDevice();
	virtual	void			UninitDevice();

	virtual void			Removed();

	virtual	bool			HasSelect() const;
	virtual	bool			HasDeselect() const;
	virtual	bool			HasRead() const;
	virtual	bool			HasWrite() const;
	virtual	bool			HasIO() const;

	virtual	status_t		Open(const char* path, int openMode,
								void** _cookie);
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

	virtual	status_t		Close(void* cookie);
	virtual	status_t		Free(void* cookie);

private:
	struct Cookie;

private:
	int						fFD;
	off_t					fFileSize;
};


} // namespace BPrivate


using BPrivate::FileDevice;


#endif	// FILE_DEVICE_H
