/*
 * Copyright 2006-2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Overlay.h"

#include <BitmapPrivate.h>

#include "HWInterface.h"
#include "ServerBitmap.h"


//#define TRACE_OVERLAY
#ifdef TRACE_OVERLAY
#	define TRACE(x...) ktrace_printf(x);
#else
#	define TRACE(x...) ;
#endif


const static bigtime_t kOverlayTimeout = 1000000LL;
	// after 1 second, the team holding the lock will be killed

class SemaphoreLocker {
public:
	SemaphoreLocker(sem_id semaphore, bigtime_t timeout = B_INFINITE_TIMEOUT)
		:
		fSemaphore(semaphore)
	{
		do {
			fStatus = acquire_sem_etc(fSemaphore, 1, B_RELATIVE_TIMEOUT,
				timeout);
		} while (fStatus == B_INTERRUPTED);
	}

	~SemaphoreLocker()
	{
		if (fStatus == B_OK)
			release_sem_etc(fSemaphore, 1, B_DO_NOT_RESCHEDULE);
	}

	status_t LockStatus()
	{
		return fStatus;
	}

private:
	sem_id		fSemaphore;
	status_t	fStatus;
};


//	#pragma mark -


Overlay::Overlay(HWInterface& interface, ServerBitmap* bitmap,
		overlay_token token)
	:
	fHWInterface(interface),
	fOverlayBuffer(NULL),
	fClientData(NULL),
	fOverlayToken(token)
{
	fSemaphore = create_sem(1, "overlay lock");
	fColor = (rgb_color){ 21, 16, 21, 16 };
		// TODO: whatever fine color we want to use here...

	fWindow.offset_top = 0;
	fWindow.offset_left = 0;
	fWindow.offset_right = 0;
	fWindow.offset_bottom = 0;

	fWindow.flags = B_OVERLAY_COLOR_KEY;

	_AllocateBuffer(bitmap);

	TRACE("overlay: created %p, bitmap %p\n", this, bitmap);
}


Overlay::~Overlay()
{
	fHWInterface.ReleaseOverlayChannel(fOverlayToken);
	_FreeBuffer();

	delete_sem(fSemaphore);
	TRACE("overlay: deleted %p\n", this);
}


status_t
Overlay::InitCheck() const
{
	if (fSemaphore < B_OK)
		return fSemaphore;

	if (fOverlayBuffer == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
Overlay::Resume(ServerBitmap* bitmap)
{
	SemaphoreLocker locker(fSemaphore, kOverlayTimeout);
	if (locker.LockStatus() == B_TIMED_OUT) {
		// TODO: kill app!
	}

	TRACE("overlay: resume %p (lock status %ld)\n", this, locker.LockStatus());

	status_t status = _AllocateBuffer(bitmap);
	if (status < B_OK)
		return status;

	fClientData->buffer = (uint8*)fOverlayBuffer->buffer;
	return B_OK;
}


status_t
Overlay::Suspend(ServerBitmap* bitmap, bool needTemporary)
{
	SemaphoreLocker locker(fSemaphore, kOverlayTimeout);
	if (locker.LockStatus() == B_TIMED_OUT) {
		// TODO: kill app!
	}

	TRACE("overlay: suspend %p (lock status %ld)\n", this, locker.LockStatus());

	_FreeBuffer();
	fClientData->buffer = NULL;

	return B_OK;
}


void
Overlay::_FreeBuffer()
{
	fHWInterface.FreeOverlayBuffer(fOverlayBuffer);
	fOverlayBuffer = NULL;
}


status_t
Overlay::_AllocateBuffer(ServerBitmap* bitmap)
{
	fOverlayBuffer = fHWInterface.AllocateOverlayBuffer(bitmap->Width(),
		bitmap->Height(), bitmap->ColorSpace());
	if (fOverlayBuffer == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
Overlay::SetClientData(overlay_client_data* clientData)
{
	fClientData = clientData;
	fClientData->lock = fSemaphore;
	fClientData->buffer = (uint8*)fOverlayBuffer->buffer;
}


void
Overlay::SetFlags(uint32 flags)
{
	if (flags & B_OVERLAY_FILTER_HORIZONTAL)
		fWindow.flags |= B_OVERLAY_HORIZONTAL_FILTERING;
	if (flags & B_OVERLAY_FILTER_VERTICAL)
		fWindow.flags |= B_OVERLAY_VERTICAL_FILTERING;
	if (flags & B_OVERLAY_MIRROR)
		fWindow.flags |= B_OVERLAY_HORIZONTAL_MIRRORING;
}


void
Overlay::TakeOverToken(Overlay* other)
{
	overlay_token token = other->OverlayToken();
	if (token == NULL)
		return;

	fOverlayToken = token;
	//other->fOverlayToken = NULL;
}


const overlay_buffer*
Overlay::OverlayBuffer() const
{
	return fOverlayBuffer;
}


overlay_client_data*
Overlay::ClientData() const
{
	return fClientData;
}


overlay_token
Overlay::OverlayToken() const
{
	return fOverlayToken;
}


void
Overlay::Hide()
{
	if (fOverlayToken == NULL)
		return;

	fHWInterface.HideOverlay(this);
	TRACE("overlay: hide %p\n", this);
}


void
Overlay::SetColorSpace(uint32 colorSpace)
{
	if ((fWindow.flags & B_OVERLAY_COLOR_KEY) == 0)
		return;

	uint8 colorShift = 0, greenShift = 0, alphaShift = 0;
	rgb_color colorKey = fColor;

	switch (colorSpace) {
		case B_CMAP8:
			colorKey.red = 0xff;
			colorKey.green = 0xff;
			colorKey.blue = 0xff;
			colorKey.alpha = 0xff;
			break;
		case B_RGB15:
			greenShift = colorShift = 3;
			alphaShift = 7;
			break;
		case B_RGB16:
			colorShift = 3;
			greenShift = 2;
			alphaShift = 8;
			break;
	}

	fWindow.red.value = colorKey.red >> colorShift;
	fWindow.green.value = colorKey.green >> greenShift;
	fWindow.blue.value = colorKey.blue >> colorShift;
	fWindow.alpha.value = colorKey.alpha >> alphaShift;
	fWindow.red.mask = 0xff >> colorShift;
	fWindow.green.mask = 0xff >> greenShift;
	fWindow.blue.mask = 0xff >> colorShift;
	fWindow.alpha.mask = 0xff >> alphaShift;
}


void
Overlay::Configure(const BRect& source, const BRect& destination)
{
	if (fOverlayToken == NULL) {
		fOverlayToken = fHWInterface.AcquireOverlayChannel();
		if (fOverlayToken == NULL)
			return;
	}

	TRACE("overlay: configure %p\n", this);

	fView.h_start = (uint16)source.left;
	fView.v_start = (uint16)source.top;
	fView.width = (uint16)source.IntegerWidth() + 1;
	fView.height = (uint16)source.IntegerHeight() + 1;

	fWindow.h_start = (int16)destination.left;
	fWindow.v_start = (int16)destination.top;
	fWindow.width = (uint16)destination.IntegerWidth() + 1;
	fWindow.height = (uint16)destination.IntegerHeight() + 1;

	fHWInterface.ConfigureOverlay(this);
}

