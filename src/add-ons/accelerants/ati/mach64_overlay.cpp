/*
	Haiku ATI video driver adapted from the X.org ATI driver which has the
	following copyright:

	Copyright 2003 through 2004 by Marc Aurele La France, tsi@xfree86.org

	Copyright 2011 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac
*/

#include "accelerant.h"
#include "mach64.h"



bool
Mach64_DisplayOverlay(const overlay_window* window,
						const overlay_buffer* buffer)
{
	// Return true if setup is successful.

	SharedInfo& si = *gInfo.sharedInfo;

	if (window == NULL || buffer == NULL)
		return false;

	uint32 videoFormat;

	if (buffer->space == B_YCbCr422)
		videoFormat = SCALE_IN_VYUY422;
	else
		return false;	// color space not supported

	int32 x1 = (window->h_start < 0) ? 0 : window->h_start;
	int32 y1 = (window->v_start < 0) ? 0 : window->v_start;

	int32 x2 = window->h_start + window->width - 1;
	int32 y2 = window->v_start + window->height - 1;

	if (x2 > si.displayMode.timing.h_display)
		x2 = si.displayMode.timing.h_display;

	if (y2 > si.displayMode.timing.v_display)
		y2 = si.displayMode.timing.v_display;

	// If window is moved beyond edge of screen, do not allow width < 4 or
	// height < 4;  otherwise there is a possibilty of divide by zero when
	// computing the scale factors..
	if (x2 < x1 + 4)
		x2 = x1 + 4;

	if (y2 < y1 + 4)
		y2 = y1 + 4;

	// Calculate overlay scale factors.
	uint32 horzScale = (buffer->width << 12) / (x2 - x1 + 1);
	uint32 vertScale = (buffer->height << 12) / (y2 - y1 + 1);
	
	if (horzScale > 0xffff)		// only 16 bits are used for scale factors
		horzScale = 0xffff;
	if (vertScale > 0xffff)
		vertScale = 0xffff;

	gInfo.WaitForFifo(2);
	OUTREG(BUS_CNTL, INREG(BUS_CNTL) | BUS_EXT_REG_EN);	// enable reg block 1
	OUTREG(OVERLAY_SCALE_CNTL, SCALE_EN);	// reset the video

	if (si.chipType >= MACH64_264GTPRO) {
		const uint32 brightness = 0;
		const uint32 saturation = 12;

		gInfo.WaitForFifo(6);
		OUTREG(SCALER_COLOUR_CNTL, brightness | saturation << 8
			| saturation << 16);
		OUTREG(SCALER_H_COEFF0, 0x0002000);
		OUTREG(SCALER_H_COEFF1, 0xd06200d);
		OUTREG(SCALER_H_COEFF2, 0xd0a1c0d);
		OUTREG(SCALER_H_COEFF3, 0xc0e1a0c);
		OUTREG(SCALER_H_COEFF4, 0xc14140c);
	}

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

	gInfo.WaitForFifo(3);
	OUTREG(OVERLAY_GRAPHICS_KEY_MSK, keyMask);
	OUTREG(OVERLAY_GRAPHICS_KEY_CLR, keyColor);
	OUTREG(OVERLAY_KEY_CNTL, OVERLAY_MIX_FALSE | OVERLAY_MIX_EQUAL);

	gInfo.WaitForFifo(8);
	OUTREG(OVERLAY_Y_X_START, OVERLAY_LOCK_START | (x1 << 16) | y1);
	OUTREG(OVERLAY_Y_X_END, (x2 << 16) | y2);
	OUTREG(OVERLAY_SCALE_INC, (horzScale << 16) | vertScale);
	OUTREG(SCALER_HEIGHT_WIDTH, (buffer->width << 16) | buffer->height);
	OUTREG(VIDEO_FORMAT, videoFormat);

	// Compute offset of overlay buffer in the video memory.
	uint32 offset = uint32(buffer->buffer) - si.videoMemAddr;
	
	if (si.chipType < MACH64_264VTB) {
		OUTREG(BUF0_OFFSET, offset);
		OUTREG(BUF0_PITCH, buffer->width);
	} else {
		OUTREG(SCALER_BUF0_OFFSET, offset);
		OUTREG(SCALER_BUF0_PITCH, buffer->width);
	}

	OUTREG(OVERLAY_SCALE_CNTL, SCALE_PIX_EXPAND | OVERLAY_EN | SCALE_EN);

	return true;
}


void
Mach64_StopOverlay(void)
{
	OUTREG(OVERLAY_SCALE_CNTL, SCALE_EN);	// reset the video
}
