/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FD_IO_H_
#define _FD_IO_H_


#include <DataIO.h>


class BFdIO : public BPositionIO {
public:
								BFdIO();
								BFdIO(int fd, bool keepFd);
	virtual						~BFdIO();

			void				SetTo(int fd, bool keepFd);
			void				Unset();

	virtual	ssize_t				Read(void* buffer, size_t size);
	virtual	ssize_t				Write(const void* buffer, size_t size);

	virtual	ssize_t				ReadAt(off_t position, void* buffer,
									size_t size);
	virtual	ssize_t				WriteAt(off_t position, const void* buffer,
									size_t size);

	virtual	off_t				Seek(off_t position, uint32 seekMode);
	virtual	off_t				Position() const;

	virtual	status_t			SetSize(off_t size);
	virtual	status_t			GetSize(off_t* _size) const;

private:
								BFdIO(const BFdIO& other);
			BFdIO&				operator=(const BFdIO& other);

private:
			int					fFd;
			bool				fOwnsFd;
};


#endif	// _FD_IO_H_
