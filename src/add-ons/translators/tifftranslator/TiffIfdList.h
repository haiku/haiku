/*****************************************************************************/
// TiffIfdList
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TiffIfdList.h
//
// This object is for storing all IFD entries from a TIFF file
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

#ifndef TIFF_IFD_LIST_H
#define TIFF_IFD_LIST_H

#include <ByteOrder.h>
#include <DataIO.h>
#include "TiffIfd.h"

struct TiffIfdNode {
	TiffIfd *pifd;
	TiffIfdNode *pnext;
};

class TiffIfdList {
public:
	TiffIfdList(uint32 offset, BPositionIO &io, swap_action swp);
	~TiffIfdList();
	status_t InitCheck() { return finitStatus; };
	
	status_t LoadIfds(uint32 offset, BPositionIO &io, swap_action swp);
	void Empty();
	
	int32 GetCount() { return fcount; };
	TiffIfd *GetIfd(int32 index);
		// index is zero based

private:
	status_t finitStatus;
	TiffIfdNode *fpfirst;
	int32 fcount;
};

#endif // #ifndef TIFF_IFD_LIST_H
