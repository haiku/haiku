/*
 * Copyright 2002-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include "Bitmap.h"
#include "BitmapBuffer.h"
#include "BBitmapBuffer.h"

#include "BitmapHWInterface.h"

using std::nothrow;


BitmapHWInterface::BitmapHWInterface(ServerBitmap* bitmap)
	:
	HWInterface(false, false),
	fBackBuffer(NULL),
	fFrontBuffer(new(nothrow) BitmapBuffer(bitmap))
{
}


BitmapHWInterface::~BitmapHWInterface()
{
}


status_t
BitmapHWInterface::Initialize()
{
	status_t ret = HWInterface::Initialize();
	if (ret < B_OK)
		return ret;

	ret = fFrontBuffer->InitCheck();
	if (ret < B_OK)
		return ret;

// TODO: Remove once unnecessary...
	// fall back to double buffered mode until Painter knows how
	// to draw onto non 32-bit surfaces...
	if (fFrontBuffer->ColorSpace() != B_RGB32
		&& fFrontBuffer->ColorSpace() != B_RGBA32) {
		BBitmap* backBitmap = new BBitmap(fFrontBuffer->Bounds(),
			B_BITMAP_NO_SERVER_LINK, B_RGBA32);
		fBackBuffer.SetTo(new BBitmapBuffer(backBitmap));

		ret = fBackBuffer->InitCheck();
		if (ret < B_OK) {
			fBackBuffer.Unset();
		} else {
			// import the current contents of the bitmap
			// into the back bitmap
			backBitmap->ImportBits(fFrontBuffer->Bits(),
				fFrontBuffer->BitsLength(), fFrontBuffer->BytesPerRow(), 0,
				fFrontBuffer->ColorSpace());
		}
	}

	return ret;
}


status_t
BitmapHWInterface::Shutdown()
{
	return B_OK;
}


status_t
BitmapHWInterface::SetMode(const display_mode& mode)
{
	return B_UNSUPPORTED;
}


void
BitmapHWInterface::GetMode(display_mode* mode)
{
	if (mode != NULL)
		memset(mode, 0, sizeof(display_mode));
}


status_t
BitmapHWInterface::GetDeviceInfo(accelerant_device_info* info)
{
	return B_UNSUPPORTED;
}


status_t
BitmapHWInterface::GetFrameBufferConfig(frame_buffer_config& config)
{
	return B_UNSUPPORTED;
}


status_t
BitmapHWInterface::GetModeList(display_mode** modes, uint32 *count)
{
	return B_UNSUPPORTED;
}


status_t
BitmapHWInterface::GetPixelClockLimits(display_mode* mode, uint32* low,
	uint32* high)
{
	return B_UNSUPPORTED;
}


status_t
BitmapHWInterface::GetTimingConstraints(display_timing_constraints* constraints)
{
	return B_UNSUPPORTED;
}


status_t
BitmapHWInterface::ProposeMode(display_mode* candidate, const display_mode* low,
	const display_mode* high)
{
	return B_UNSUPPORTED;
}


sem_id
BitmapHWInterface::RetraceSemaphore()
{
	return -1;
}


status_t
BitmapHWInterface::WaitForRetrace(bigtime_t timeout)
{
	return B_UNSUPPORTED;
}


status_t
BitmapHWInterface::SetDPMSMode(uint32 state)
{
	return B_UNSUPPORTED;
}


uint32
BitmapHWInterface::DPMSMode()
{
	return 0;
}


uint32
BitmapHWInterface::DPMSCapabilities()
{
	return 0;
}


status_t
BitmapHWInterface::SetBrightness(float)
{
	return B_UNSUPPORTED;
}


status_t
BitmapHWInterface::GetBrightness(float*)
{
	return B_UNSUPPORTED;
}


RenderingBuffer*
BitmapHWInterface::FrontBuffer() const
{
	return fFrontBuffer.Get();
}


RenderingBuffer*
BitmapHWInterface::BackBuffer() const
{
	return fBackBuffer.Get();
}


bool
BitmapHWInterface::IsDoubleBuffered() const
{
	// overwrite double buffered preference
	if (fFrontBuffer.Get() != NULL)
		return fBackBuffer.Get() != NULL;

	return HWInterface::IsDoubleBuffered();
}
