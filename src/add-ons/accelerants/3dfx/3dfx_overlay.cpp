/*
 * Copyright 2010 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

#include "accelerant.h"
#include "3dfx.h"



bool
TDFX_DisplayOverlay(const overlay_window* window,
						const overlay_buffer* buffer,
						const overlay_view* view)
{
	// Return true if setup is successful.

	SharedInfo& si = *gInfo.sharedInfo;

	if (window == NULL || buffer == NULL || view == NULL)
		return false;

	if (window->flags & B_OVERLAY_COLOR_KEY) {
		uint32 color = 0;

		if (si.displayMode.bitsPerPixel == 16) {
			color = (window->blue.value & window->blue.mask) << 0
				  | (window->green.value & window->green.mask) << 5
				  | (window->red.value & window->red.mask) << 11;
				  // 16 bit color has no alpha bits
		} else {
			color = (window->blue.value & window->blue.mask) << 0
				  | (window->green.value & window->green.mask) << 8
				  | (window->red.value & window->red.mask) << 16
				  | (window->alpha.value & window->alpha.mask) << 24;
		}

		TDFX_WaitForFifo(2);
		OUTREG32(VIDEO_CHROMA_MIN, color);
		OUTREG32(VIDEO_CHROMA_MAX, color);
	}

	uint32 videoConfig = INREG32(VIDEO_PROC_CONFIG);
	videoConfig &= ~VIDEO_PROC_CONFIG_MASK;
	videoConfig |= (0x00000320 | OVERLAY_CLUT_BYPASS);

	// Scale image if window dimension is larger than the buffer dimension.
	// Scaling is not done if window dimension is smaller since the chip only
	// scales up to a larger dimension, and does not scale down to a smaller
	// dimension.

	if (window->width > buffer->width)
		videoConfig |= (1 << 14);
	if (window->height > buffer->height)
		videoConfig |= (1 << 15);

	switch (buffer->space) {
		case B_YCbCr422:
			videoConfig |= VIDCFG_OVL_FMT_YUYV422;
			break;
		case B_RGB16:
			videoConfig |= VIDCFG_OVL_FMT_RGB565;
			break;
		default:
			return false;	// color space not supported
	}

	// can't do bilinear filtering when in 2X mode
	if ((videoConfig & VIDEO_2X_MODE_ENABLE) == 0)
		videoConfig |= (3 << 16);

	TDFX_WaitForFifo(1);
	OUTREG32(VIDEO_PROC_CONFIG, videoConfig);

	// Subtract 1 from height to eliminate junk on last line of image.
	int32 dudx = (buffer->width << 20) / window->width;
	int32 dudy = ((buffer->height - 1) << 20) / window->height;

	int32 x1 = (window->h_start < 0) ? 0 : window->h_start;
	int32 y1 = (window->v_start < 0) ? 0 : window->v_start;

	int32 x2 = x1 + window->width - 1;
	int32 y2 = y1 + window->height - 1;

	TDFX_WaitForFifo(6);

	// Set up coordinates of overlay window on screen.
	OUTREG32(VIDEO_OVERLAY_START_COORDS, x1 | (y1 << 12));
	OUTREG32(VIDEO_OVERLAY_END_COORDS, x2 | (y2 << 12));
	// Set up scale and position of overlay in graphics memory.
	OUTREG32(VIDEO_OVERLAY_DUDX, dudx);
	OUTREG32(VIDEO_OVERLAY_DUDX_OFFSET_SRC_WIDTH, ((x1 & 0x0001ffff) << 3)
		| (buffer->width << 20));
	OUTREG32(VIDEO_OVERLAY_DVDY, dudy);
	OUTREG32(VIDEO_OVERLAY_DVDY_OFFSET, (y1 & 0x0000ffff) << 3);

	// Add width of overlay buffer to stride.
	uint32 stride = INREG32(VIDEO_DESKTOP_OVERLAY_STRIDE) & 0x0000ffff;
	stride |= (buffer->width << 1) << 16;
	uint32 offset = (uint32)(addr_t)buffer->buffer_dma;

	TDFX_WaitForFifo(2);

	OUTREG32(VIDEO_DESKTOP_OVERLAY_STRIDE, stride);
	OUTREG32(VIDEO_IN_ADDR0, offset);

	return true;
}


void
TDFX_StopOverlay(void)
{
	// reset the video
	uint32 videoConfig = INREG32(VIDEO_PROC_CONFIG) & ~VIDEO_PROC_CONFIG_MASK;
	OUTREG32(VIDEO_PROC_CONFIG, videoConfig);
	OUTREG32(RGB_MAX_DELTA, 0x0080808);
}
