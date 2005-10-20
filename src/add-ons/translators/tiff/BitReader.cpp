/*****************************************************************************/
// BitReader
// Written by Michael Wilber, OBOS Translation Kit Team
//
// BitReader.cpp
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

#include "BitReader.h"

BitReader::BitReader(uint16 fillOrder, StreamBuffer *pstreambuf, bool binitialRead)
{
	SetTo(fillOrder, pstreambuf, binitialRead);
}

BitReader::~BitReader()
{
	finitStatus = B_ERROR;
	fpstreambuf = NULL;
	fbitbuf = 0;
	fcurrentbit = 0;
}

status_t
BitReader::SetTo(uint16 fillOrder, StreamBuffer *pstreambuf, bool binitialRead)
{
	finitStatus = B_OK;
	fnbytesRead = 0;
	fbitbuf = 0;
	fcurrentbit = 0;
	fpstreambuf = pstreambuf;
	ffillOrder = fillOrder;
	if (ffillOrder != 1 && ffillOrder != 2)
		finitStatus = B_BAD_VALUE;
	else if (!fpstreambuf)
		finitStatus = B_BAD_VALUE;
	else if (binitialRead)
		finitStatus = ReadByte();
		
	return finitStatus;
}

status_t
BitReader::NextBit()
{
	status_t result;
	result = PeekBit();
	if (result >= 0)
		fcurrentbit--;
	
	return result;
}

status_t
BitReader::PeekBit()
{
	// if fcurrentbit is zero, read in next byte
	if (!fcurrentbit && ReadByte() != B_OK)
		return B_ERROR;
		
	if (ffillOrder == 1)
		return (fbitbuf >> (fcurrentbit - 1)) & 1;
	else
		return (fbitbuf >> (8 - fcurrentbit)) & 1;
}

status_t
BitReader::ReadByte()
{
	if (fpstreambuf->Read(&fbitbuf, 1) != 1)
		return B_ERROR;
	
	fnbytesRead++;
	fcurrentbit = 8;
	return B_OK;
}
