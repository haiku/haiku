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

class TiffIfd {
public:
	TiffIfd(uint32 offset, BPositionIO &io, swap_action swp);
	~TiffIfd();
	
	status_t InitCheck() { return finitStatus; };
	status_t GetUintField(uint16 tag, TiffUintField *&poutField);

private:
	void LoadFields(uint32 offset, BPositionIO &io, swap_action swp);
	
	TiffField **fpfields;
	status_t finitStatus;
	uint32 fnextIFDOffset;
	uint16 ffieldCount;
};

#endif // TIFF_IFD_H
