/*
 * Copyright 2008-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "DirectWindowInfo.h"

#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <Autolock.h>

#include "RenderingBuffer.h"
#include "clipping.h"


DirectWindowInfo::DirectWindowInfo()
	:
	fBufferInfo(NULL),
	fSem(-1),
	fAcknowledgeSem(-1),
	fBufferArea(-1),
	fOriginalFeel(B_NORMAL_WINDOW_FEEL),
	fFullScreen(false)
{
	fBufferArea = create_area("direct area", (void**)&fBufferInfo,
		B_ANY_ADDRESS, DIRECT_BUFFER_INFO_AREA_SIZE,
		B_NO_LOCK, B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA);

	memset(fBufferInfo, 0, DIRECT_BUFFER_INFO_AREA_SIZE);
	fBufferInfo->buffer_state = B_DIRECT_STOP;

	fSem = create_sem(0, "direct sem");
	fAcknowledgeSem = create_sem(0, "direct sem ack");
}


DirectWindowInfo::~DirectWindowInfo()
{
	// this should make the client die in case it's still running
	fBufferInfo->bits = NULL;
	fBufferInfo->bytes_per_row = 0;

	delete_area(fBufferArea);
	delete_sem(fSem);
	delete_sem(fAcknowledgeSem);
}


status_t
DirectWindowInfo::InitCheck() const
{
	if (fBufferArea < B_OK)
		return fBufferArea;
	if (fSem < B_OK)
		return fSem;
	if (fAcknowledgeSem < B_OK)
		return fAcknowledgeSem;

	return B_OK;
}


status_t
DirectWindowInfo::GetSyncData(direct_window_sync_data& data) const
{
	data.area = fBufferArea;
	data.disable_sem = fSem;
	data.disable_sem_ack = fAcknowledgeSem;

	return B_OK;
}


status_t
DirectWindowInfo::SetState(direct_buffer_state bufferState,
	direct_driver_state driverState, RenderingBuffer* buffer,
	const BRect& windowFrame, const BRegion& clipRegion)
{
	if ((fBufferInfo->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_STOP
		&& (bufferState & B_DIRECT_MODE_MASK) != B_DIRECT_START)
		return B_OK;

	fBufferInfo->buffer_state = bufferState;

	if ((int)driverState != -1)
		fBufferInfo->driver_state = driverState;

	if ((bufferState & B_DIRECT_MODE_MASK) != B_DIRECT_STOP) {
		fBufferInfo->bits = buffer->Bits();
		fBufferInfo->pci_bits = NULL; // TODO
		fBufferInfo->bytes_per_row = buffer->BytesPerRow();

		switch (buffer->ColorSpace()) {
			case B_RGBA64:
			case B_RGBA64_BIG:
				fBufferInfo->bits_per_pixel = 64;
				break;
			case B_RGB48:
			case B_RGB48_BIG:
				fBufferInfo->bits_per_pixel = 48;
				break;
			case B_RGB32:
			case B_RGBA32:
			case B_RGB32_BIG:
			case B_RGBA32_BIG:
				fBufferInfo->bits_per_pixel = 32;
				break;
			case B_RGB24:
			case B_RGB24_BIG:
				fBufferInfo->bits_per_pixel = 24;
				break;
			case B_RGB16:
			case B_RGB16_BIG:
			case B_RGB15:
			case B_RGB15_BIG:
				fBufferInfo->bits_per_pixel = 16;
				break;
			case B_CMAP8:
			case B_GRAY8:
				fBufferInfo->bits_per_pixel = 8;
				break;
			default:
				syslog(LOG_ERR,
					"unknown colorspace in DirectWindowInfo::SetState()!\n");
				fBufferInfo->bits_per_pixel = 0;
				break;
		}

		fBufferInfo->pixel_format = buffer->ColorSpace();
		fBufferInfo->layout = B_BUFFER_NONINTERLEAVED;
		fBufferInfo->orientation = B_BUFFER_TOP_TO_BOTTOM;
			// TODO
		fBufferInfo->window_bounds = to_clipping_rect(windowFrame);

		const int32 kMaxClipRectsCount = (DIRECT_BUFFER_INFO_AREA_SIZE
			- sizeof(direct_buffer_info)) / sizeof(clipping_rect);

		fBufferInfo->clip_list_count = min_c(clipRegion.CountRects(),
			kMaxClipRectsCount);
		fBufferInfo->clip_bounds = clipRegion.FrameInt();

		for (uint32 i = 0; i < fBufferInfo->clip_list_count; i++)
			fBufferInfo->clip_list[i] = clipRegion.RectAtInt(i);
	}

	return _SyncronizeWithClient();
}


void
DirectWindowInfo::EnableFullScreen(const BRect& frame, window_feel feel)
{
	fOriginalFrame = frame;
	fOriginalFeel = feel;
	fFullScreen = true;
}


void
DirectWindowInfo::DisableFullScreen()
{
	fFullScreen = false;
}


status_t
DirectWindowInfo::_SyncronizeWithClient()
{
	// Releasing this semaphore causes the client to call
	// BDirectWindow::DirectConnected()
	status_t status = release_sem(fSem);
	if (status != B_OK)
		return status;

	// Wait with a timeout of half a second until the client exits
	// from its DirectConnected() implementation
	do {
		status = acquire_sem_etc(fAcknowledgeSem, 1, B_TIMEOUT, 500000);
	} while (status == B_INTERRUPTED);

	return status;
}
