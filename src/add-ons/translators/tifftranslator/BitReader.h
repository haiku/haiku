/*****************************************************************************/
// BitReader
// Written by Michael Wilber, OBOS Translation Kit Team
//
// BitReader.h
//
// Wrapper class for StreamBuffer to make it convenient to read 1 bit at
// a time
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


#ifndef BIT_READER_H
#define BIT_READER_H

#include "StreamBuffer.h"

class BitReader {
public:
	// Setup the BitReader to read from the given stream with the given fill order
	BitReader(uint16 fillOrder, StreamBuffer *pstreambuf, bool binitialRead = true);
	~BitReader();
	
	// Change the BitReader to read from given stream with the given fill order
	status_t SetTo(uint16 fillOrder, StreamBuffer *pstreambuf, bool binitialRead = true);
	
	status_t InitCheck() const { return finitStatus; };
	
	uint32 BytesRead() const { return fnbytesRead; };
	uint32 BitsInBuffer() const { return fcurrentbit; };
	
	// return the current bit and increment the position
	// If the result is negative, an error occured
	status_t NextBit();
	
	// return the current bit without incrementing the position
	// (if buffer is empty, will read next byte in StreamBuffer)
	// If the result is negative, an error occured
	status_t PeekBit();

private:
	status_t ReadByte();
	
	StreamBuffer *fpstreambuf;
	status_t finitStatus;
	uint32 fnbytesRead;
	uint16 ffillOrder;
	uint8 fbitbuf;
	uint8 fcurrentbit;
};

#endif
