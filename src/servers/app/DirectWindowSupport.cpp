/*
 * Copyright 2008-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include "DirectWindowSupport.h"

#include <Autolock.h>

#include "RenderingBuffer.h"
#include "clipping.h"

#include <string.h>
#include <syslog.h>


DirectWindowData::DirectWindowData()
	:
	buffer_info(NULL),
	full_screen(false),
	fSem(-1),
	fAcknowledgeSem(-1),
	fBufferArea(-1),
	fTransition(0)
{
	fBufferArea = create_area("direct area", (void**)&buffer_info,
		B_ANY_ADDRESS, DIRECT_BUFFER_INFO_AREA_SIZE,
		B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

	memset(buffer_info, 0, DIRECT_BUFFER_INFO_AREA_SIZE);
	buffer_info->buffer_state = B_DIRECT_STOP;
	fSem = create_sem(0, "direct sem");
	fAcknowledgeSem = create_sem(0, "direct sem ack");
}


DirectWindowData::~DirectWindowData()
{
	// this should make the client die in case it's still running
	buffer_info->bits = NULL;
	buffer_info->bytes_per_row = 0;

	delete_area(fBufferArea);
	delete_sem(fSem);
	delete_sem(fAcknowledgeSem);
}


status_t
DirectWindowData::InitCheck() const
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
DirectWindowData::GetSyncData(direct_window_sync_data& data) const
{
	data.area = fBufferArea;
	data.disable_sem = fSem;
	data.disable_sem_ack = fAcknowledgeSem;

	return B_OK;
}


status_t
DirectWindowData::_SyncronizeWithClient()
{
	// Releasing this semaphore causes the client to call
	// BDirectWindow::DirectConnected()
	status_t status = release_sem(fSem);
	if (status != B_OK)
		return status;

	// Wait with a timeout of half a second until the client exits
	// from its DirectConnected() implementation
	do {
#if 0
		status = acquire_sem(fAcknowledgeSem);
#else	
		status = acquire_sem_etc(fAcknowledgeSem, 1, B_TIMEOUT, 3000000);
#endif
	} while (status == B_INTERRUPTED);

	return status;
}


status_t
DirectWindowData::SetState(const direct_buffer_state& bufferState,
	const direct_driver_state& driverState, RenderingBuffer *buffer,
	const BRect& windowFrame, const BRegion& clipRegion)
{	
	bool wasStopped = fTransition <= 0;
	
	if ((bufferState & B_DIRECT_MODE_MASK) == B_DIRECT_STOP)
		fTransition--;
	else if ((bufferState & B_DIRECT_MODE_MASK) == B_DIRECT_START)
		fTransition++;
	
	bool isStopped = fTransition <= 0;
		
	if (wasStopped && isStopped) {
		// Nothing to change
		return B_OK;
	}
						
	buffer_info->buffer_state = bufferState;
		
	if (driverState != -1)
		buffer_info->driver_state = driverState;

	if ((bufferState & B_DIRECT_MODE_MASK) != B_DIRECT_STOP) {
		buffer_info->bits = buffer->Bits();
		buffer_info->pci_bits = NULL; // TODO
		buffer_info->bytes_per_row = buffer->BytesPerRow();

		switch (buffer->ColorSpace()) {
			case B_RGB32:
			case B_RGBA32:
			case B_RGB32_BIG:
			case B_RGBA32_BIG:
				buffer_info->bits_per_pixel = 32;
				break;
			case B_RGB24:
			case B_RGB24_BIG:
				buffer_info->bits_per_pixel = 24;
				break;
			case B_RGB16:
			case B_RGB16_BIG:
			case B_RGB15:
			case B_RGB15_BIG:
				buffer_info->bits_per_pixel = 16;
				break;
			case B_CMAP8:
			case B_GRAY8:
				buffer_info->bits_per_pixel = 8;
				break;
			default:
				syslog(LOG_ERR,
					"unknown colorspace in DirectWindowData::SetState()!\n");
				buffer_info->bits_per_pixel = 0;
				break;
		}

		buffer_info->pixel_format = buffer->ColorSpace();
		buffer_info->layout = B_BUFFER_NONINTERLEAVED;
		buffer_info->orientation = B_BUFFER_TOP_TO_BOTTOM;
			// TODO
		buffer_info->window_bounds = to_clipping_rect(windowFrame);

		// TODO: Review this
		const int32 kMaxClipRectsCount = (DIRECT_BUFFER_INFO_AREA_SIZE
			- sizeof(direct_buffer_info)) / sizeof(clipping_rect);

		buffer_info->clip_list_count = min_c(clipRegion.CountRects(),
			kMaxClipRectsCount);
		buffer_info->clip_bounds = clipRegion.FrameInt();

		for (uint32 i = 0; i < buffer_info->clip_list_count; i++)
			buffer_info->clip_list[i] = clipRegion.RectAtInt(i);
	}
	
	return _SyncronizeWithClient();
}
