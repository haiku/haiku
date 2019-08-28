/*
 * Copyright 2010 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

#include "accelerant.h"
#include "3dfx.h"

#include <stdlib.h>



uint32
OverlayCount(const display_mode *mode)
{
	(void)mode;		// avoid compiler warning for unused arg

	return 1;
}


const uint32*
OverlaySupportedSpaces(const display_mode* mode)
{
	(void)mode;		// avoid compiler warning for unused arg

	static const uint32 kSupportedSpaces[] = {B_RGB16, B_YCbCr422, 0};

	return kSupportedSpaces;
}


uint32
OverlaySupportedFeatures(uint32 colorSpace)
{
	(void)colorSpace;		// avoid compiler warning for unused arg

	return B_OVERLAY_COLOR_KEY
		| B_OVERLAY_HORIZONTAL_FILTERING
		| B_OVERLAY_VERTICAL_FILTERING;
}


const overlay_buffer*
AllocateOverlayBuffer(color_space colorSpace, uint16 width, uint16 height)
{
	SharedInfo& si = *gInfo.sharedInfo;

	TRACE("AllocateOverlayBuffer() width %u, height %u, colorSpace 0x%lx\n",
		width, height, colorSpace);

	si.overlayLock.Acquire();

	// Note:  When allocating buffers, buffer allocation starts at the end of
	// video memory, and works backward to the end of the video frame buffer.
	// The allocated buffers are recorded in a linked list of OverlayBuffer
	// objects which are ordered by the buffer address with the first object
	// in the list having the highest buffer address.

	uint32 bytesPerPixel;

	switch (colorSpace) {
		case B_YCbCr422:
		case B_RGB16:
			bytesPerPixel = 2;
			break;
		default:
			si.overlayLock.Release();
			TRACE("AllocateOverlayBuffer() unsupported color space 0x%x\n",
				colorSpace);
			return NULL;
	}

	// Calculate required buffer size as a multiple of 1K.
	uint32 buffSize = (width * bytesPerPixel * height + 0x3ff) & ~0x3ff;

	// If the buffer area starts at the end of the video memory, the Voodoo5
	// chip has the following display problems:  displays a pink/red band at
	// the bottom of the overlay display, leaves some artifacts at the top of
	// screen, and messes up the displayed cursor when a hardware cursor is 
	// used (cursor image is at beginning of video memory).  I don't know
	// whether the Voodoo5 goes beyond the buffer area or whether this is some
	// sort of memory mapping problem;  nevertheless, adding 4k or 8K to the
	// buffer size solves the problem.  Thus, 16K is added to the buffer size.

	if (si.chipType == VOODOO_5)
		buffSize += 16 * 1024;

	OverlayBuffer* ovBuff = si.overlayBuffer;
	OverlayBuffer* prevOvBuff = NULL;

	// If no buffers have been allocated, prevBuffAddr calculated here will be
	// the address where the buffer area will start.

	addr_t prevBuffAddr = si.videoMemAddr + si.frameBufferOffset
						+ si.maxFrameBufferSize;

	while (ovBuff != NULL) {
		// Test if there is sufficient space between the end of the current
		// buffer and the start of the previous buffer to allocate the new
		// buffer.

		addr_t currentBuffEndAddr = (addr_t)ovBuff->buffer + ovBuff->size;
		if ((prevBuffAddr - currentBuffEndAddr) >= buffSize)
			break;		// sufficient space for the new buffer

		prevBuffAddr = (addr_t)ovBuff->buffer;
		prevOvBuff = ovBuff;
		ovBuff = ovBuff->nextBuffer;
	}

	OverlayBuffer* nextOvBuff = ovBuff;

	if (ovBuff == NULL) {
		// No space between any current buffers of the required size was found;
		// thus space must be allocated between the last buffer and the end of
		// the video frame buffer.  Compute where current video frame buffer
		// ends so that it can be determined if there is sufficient space for
		// the new buffer to be created.

		addr_t fbEndAddr = si.videoMemAddr + si.frameBufferOffset
			+ (si.displayMode.virtual_width * si.displayMode.bytesPerPixel
			* si.displayMode.virtual_height);

		if (buffSize > prevBuffAddr - fbEndAddr) {
			si.overlayLock.Release();
			TRACE("AllocateOverlayBuffer() insuffcient space for %ld (0x%lx) "
					"byte buffer\n", buffSize, buffSize);
			return NULL;	// insufficient space for buffer
		}

		nextOvBuff = NULL;
	}

	ovBuff = (OverlayBuffer*)malloc(sizeof(OverlayBuffer));
	if (ovBuff == NULL) {
		si.overlayLock.Release();
		return NULL;		// memory not available for OverlayBuffer struct
	}

	ovBuff->nextBuffer = nextOvBuff;
	ovBuff->size = buffSize;
	ovBuff->space = colorSpace;
	ovBuff->width = width;
	ovBuff->height = height;
	ovBuff->bytes_per_row = width * bytesPerPixel;
	ovBuff->buffer = (void*)(prevBuffAddr - buffSize);
	ovBuff->buffer_dma = (void*)(si.videoMemPCI
		+ ((addr_t)ovBuff->buffer - si.videoMemAddr));

	if (prevOvBuff == NULL)
		si.overlayBuffer = ovBuff;
	else
		prevOvBuff->nextBuffer = ovBuff;

	si.overlayLock.Release();
	TRACE("AllocateOverlayBuffer() allocated %ld (0x%lx) byte buffer at 0x%lx\n",
		buffSize, buffSize, ovBuff->buffer);
	return ovBuff;
}


status_t
ReleaseOverlayBuffer(const overlay_buffer* buffer)
{
	SharedInfo& si = *gInfo.sharedInfo;

	TRACE("ReleaseOverlayBuffer() called\n");

	if (buffer == NULL)
		return B_BAD_VALUE;

	// Find the buffer to be released.

	OverlayBuffer* ovBuff = si.overlayBuffer;
	OverlayBuffer* prevOvBuff = NULL;

	while (ovBuff != NULL) {
		if (ovBuff->buffer == buffer->buffer) {
			// Buffer to be released has been found.  Remove the OverlayBuffer
			// object from the chain of overlay buffers.

			if (prevOvBuff == NULL)
				si.overlayBuffer = ovBuff->nextBuffer;
			else
				prevOvBuff->nextBuffer = ovBuff->nextBuffer;

			free(ovBuff);
			TRACE("ReleaseOverlayBuffer() return OK\n");
			return B_OK;
		}

		prevOvBuff = ovBuff;
		ovBuff = ovBuff->nextBuffer;
	}

	return B_ERROR;		// buffer to be released not found in chain of buffers
}


status_t
GetOverlayConstraints(const display_mode* mode, const overlay_buffer* buffer,
					overlay_constraints* constraints)
{
	if ((mode == NULL) || (buffer == NULL) || (constraints == NULL))
		return B_ERROR;

	// Position (values are in pixels)
	constraints->view.h_alignment = 0;
	constraints->view.v_alignment = 0;

	switch (buffer->space) {
		case B_YCbCr422:
		case B_RGB16:
			constraints->view.width_alignment = 7;
			break;
		default:
			TRACE("GetOverlayConstraints() color space 0x%x out of range\n",
				buffer->space);
			return B_BAD_VALUE;
	}

	constraints->view.height_alignment = 0;

	//Size
	constraints->view.width.min = 4;
	constraints->view.height.min = 4;
	constraints->view.width.max = buffer->width;
	constraints->view.height.max = buffer->height;

	// Scaler output restrictions
	constraints->window.h_alignment = 0;
	constraints->window.v_alignment = 0;
	constraints->window.width_alignment = 0;
	constraints->window.height_alignment = 0;
	constraints->window.width.min = 2;
	constraints->window.width.max = mode->virtual_width;
	constraints->window.height.min = 2;
	constraints->window.height.max = mode->virtual_height;

	constraints->h_scale.min = 1.0;
	constraints->h_scale.max = 8.0;
	constraints->v_scale.min = 1.0;
	constraints->v_scale.max = 8.0;

	return B_OK;
}


overlay_token
AllocateOverlay(void)
{
	SharedInfo& si = *gInfo.sharedInfo;

	TRACE("AllocateOverlay() called\n");

	// There is only a single overlay channel;  thus, check if it is already
	// allocated.

	if (atomic_or(&si.overlayAllocated, 1) != 0) {
		TRACE("AllocateOverlay() overlay channel already in use\n");
		return NULL;
	}
	TRACE("AllocateOverlay() Overlay allocated, overlayToken: %d\n",
		si.overlayToken);
	return (overlay_token)++si.overlayToken;
}


status_t
ReleaseOverlay(overlay_token overlayToken)
{
	SharedInfo& si = *gInfo.sharedInfo;

	TRACE("ReleaseOverlay() called\n");

	if (overlayToken != (overlay_token)si.overlayToken)
		return B_BAD_VALUE;

	TDFX_StopOverlay();

	atomic_and(&si.overlayAllocated, 0);	// mark overlay as unallocated

	TRACE("ReleaseOverlay() return OK\n");
	return B_OK;
}


status_t
ConfigureOverlay (overlay_token overlayToken, const overlay_buffer* buffer,
	const overlay_window* window, const overlay_view* view)
{
	SharedInfo& si = *gInfo.sharedInfo;

	if (overlayToken != (overlay_token)si.overlayToken)
		return B_BAD_VALUE;

	if (buffer == NULL)
		return B_BAD_VALUE;

	if (window == NULL || view == NULL) {
		TDFX_StopOverlay();
		TRACE("ConfigureOverlay() hide only\n");
		return B_OK;
	}

	// Program the overlay hardware.
	if (!TDFX_DisplayOverlay(window, buffer, view)) {
		TRACE("ConfigureOverlay(), call to TDFX_DisplayOverlay() returned error\n");
		return B_ERROR;
	}

	return B_OK;
}
