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
//	File Name:		DataIO.h
//	Author(s):		Jack Burton (burton666@libero.it)
//	Description:	Pure virtual BDataIO and BPositioIO classes provide
//					the protocol for Read()/Write()/Seek().
//
//					BMallocIO and BMemoryIO classes implement the protocol,
//					as does BFile in the Storage Kit.
//------------------------------------------------------------------------------

#ifndef	_DATA_IO_H
#define	_DATA_IO_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <SupportDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// BDataIO Class ---------------------------------------------------------------
class BDataIO {
public:
					BDataIO();
virtual				~BDataIO();

virtual	ssize_t		Read(void *buffer, size_t size) = 0;
virtual	ssize_t		Write(const void *buffer, size_t size) =0;

// Private or reserved ----------------------
private:

virtual	void		_ReservedDataIO1();
virtual	void		_ReservedDataIO2();
virtual	void		_ReservedDataIO3();
virtual	void		_ReservedDataIO4();

#if !_PR3_COMPATIBLE_
virtual	void		_ReservedDataIO5();
virtual	void		_ReservedDataIO6();
virtual	void		_ReservedDataIO7();
virtual	void		_ReservedDataIO8();
virtual	void		_ReservedDataIO9();
virtual	void		_ReservedDataIO10();
virtual	void		_ReservedDataIO11();
virtual	void		_ReservedDataIO12();
#endif

					BDataIO(const BDataIO &);
		BDataIO		&operator=(const BDataIO &);

		int32		_reserved[2];
};

// BPositionIO Class -----------------------------------------------------------

class BPositionIO : public BDataIO {
public:
					BPositionIO();
virtual				~BPositionIO();

virtual	ssize_t		Read(void *buffer, size_t size);
virtual	ssize_t		Write(const void *buffer, size_t size);

virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size) = 0;
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size) = 0;

virtual off_t		Seek(off_t position, uint32 seek_mode) = 0;
virtual	off_t		Position() const = 0;

virtual status_t	SetSize(off_t size);

// Private or reserved ----------------------
private:
virtual	void		_ReservedPositionIO1();
virtual	void		_ReservedPositionIO2();
virtual void		_ReservedPositionIO3();
virtual void		_ReservedPositionIO4();

#if !_PR3_COMPATIBLE_
virtual	void		_ReservedPositionIO5();
virtual	void		_ReservedPositionIO6();
virtual	void		_ReservedPositionIO7();
virtual	void		_ReservedPositionIO8();
virtual	void		_ReservedPositionIO9();
virtual	void		_ReservedPositionIO10();
virtual	void		_ReservedPositionIO11();
virtual	void		_ReservedPositionIO12();
#endif

		int32		_reserved[2];
};

// BMallocIO Class -------------------------------------------------------------

class BMallocIO : public BPositionIO {
public:
					BMallocIO();
virtual				~BMallocIO();

virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

virtual	off_t		Seek(off_t pos, uint32 seek_mode);
virtual off_t		Position() const;
virtual status_t	SetSize(off_t size);

		void		SetBlockSize(size_t blocksize);

const	void		*Buffer() const;
		size_t		BufferLength() const;

// Private or reserved ----------------------
private:

virtual	void		_ReservedMallocIO1();
virtual	void		_ReservedMallocIO2();

					BMallocIO(const BMallocIO &);
		BMallocIO	&operator=(const BMallocIO &);

		size_t		fBlockSize;
		size_t		fMallocSize;
		size_t		fLength;
		char		*fData;
		off_t		fPosition;
		int32		_reserved[1];
};

// BMemoryIO Class -------------------------------------------------------------

class BMemoryIO : public BPositionIO {
public:
					BMemoryIO(void *p, size_t len);
					BMemoryIO(const void *p, size_t len);
virtual				~BMemoryIO();

virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

virtual	off_t		Seek(off_t pos, uint32 seek_mode);
virtual off_t		Position() const;

virtual	status_t	SetSize(off_t size);

// Private or reserved ----------------------
private:
virtual	void		_ReservedMemoryIO1();
virtual	void		_ReservedMemoryIO2();

					BMemoryIO(const BMemoryIO &);
		BMemoryIO	&operator=(const BMemoryIO &);

		bool		fReadOnly;
		char		*fBuf;
		size_t		fLen;
		size_t		fPhys;
		size_t		fPos;

		int32		_reserved[1];
};

//------------------------------------------------------------------------------

#endif // _DATA_IO_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

