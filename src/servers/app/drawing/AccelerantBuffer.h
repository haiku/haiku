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
//	File Name:		AccelerantBuffer.h
//	Author:			Michael Lotz <mmlr@mlotz.ch>
//	Description:	A RenderingBuffer implementation that accesses graphics
//					memory directly.
//  
//------------------------------------------------------------------------------

#ifndef ACCELERANT_BUFFER_H
#define ACCELERANT_BUFFER_H

#include <Accelerant.h>
#include "RenderingBuffer.h"

class AccelerantBuffer : public RenderingBuffer {
public:
								AccelerantBuffer();
								AccelerantBuffer(const display_mode &mode,
												 const frame_buffer_config &config);
virtual							~AccelerantBuffer();

virtual	status_t				InitCheck() const;

virtual	color_space				ColorSpace() const;
virtual	void					*Bits() const;
virtual	uint32					BytesPerRow() const;
virtual	uint32					Width() const;
virtual	uint32					Height() const;

		void					SetDisplayMode(const display_mode &mode);
		void					SetFrameBufferConfig(const frame_buffer_config &config);

private:
		display_mode			fDisplayMode;
		frame_buffer_config		fFrameBufferConfig;
		
		bool					fDisplayModeSet;
		bool					fFrameBufferConfigSet;
};

#endif // ACCELERANT_BUFFER_H
