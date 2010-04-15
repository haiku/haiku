/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FILE_DESCRIPTOR_IO_H
#define _FILE_DESCRIPTOR_IO_H


#include <DataIO.h>


class BFileDescriptorIO : public BPositionIO {
public:
								BFileDescriptorIO(int fd,
									bool takeOverOwnership = false);
	virtual						~BFileDescriptorIO();

	virtual	ssize_t				Read(void *buffer, size_t size);
	virtual	ssize_t				Write(const void *buffer, size_t size);

	virtual	ssize_t				ReadAt(off_t position, void *buffer,
									size_t size);
	virtual	ssize_t				WriteAt(off_t position, const void *buffer,
									size_t size);

	virtual	off_t				Seek(off_t position, uint32 seekMode);
	virtual	off_t				Position() const;

	virtual	status_t			SetSize(off_t size);
	virtual	status_t			GetSize(off_t* size) const;

private:
								BFileDescriptorIO(
									const BFileDescriptorIO& other);
			BFileDescriptorIO&	operator=(const BFileDescriptorIO& other);

private:
			int					fFD;
			bool				fOwnsFD;
};


#endif	// _FILE_DESCRIPTOR_IO_H
