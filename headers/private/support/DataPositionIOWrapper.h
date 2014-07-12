/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DATA_POSITION_IO_WRAPPER_H_
#define _DATA_POSITION_IO_WRAPPER_H_


#include <DataIO.h>


class BDataPositionIOWrapper : public BPositionIO {
public:
								BDataPositionIOWrapper(BDataIO* io);
								~BDataPositionIOWrapper();

	virtual	ssize_t				Read(void* buffer, size_t size);
	virtual	ssize_t				Write(const void* buffer, size_t size);

	virtual	ssize_t				ReadAt(off_t position, void* buffer,
									size_t size);
	virtual	ssize_t				WriteAt(off_t position, const void* buffer,
									size_t size);

	virtual	off_t				Seek(off_t position, uint32 seekMode);
	virtual	off_t				Position() const;

	virtual	status_t			SetSize(off_t size);
	virtual	status_t			GetSize(off_t* size) const;

private:
				BDataIO*		fIO;
				off_t			fPosition;
};


#endif	// _DATA_POSITION_IO_WRAPPER_H_
