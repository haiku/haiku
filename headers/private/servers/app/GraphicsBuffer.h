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
//	File Name:		GraphicsBuffer.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//
//	Description:	Convenience class for working with graphics buffers
//					Based on concepts from the Anti-Grain Geometry vector gfx library
//------------------------------------------------------------------------------
#ifndef GFX_BUFFER_H
#define GFX_BUFFER_H

#include <SupportDefs.h>

class GraphicsBuffer
{
public:
	GraphicsBuffer(uint8 *buffer, uint32 width, uint32 height, uint32 rowbytes);
	~GraphicsBuffer();
	
	void SetTo(uint8 *buffer, uint32 width, uint32 height, uint32 rowbytes);
	void Unset(void);
	
	uint8 *Bits(void) const { return fBuffer;    }
	uint8 *RowAt(uint32 row) { return fRowList[row]; }
	
	uint32 Width(void) const { return fWidth;  }
	uint32 Height(void) const { return fHeight; }
	
	uint32 BytesPerRow(void) const { return fBytesPerRow; }
	
private:
	
	uint32 fWidth;
	uint32 fHeight;
	uint8 *fBuffer;
	uint8 **fRowList;
	uint32 fBytesPerRow;
	uint32 fMaxHeight;
};

#endif
