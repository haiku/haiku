/*
	Haiku ATI video driver adapted from the X.org ATI driver which has the
	following copyright:

	Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
						 Precision Insight, Inc., Cedar Park, Texas, and
						 VA Linux Systems Inc., Fremont, California.

	Copyright 2011 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac
 */

#include "accelerant.h"
#include "rage128.h"


static uint32 sCurrentKeyColor = 0;
static uint32 sCurrentKeyMask = 0;



bool
Rage128_DisplayOverlay(const overlay_window* window,
						const overlay_buffer* buffer)
{
	// Return true if setup is successful.

	SharedInfo& si = *gInfo.sharedInfo;

	if (window == NULL || buffer == NULL)
		return false;

	if (buffer->space != B_YCbCr422)
		return false;	// color space not supported

	uint32 keyColor = 0;
	uint32 keyMask = 0;

	switch (si.displayMode.bitsPerPixel) {
		case 15:
			keyMask = 0x7fff;
			keyColor = (window->blue.value & window->blue.mask) << 0
				| (window->green.value & window->green.mask) << 5
				| (window->red.value & window->red.mask) << 10;
				// 15 bit color has no alpha bits
			break;
		case 16:
			keyMask = 0xffff;
			keyColor = (window->blue.value & window->blue.mask) << 0
				| (window->green.value & window->green.mask) << 5
				| (window->red.value & window->red.mask) << 11;
				// 16 bit color has no alpha bits
			break;
		default:
			keyMask = 0xffffffff;
			keyColor = (window->blue.value & window->blue.mask) << 0
				| (window->green.value & window->green.mask) << 8
				| (window->red.value & window->red.mask) << 16
				| (window->alpha.value & window->alpha.mask) << 24;
			break;
	}

	// If the key color or key mask has changed since the overlay was
	// previously initialized, initialize it again.  This is to avoid
	// initializing the overlay video everytime the overlay buffer is
	// switched which causes artifacts in the overlay display.

	if (keyColor != sCurrentKeyColor || keyMask != sCurrentKeyMask)
	{
		TRACE("Rage128_DisplayOverlay() initializing overlay video\n");
		
		// Initialize the overlay video by first resetting the video.

		OUTREG(R128_OV0_SCALE_CNTL, 0);
		OUTREG(R128_OV0_EXCLUSIVE_HORZ, 0);
		OUTREG(R128_OV0_AUTO_FLIP_CNTL, 0);
		OUTREG(R128_OV0_FILTER_CNTL, 0x0000000f);
	
		const uint32 brightness = 0;
		const uint32 saturation = 16;
		OUTREG(R128_OV0_COLOUR_CNTL, brightness | saturation << 8
			| saturation << 16);
	
		OUTREG(R128_OV0_GRAPHICS_KEY_MSK, keyMask);
		OUTREG(R128_OV0_GRAPHICS_KEY_CLR, keyColor);
		OUTREG(R128_OV0_KEY_CNTL, R128_GRAPHIC_KEY_FN_NE);
		OUTREG(R128_OV0_TEST, 0);

		sCurrentKeyColor = keyColor;
		sCurrentKeyMask = keyMask;
	}

	uint32 ecpDiv;
	if (si.displayMode.timing.pixel_clock < 125000)
		ecpDiv = 0;
	else if (si.displayMode.timing.pixel_clock < 250000)
		ecpDiv = 1;
	else
		ecpDiv = 2;

	SetPLLReg(R128_VCLK_ECP_CNTL, ecpDiv << 8, R128_ECP_DIV_MASK);

	int32 vertInc = (buffer->height << 20) / window->height;
	int32 horzInc = (buffer->width << (12 + ecpDiv)) / window->width;
	int32 stepBy = 1;

	while (horzInc >= (2 << 12)) {
		stepBy++;
		horzInc >>= 1;
	}

	int32 x1 = window->h_start;
	int32 y1 = window->v_start;

	int32 x2 = window->h_start + window->width;
	int32 y2 = window->v_start + window->height;

	int32 left = x1;
	int32 tmp = (left & 0x0003ffff) + 0x00028000 + (horzInc << 3);
	int32 p1_h_accum_init = ((tmp << 4) & 0x000f8000) |
					  ((tmp << 12) & 0xf0000000);

	tmp = ((left >> 1) & 0x0001ffff) + 0x00028000 + (horzInc << 2);
	int32 p23_h_accum_init = ((tmp << 4) & 0x000f8000) |
						((tmp << 12) & 0x70000000);

	tmp = (y1 & 0x0000ffff) + 0x00018000;
	int32 p1_v_accum_init = ((tmp << 4) & 0x03ff8000) | 0x00000001;

	// Compute offset of overlay buffer in the video memory.
	uint32 offset = (uint32)((addr_t)buffer->buffer - si.videoMemAddr);

	OUTREG(R128_OV0_REG_LOAD_CNTL, 1);
	while (!(INREG(R128_OV0_REG_LOAD_CNTL) & (1 << 3)))
		;

	OUTREG(R128_OV0_H_INC, horzInc | ((horzInc >> 1) << 16));
	OUTREG(R128_OV0_STEP_BY, stepBy | (stepBy << 8));
	OUTREG(R128_OV0_Y_X_START, x1 | y1 << 16);
	OUTREG(R128_OV0_Y_X_END, x2 | y2 << 16);
	OUTREG(R128_OV0_V_INC, vertInc);
	OUTREG(R128_OV0_P1_BLANK_LINES_AT_TOP,
			0x00000fff | ((buffer->height - 1) << 16));
	OUTREG(R128_OV0_VID_BUF_PITCH0_VALUE, buffer->bytes_per_row);

	int32 width = window->width;
	left = 0;
	OUTREG(R128_OV0_P1_X_START_END, (width - 1) | (left << 16));
	width >>= 1;
	OUTREG(R128_OV0_P2_X_START_END, (width - 1) | (left << 16));
	OUTREG(R128_OV0_P3_X_START_END, (width - 1) | (left << 16));
	OUTREG(R128_OV0_VID_BUF0_BASE_ADRS, offset);
	OUTREG(R128_OV0_P1_V_ACCUM_INIT, p1_v_accum_init);
	OUTREG(R128_OV0_P23_V_ACCUM_INIT, 0);
	OUTREG(R128_OV0_P1_H_ACCUM_INIT, p1_h_accum_init);
	OUTREG(R128_OV0_P23_H_ACCUM_INIT, p23_h_accum_init);

	OUTREG(R128_OV0_SCALE_CNTL, 0x41FF8B03);
	OUTREG(R128_OV0_REG_LOAD_CNTL, 0);

	return true;
}


void
Rage128_StopOverlay(void)
{
	OUTREG(R128_OV0_SCALE_CNTL, 0);		// reset the video

	// Reset the key color and mask so that when the overlay is started again
	// it will be initialized.
	
	sCurrentKeyColor = 0;
	sCurrentKeyMask = 0;
}
