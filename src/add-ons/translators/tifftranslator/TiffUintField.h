/*****************************************************************************/
// TiffUintField
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TiffUintField.h
//
// This object is for storing Unsigned Integer TIFF fields
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

#ifndef TIFF_UNIT_FIELD_H
#define TIFF_UNIT_FIELD_H

#include <DataIO.h>
#include <ByteOrder.h>
#include "TiffField.h"

class TiffUintField : public TiffField {
public:
	TiffUintField(IFDEntry &entry, BPositionIO &io, swap_action swp);
	virtual ~TiffUintField();
	
	status_t GetUint(uint32 &out, uint32 index = 0);
	
private:
	void LoadByte(IFDEntry &entry, BPositionIO &io, swap_action swp);
	void LoadShort(IFDEntry &entry, BPositionIO &io, swap_action swp);
	void LoadLong(IFDEntry &entry, BPositionIO &io, swap_action swp);
	
	union {
		uint8	*fpByte;
		uint16	*fpShort;
		uint32	*fpLong;
	};
};

#endif // #define TIFF_UNIT_FIELD_H
