/*
 * Copyright 2005 Michael Lotz <mmlr@mlotz.ch>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

//! A RenderingBuffer implementation that accesses graphics memory directly.


#include "AccelerantBuffer.h"


AccelerantBuffer::AccelerantBuffer()
	: fDisplayModeSet(false),
	  fFrameBufferConfigSet(false),
	  fIsOffscreenBuffer(false)
{
}


AccelerantBuffer::AccelerantBuffer(const display_mode& mode,
		const frame_buffer_config& config)
	: fDisplayModeSet(false),
	  fFrameBufferConfigSet(false),
	  fIsOffscreenBuffer(false)
{
	SetDisplayMode(mode);
	SetFrameBufferConfig(config);
}


AccelerantBuffer::AccelerantBuffer(const AccelerantBuffer& other,
		bool offscreenBuffer)
	: fDisplayMode(other.fDisplayMode),
	  fFrameBufferConfig(other.fFrameBufferConfig),
	  fDisplayModeSet(other.fDisplayModeSet),
	  fFrameBufferConfigSet(other.fFrameBufferConfigSet),
	  fIsOffscreenBuffer(other.fIsOffscreenBuffer || offscreenBuffer)
{
}


AccelerantBuffer::~AccelerantBuffer()
{
}


status_t
AccelerantBuffer::InitCheck() const
{
	if (fDisplayModeSet && fFrameBufferConfigSet)
		return B_OK;
	
	return B_NO_INIT;
}


color_space
AccelerantBuffer::ColorSpace() const
{
	if (InitCheck() == B_OK)
		return (color_space)fDisplayMode.space;
	
	return B_NO_COLOR_SPACE;
}


void*
AccelerantBuffer::Bits() const
{
	if (InitCheck() != B_OK)
		return NULL;

	uint8* bits = (uint8*)fFrameBufferConfig.frame_buffer;

	if (fIsOffscreenBuffer)
		bits += fDisplayMode.virtual_height * fFrameBufferConfig.bytes_per_row;

	return bits;
}


uint32
AccelerantBuffer::BytesPerRow() const
{
	if (InitCheck() == B_OK)
		return fFrameBufferConfig.bytes_per_row;
	
	return 0;
}


uint32
AccelerantBuffer::Width() const
{
	if (InitCheck() == B_OK)
		return fDisplayMode.virtual_width;
	
	return 0;
}


uint32
AccelerantBuffer::Height() const
{
	if (InitCheck() == B_OK)
		return fDisplayMode.virtual_height;
	
	return 0;
}


void
AccelerantBuffer::SetDisplayMode(const display_mode& mode)
{
	fDisplayMode = mode;
	fDisplayModeSet = true;
}


void
AccelerantBuffer::SetFrameBufferConfig(const frame_buffer_config& config)
{
	fFrameBufferConfig = config;
	fFrameBufferConfigSet = true;
}


void
AccelerantBuffer::SetOffscreenBuffer(bool offscreenBuffer)
{
	fIsOffscreenBuffer = offscreenBuffer;
}
