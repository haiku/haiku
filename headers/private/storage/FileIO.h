/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FILE_IO_H
#define _FILE_IO_H


#include <stdio.h>

#include <DataIO.h>


class BFileIO : public BPositionIO {
public:
								BFileIO(FILE* file,
									bool takeOverOwnership = false);
	virtual						~BFileIO();

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
								BFileIO(const BFileIO& other);
			BFileIO&			operator=(const BFileIO& other);

			off_t				_Seek(off_t position, uint32 seekMode) const;

private:
			FILE*				fFile;
			bool				fOwnsFile;
};


#endif	// _FILE_IO_H
