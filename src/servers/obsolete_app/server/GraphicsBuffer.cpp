//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		GraphicsBuffer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//
//	Description:	Convenience class for working with graphics buffers
//					Based on concepts from the Anti-Grain Geometry vector gfx library
//------------------------------------------------------------------------------
#include "GraphicsBuffer.h"

GraphicsBuffer::GraphicsBuffer(uint8 *buffer, uint32 width, uint32 height, uint32 rowbytes)
{
	fBuffer=NULL;
	fRowList=NULL;
	fWidth=0;
	fHeight=0;
	fBytesPerRow=0;
	fMaxHeight=0;
	
	SetTo(buffer, width, height, rowbytes);
}

GraphicsBuffer::GraphicsBuffer(const GraphicsBuffer &buffer)
{
	SetTo(buffer.fBuffer, buffer.fWidth, buffer.fHeight, buffer.fBytesPerRow);
}

GraphicsBuffer::~GraphicsBuffer()
{
	delete [] fRowList;
}

void GraphicsBuffer::SetTo(uint8 *buffer, uint32 width, uint32 height, uint32 rowbytes)
{
	fBuffer = buffer;
	fWidth = width;
	fHeight = height;
	fBytesPerRow = rowbytes;
	
	if(height > fMaxHeight)
	{
		delete [] fRowList;
		fRowList = new uint8* [fMaxHeight = height];
	}
	
	uint8 *row_ptr = fBuffer;
	
	if(rowbytes < 0)
	{
		row_ptr = fBuffer - int(height - 1) * rowbytes;
	}
	
	uint8 **rows = fRowList;
	
	while(height--)
	{
		*rows++ = row_ptr;
		row_ptr += rowbytes;
	}
}

void GraphicsBuffer::Unset(void)
{
	delete [] fRowList;
	fBuffer=NULL;
	fRowList=NULL;
	fWidth=0;
	fHeight=0;
	fBytesPerRow=0;
	fMaxHeight=0;
}


GraphicsBuffer &GraphicsBuffer::operator=(const GraphicsBuffer &buffer)
{
	SetTo(buffer.fBuffer, buffer.fWidth, buffer.fHeight, buffer.fBytesPerRow);
	return *this;
}
