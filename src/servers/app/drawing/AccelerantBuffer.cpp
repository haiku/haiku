//------------------------------------------------------------------------------
//	Copyright (c) 2005, Haiku, Inc.
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
//	File Name:		AccelerantBuffer.cpp
//	Author:			Michael Lotz <mmlr@mlotz.ch>
//	Description:	A RenderingBuffer implementation that accesses graphics
//					memory directly.
//  
//------------------------------------------------------------------------------

#include "AccelerantBuffer.h"

// constructors
AccelerantBuffer::AccelerantBuffer()
	:	fDisplayModeSet(false),
		fFrameBufferConfigSet(false)
{
}

AccelerantBuffer::AccelerantBuffer( const display_mode &mode,
									const frame_buffer_config &config)
	:	fDisplayModeSet(false),
		fFrameBufferConfigSet(false)
{
	SetDisplayMode(mode);
	SetFrameBufferConfig(config);
}

// destructor
AccelerantBuffer::~AccelerantBuffer()
{
}

// InitCheck
status_t
AccelerantBuffer::InitCheck() const
{
	if (fDisplayModeSet && fFrameBufferConfigSet)
		return B_OK;
	
	return B_NO_INIT;
}

// ColorSpace
color_space
AccelerantBuffer::ColorSpace() const
{
	if (InitCheck() == B_OK)
		return (color_space)fDisplayMode.space;
	
	return B_NO_COLOR_SPACE;
}

// Bits
void *
AccelerantBuffer::Bits() const
{
	if (InitCheck() != B_OK)
		return NULL;
	
	// TODO: Enable this if we can ensure that frame_buffer_dma is valid
	/*if (fFrameBufferConfig.frame_buffer_dma)
		return fFrameBufferConfig.frame_buffer_dma;*/
	
	return fFrameBufferConfig.frame_buffer;
}

// BytesPerRow
uint32
AccelerantBuffer::BytesPerRow() const
{
	if (InitCheck() == B_OK)
		return fFrameBufferConfig.bytes_per_row;
	
	return 0;
}

// Width
uint32
AccelerantBuffer::Width() const
{
	if (InitCheck() == B_OK)
		return fDisplayMode.virtual_width;
	
	return 0;
}

// Height
uint32
AccelerantBuffer::Height() const
{
	if (InitCheck() == B_OK)
		return fDisplayMode.virtual_height;
	
	return 0;
}

void
AccelerantBuffer::SetDisplayMode(const display_mode &mode)
{
	fDisplayMode = mode;
	fDisplayModeSet = true;
}

void
AccelerantBuffer::SetFrameBufferConfig(const frame_buffer_config &config)
{
	fFrameBufferConfig = config;
	fFrameBufferConfigSet = true;
}
