//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		DataIO.cpp
//	Author(s):		Ingo Weinhold (bonefish@users.sf.net)
//					Stefano Ceccherini (burton666@libero.it)
//					The Storage Team
//	Description:	Pure virtual BDataIO and BPositionIO classes provide
//					the protocol for Read()/Write()/Seek().
//
//					BMallocIO and BMemoryIO classes implement the protocol,
//					as does BFile in the Storage Kit.
//------------------------------------------------------------------------------
#ifndef _DATA_IO_H
#define _DATA_IO_H

#include <SupportDefs.h>

// DataIO
class BDataIO {
public:
	BDataIO();
	virtual ~BDataIO();

	virtual ssize_t Read(void *buffer, size_t size) = 0;
	virtual ssize_t Write(const void *buffer, size_t size) = 0;

private:
	BDataIO(const BDataIO &);
	BDataIO &operator=(const BDataIO &);

	virtual void _ReservedDataIO1();
	virtual void _ReservedDataIO2();
	virtual void _ReservedDataIO3();
	virtual void _ReservedDataIO4();
	virtual void _ReservedDataIO5();
	virtual void _ReservedDataIO6();
	virtual void _ReservedDataIO7();
	virtual void _ReservedDataIO8();
	virtual void _ReservedDataIO9();
	virtual void _ReservedDataIO10();
	virtual void _ReservedDataIO11();
	virtual void _ReservedDataIO12();

	uint32	_reserved[2];
};

// BPositionIO
class BPositionIO : public BDataIO {
public:
	BPositionIO();
	virtual ~BPositionIO();

	virtual ssize_t Read(void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);

	virtual ssize_t ReadAt(off_t position, void *buffer, size_t size) = 0;
	virtual ssize_t WriteAt(off_t position, const void *buffer,
							size_t size) = 0;

	virtual off_t Seek(off_t position, uint32 seekMode) = 0;
	virtual off_t Position() const = 0;

	virtual status_t SetSize(off_t size);

private:
	virtual void _ReservedPositionIO1();
	virtual void _ReservedPositionIO2();
	virtual void _ReservedPositionIO3();
	virtual void _ReservedPositionIO4();
	virtual void _ReservedPositionIO5();
	virtual void _ReservedPositionIO6();
	virtual void _ReservedPositionIO7();
	virtual void _ReservedPositionIO8();
	virtual void _ReservedPositionIO9();
	virtual void _ReservedPositionIO10();
	virtual void _ReservedPositionIO11();
	virtual void _ReservedPositionIO12();

	uint32	_reserved[2];
};

// BMemoryIO
class BMemoryIO : public BPositionIO {
public:
	BMemoryIO(void *data, size_t len);
	BMemoryIO(const void *data, size_t len);
	virtual ~BMemoryIO();

	virtual ssize_t ReadAt(off_t position, void *buffer, size_t size);
	virtual ssize_t WriteAt(off_t position, const void *buffer, size_t size);

	virtual off_t Seek(off_t position, uint32 seekMode);
	virtual off_t Position() const;

	virtual status_t SetSize(off_t size);

private:
	BMemoryIO(const BMemoryIO &);
	BMemoryIO &operator=(const BMemoryIO &);

	virtual void _ReservedMemoryIO1();
	virtual void _ReservedMemoryIO2();

	bool	fReadOnly;
	char	*fBuf;
	size_t	fLen;
	size_t	fPhys;
	size_t	fPos;
	uint32	_reserved[1];
};

// BMallocIO
class BMallocIO : public BPositionIO {
public:
	BMallocIO();
	virtual ~BMallocIO();

	virtual ssize_t ReadAt(off_t position, void *buffer, size_t size);
	virtual ssize_t WriteAt(off_t position, const void *buffer, size_t size);

	virtual off_t Seek(off_t position, uint32 seekMode);
	virtual off_t Position() const;

	virtual status_t SetSize(off_t size);

	void SetBlockSize(size_t blockSize);

	const void *Buffer() const;
	size_t BufferLength() const;

private:
	BMallocIO(const BMallocIO &);
	BMallocIO &operator=(const BMallocIO &);

	virtual void _ReservedMallocIO1();
	virtual void _ReservedMallocIO2();

	size_t	fBlockSize;
	size_t	fMallocSize;
	size_t	fLength;
	char	*fData;
	off_t	fPosition;
	uint32	_reserved[1];
};

#endif
