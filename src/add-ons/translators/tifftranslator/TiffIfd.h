/*****************************************************************************/
// TiffIfd
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TiffIfd.h
//
// This object is for storing a TIFF Image File Directory
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef TIFF_IFD_H
#define TIFF_IFD_H

#include <ByteOrder.h>
#include <DataIO.h>
#include "TiffField.h"
#include "TiffUintField.h"

class TiffIfdException {
public:
	TiffIfdException() { };
};
class TiffIfdFieldNotFoundException : public TiffIfdException {
public:
	TiffIfdFieldNotFoundException() { };
};
class TiffIfdUnexpectedTypeException : public TiffIfdException {
public:
	TiffIfdUnexpectedTypeException() { };
};
class TiffIfdBadIndexException : public TiffIfdException {
public:
	TiffIfdBadIndexException() { };
};
class TiffIfdNoMemoryException : public TiffIfdException {
public:
	TiffIfdNoMemoryException() { };
};

class TiffIfd {
public:
	TiffIfd(uint32 offset, BPositionIO &io, swap_action swp);
	~TiffIfd();	
	status_t InitCheck() { return finitStatus; };
	
	bool HasField(uint16 tag);
	uint32 GetCount(uint16 tag);
		// throws: TiffIfdFieldNotFoundException()
		
	uint32 GetNextIfdOffset() { return fnextIFDOffset; };
	
	uint32 GetUint(uint16 tag, uint32 index = 0);
		// index is the base one index for the desired
		// number in the specified field. When index is the default
		// value of zero: if the count is one, the
		// first number will be returned, if it is not
		// one, TiffIfdBadIndexException() will be thrown
		//
		// throws: TiffIfdFieldNotFoundException(),
		// TiffIfdUnexpectedTypeException(),
		// TiffIfdBadIndexException()
		
	uint32 GetAdjustedColorMap(uint8 **pout);
	uint32 GetUint32Array(uint16 tag, uint32 **pout);
		// copies all of the uints from tag to
		// the pointer pointed to by pout
		// and returns the number of uints copied
		//
		// throws: TiffIfdFieldNotFoundException(),
		// TiffIfdUnexpectedTypeException(),
		// TiffIfdBadIndexException()
		// TiffIfdNoMemoryException()

private:
	void LoadFields(uint32 offset, BPositionIO &io, swap_action swp);
	TiffField *GetField(uint16 tag);
	
	TiffField **fpfields;
	status_t finitStatus;
	uint32 fnextIFDOffset;
	uint16 ffieldCount;
};

#endif // TIFF_IFD_H
