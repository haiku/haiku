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
//	Author(s):		Jack Burton (burton666@libero.it)
//	Description:	Pure virtual BDataIO and BPositioIO classes provide
//					the protocol for Read()/Write()/Seek().
//
//					BMallocIO and BMemoryIO classes implement the protocol,
//					as does BFile in the Storage Kit.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <cstdio>

// System Includes -------------------------------------------------------------
#include <DataIO.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// *** BDataIO ***
BDataIO::BDataIO()
{
}


BDataIO::~BDataIO()
{
}


BDataIO::BDataIO(const BDataIO &)
{
	//Copying not allowed
}


BDataIO &
BDataIO::operator=(const BDataIO &)
{
	//Copying not allowed
	return *this;
}

void 
BDataIO::_ReservedDataIO1(){}

void 
BDataIO::_ReservedDataIO2(){}

void 
BDataIO::_ReservedDataIO3(){}

void 
BDataIO::_ReservedDataIO4(){}

#if !_PR3_COMPATIBLE_
void 
BDataIO::_ReservedDataIO5(){}

void 
BDataIO::_ReservedDataIO6(){}

void 
BDataIO::_ReservedDataIO7(){}

void 
BDataIO::_ReservedDataIO8(){}

void 
BDataIO::_ReservedDataIO9(){}

void 
BDataIO::_ReservedDataIO10(){}

void 
BDataIO::_ReservedDataIO11(){}

void 
BDataIO::_ReservedDataIO12(){}
#endif

// *** BPositionIO ***
BPositionIO::BPositionIO()
{
}


BPositionIO::~BPositionIO()
{
}


ssize_t
BPositionIO::Read(void *buffer, size_t size)
{
	off_t curPos = Position();
	ssize_t result = ReadAt(curPos, buffer, size);
	if (result > 0)
		Seek(result, SEEK_CUR);
	
	return result;
}


ssize_t
BPositionIO::Write(const void *buffer, size_t size)
{
	off_t curPos = Position();
	ssize_t result = WriteAt(curPos, buffer, size);
	if (result > 0)
		Seek(result, SEEK_CUR);
	
	return result;
}


status_t
BPositionIO::SetSize(off_t size)
{
	return B_ERROR;
}

void 
BPositionIO::_ReservedPositionIO1(){}

void 
BPositionIO::_ReservedPositionIO2(){}

void 
BPositionIO::_ReservedPositionIO3(){}

void 
BPositionIO::_ReservedPositionIO4(){}

#if !_PR3_COMPATIBLE_
void 
BPositionIO::_ReservedPositionIO5(){}

void 
BPositionIO::_ReservedPositionIO6(){}

void 
BPositionIO::_ReservedPositionIO7(){}

void 
BPositionIO::_ReservedPositionIO8(){}

void 
BPositionIO::_ReservedPositionIO9(){}

void 
BPositionIO::_ReservedPositionIO10(){}

void 
BPositionIO::_ReservedPositionIO11(){}

void 
BPositionIO::_ReservedPositionIO12(){}
#endif

// *** BMemoryIO ***
BMemoryIO::BMemoryIO(void *p, size_t len)
		:fReadOnly(false),
		fBuf((char*)p),
		fLen(len),
		fPhys(len),
		fPos(0)
		
{
}


BMemoryIO::BMemoryIO(const void *p, size_t len)
		:fReadOnly(true),
		fBuf((char*)p),
		fLen(len),
		fPhys(len),
		fPos(0)
{
}


BMemoryIO::~BMemoryIO()
{
}


ssize_t
BMemoryIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	ssize_t result = size;
	
	if (pos < 0 || pos >= fLen)
		result = 0;
	else if (pos + size >= fLen)
		result = fLen - pos;
	
	memcpy(buffer, fBuf + pos, result);
	
	return result;
}


ssize_t
BMemoryIO::WriteAt(off_t pos, const void *buffer, size_t size)
{	
	if (fReadOnly)
		return B_NOT_ALLOWED;
	
	ssize_t result = size;	
	
	if (pos < 0 || pos >= fPhys)
		result = 0;
	else if (pos + size >= fPhys)
		result = fPhys - pos;
	
	if (pos + result > fLen)
		fLen = pos + result;
					
	if (result > 0)
		memcpy(fBuf + pos, buffer, result);
	
	return result;
}


off_t
BMemoryIO::Seek(off_t position, uint32 seek_mode)
{
	switch (seek_mode) {
		case SEEK_SET:
			fPos = position;
			break;
		case SEEK_CUR:
			fPos += position;
			break;		
		case SEEK_END:
			fPos = fLen + position;
			break;
		default:
			break;
	}	
	return fPos;
}


off_t
BMemoryIO::Position() const
{
	return fPos;
}


status_t
BMemoryIO::SetSize(off_t size)
{
	if (fReadOnly)
		return B_NOT_ALLOWED;
	
	status_t err;
	
	if (size < 0 || size > fPhys)
		err = B_ERROR;
	else {
		err = B_OK;
		fLen = size;
	}

	return err;
}


BMemoryIO::BMemoryIO(const BMemoryIO &)
{
	//Copying not allowed
}


BMemoryIO &
BMemoryIO::operator=(const BMemoryIO &)
{
	//Copying not allowed
	return *this;
}

void 
BMemoryIO::_ReservedMemoryIO1(){}

void 
BMemoryIO::_ReservedMemoryIO2(){}


/*
 * $Log $
 *
 * $Id  $
 *
 */

