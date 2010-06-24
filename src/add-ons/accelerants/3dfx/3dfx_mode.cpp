/*
	Copyright 2010 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac
*/

/*
	Some of the code in this source file was adapted from the X.org tdfx
	video driver, and was covered by the following copyright and license.
	--------------------------------------------------------------------------

	Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
	All Rights Reserved.

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the
	"Software"), to deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sub license, and/or sell copies of the Software, and to
	permit persons to whom the Software is furnished to do so, subject to
	the following conditions:

	The above copyright notice and this permission notice (including the
	next paragraph) shall be included in all copies or substantial portions
	of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
	IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "accelerant.h"
#include "3dfx.h"

#include <stdlib.h>
#include <unistd.h>


// Functions to read/write PIO registers.
//=======================================

static void
WritePIOReg(uint32 offset, int16 index, uint8 value)
{
	PIORegInfo prInfo;
	prInfo.magic = TDFX_PRIVATE_DATA_MAGIC;
	prInfo.offset = offset;
	prInfo.index = index;
	prInfo.value = value;

	status_t result = ioctl(gInfo.deviceFileDesc, TDFX_SET_PIO_REG,
		&prInfo, sizeof(prInfo));
	if (result != B_OK)
		TRACE("WritePIOReg() failed, result = 0x%x\n", result);
}


static uint8
ReadPIOReg(uint32 offset, int16 index)
{
	PIORegInfo prInfo;
	prInfo.magic = TDFX_PRIVATE_DATA_MAGIC;
	prInfo.offset = offset;
	prInfo.index = index;

	status_t result = ioctl(gInfo.deviceFileDesc, TDFX_GET_PIO_REG,
		&prInfo, sizeof(prInfo));
	if (result != B_OK)
		TRACE("ReadPIOReg() failed, result = 0x%x\n", result);

	return prInfo.value;
}


static void
WriteMiscOutReg(uint8 value)
{
	WritePIOReg(MISC_OUT_W - 0x300, -1, value);
}


static void
WriteCrtcReg(uint8 index, uint8 value)
{
	WritePIOReg(CRTC_INDEX - 0x300, index, value);
}


uint8
ReadMiscOutReg()
{
	return ReadPIOReg(MISC_OUT_R - 0x300, -1);
}


uint8
ReadCrtcReg(uint8 index)
{
	return ReadPIOReg(CRTC_INDEX - 0x300, index);
}


void
TDFX_WaitForFifo(uint32 entries)
{
	// The FIFO has 32 slots.  This routine waits until at least `entries'
	// of these slots are empty.

	while ((INREG32(STATUS) & 0x1f) < entries) ;
}


void
TDFX_WaitForIdle()
{
	// Wait for the graphics engine to be completely idle.

	TDFX_WaitForFifo(1);
	OUTREG32(CMD_3D, CMD_3D_NOP);

	int i = 0;

	do {
		i = (INREG32(STATUS) & STATUS_BUSY) ? 0 : i + 1;
	} while (i < 3);
}


static int
TDFX_CalcPLL(int freq)
{
	const int refFreq = 14318;
	int best_error = freq;
	int best_k = 0;
	int best_m = 0;
	int best_n = 0;

	for (int n = 1; n < 256; n++) {
		int freqCur = refFreq * (n + 2);
		if (freqCur < freq) {
			freqCur = freqCur / 3;
			if (freq - freqCur < best_error) {
				best_error = freq - freqCur;
				best_n = n;
				best_m = 1;
				best_k = 0;
				continue;
			}
		}
		for (int m = 1; m < 64; m++) {
			for (int k = 0; k < 4; k++) {
				freqCur = refFreq * (n + 2) / (m + 2) / (1 << k);
				if (abs(freqCur - freq) < best_error) {
					best_error = abs(freqCur - freq);
					best_n = n;
					best_m = m;
					best_k = k;
				}
			}
		}
	}

	return (best_n << 8) | (best_m << 2) | best_k;
}


bool
TDFX_GetColorSpaceParams(int colorSpace, uint8& bitsPerPixel)
{
	// Get parameters for a color space which is supported by the 3dfx chips.
	// Return true if the color space is supported;  else return false.

	switch (colorSpace) {
		case B_RGB32:
			bitsPerPixel = 32;
			break;
		case B_RGB16:
			bitsPerPixel = 16;
			break;
		case B_RGB15:
			bitsPerPixel = 15;
			break;
		case B_CMAP8:
			bitsPerPixel = 8;
			break;
		default:
			TRACE("Unsupported color space: 0x%X\n", colorSpace);
			return false;
	}

	return true;
}


status_t
TDFX_SetDisplayMode(const DisplayModeEx& mode)
{
	// The code to actually configure the display.
	// All the error checking must be done in ProposeDisplayMode(),
	// and assume that the mode values we get here are acceptable.

	SharedInfo& si = *gInfo.sharedInfo;
	bool clock2X = mode.timing.pixel_clock > si.maxPixelClock / 2;

	// Initialize the timing values for CRTC registers cr00 to cr18.  Note
	// that the number following the letters 'cr' is a hexadecimal number.
	// Argument crtc will contain registers cr00 to cr18;  thus, it must
	// contain at least 25 (0x19) elements.

	// Normally the horizontal timing values are divided by 8;  however,
	// if the clock is above the 2X value, divide by 16 such that the values
	// are halved.

	int horzDiv = clock2X ? 16 : 8;

	int hTotal = mode.timing.h_total / horzDiv - 5;
	int hDisp_e = mode.timing.h_display / horzDiv - 1;
	int hSync_s = mode.timing.h_sync_start / horzDiv;
	int hSync_e = mode.timing.h_sync_end / horzDiv;
	int hBlank_s = hDisp_e + 1;		// start of horizontal blanking
	int hBlank_e = hTotal + 3;		// end of horizontal blanking

	int vTotal = mode.timing.v_total - 2;
	int vDisp_e = mode.timing.v_display - 1;
	int vSync_s = mode.timing.v_sync_start;
	int vSync_e = mode.timing.v_sync_end;
	int vBlank_s = vDisp_e + 1;		// start of vertical blanking
	int vBlank_e = vTotal;			// end of vertical blanking

	// CRTC Controller values

	uint8 crtc[25];

	crtc[0x00] = hTotal;
	crtc[0x01] = hDisp_e;
	crtc[0x02] = hBlank_s;
	crtc[0x03] = (hBlank_e & 0x1f) | 0x80;
	crtc[0x04] = hSync_s;
	crtc[0x05] = ((hSync_e & 0x1f) | ((hBlank_e & 0x20) << 2));
	crtc[0x06] = vTotal;
	crtc[0x07] = (((vTotal & 0x100) >> 8)
		| ((vDisp_e & 0x100) >> 7)
		| ((vSync_s & 0x100) >> 6)
		| ((vBlank_s & 0x100) >> 5)
		| 0x10
		| ((vTotal & 0x200) >> 4)
		| ((vDisp_e & 0x200) >> 3)
		| ((vSync_s & 0x200) >> 2));
	crtc[0x08] = 0x00;
	crtc[0x09] = ((vBlank_s & 0x200) >> 4) | 0x40;
	crtc[0x0a] = 0x00;
	crtc[0x0b] = 0x00;
	crtc[0x0c] = 0x00;
	crtc[0x0d] = 0x00;
	crtc[0x0e] = 0x00;
	crtc[0x0f] = 0x00;
	crtc[0x10] = vSync_s;
	crtc[0x11] = (vSync_e & 0x0f) | 0x20;
	crtc[0x12] = vDisp_e;
	crtc[0x13] = hDisp_e + 1;
	crtc[0x14] = 0x00;
	crtc[0x15] = vBlank_s;
	crtc[0x16] = vBlank_e;
	crtc[0x17] = 0xc3;
	crtc[0x18] = 0xff;

	// Set up the extra CR reg's to handle the higher resolution modes.

	uint8 cr1a = (hTotal & 0x100) >> 8
				| (hDisp_e & 0x100) >> 6
				| (hBlank_s & 0x100) >> 4
				| (hBlank_e & 0x40) >> 1
				| (hSync_s & 0x100) >> 2
				| (hSync_e & 0x20) << 2;

	uint8 cr1b = (vTotal & 0x400) >> 10
				| (vDisp_e & 0x400) >> 8
				| (vBlank_s & 0x400) >> 6
				| (vBlank_e & 0x400) >> 4;

	uint8 miscOutReg = 0x0f | (mode.timing.v_display < 400 ? 0xa0
							: mode.timing.v_display < 480 ? 0x60
							: mode.timing.v_display < 768 ? 0xe0 : 0x20);

	uint32 vgaInit0 = VGA0_EXTENSIONS
					| WAKEUP_3C3
					| ENABLE_ALT_READBACK
					| CLUT_SELECT_8BIT
					| EXT_SHIFT_OUT;

	uint32 videoConfig = VIDEO_PROCESSOR_ENABLE | DESKTOP_ENABLE
					| (mode.bytesPerPixel - 1) << DESKTOP_PIXEL_FORMAT_SHIFT
					| (mode.bytesPerPixel > 1 ? DESKTOP_CLUT_BYPASS : 0);

	uint32 dacMode = INREG32(DAC_MODE) & ~DAC_MODE_2X;

	if (clock2X) {
		dacMode |= DAC_MODE_2X;
		videoConfig |= VIDEO_2X_MODE_ENABLE;
	}

	uint32 pllFreq = TDFX_CalcPLL(mode.timing.pixel_clock);

	// Note that for the Banshee chip, the mode 1280x1024 at 60Hz refresh does
	// not display properly using the computed PLL frequency;  thus, set it to
	// the value that is computed when set by VESA.

	if (si.chipType == BANSHEE && pllFreq == 45831
			&& mode.timing.h_display == 1280 && mode.timing.v_display == 1024)
		pllFreq = 45912;

	uint32 screenSize = mode.timing.h_display | (mode.timing.v_display << 12);

	// Now that the values for the registers have been computed, write the
	// registers to set the mode.
	//=====================================================================

	TDFX_WaitForFifo(2);
	OUTREG32(VIDEO_PROC_CONFIG, 0);
	OUTREG32(PLL_CTRL0, pllFreq);

	WriteMiscOutReg(miscOutReg);

	for (uint8 j = 0; j < 25; j++)
		WriteCrtcReg(j, crtc[j]);

	WriteCrtcReg(0x1a, cr1a);
	WriteCrtcReg(0x1b, cr1b);

	TDFX_WaitForFifo(6);
	OUTREG32(VGA_INIT0, vgaInit0);
	OUTREG32(DAC_MODE, dacMode);
	OUTREG32(VIDEO_DESKTOP_OVERLAY_STRIDE, mode.bytesPerRow);
	OUTREG32(HW_CURSOR_PAT_ADDR, si.cursorOffset);
	OUTREG32(VIDEO_SCREEN_SIZE, screenSize);
	OUTREG32(VIDEO_DESKTOP_START_ADDR, si.frameBufferOffset);

	TDFX_WaitForFifo(7);
	OUTREG32(CLIP0_MIN, 0);
	OUTREG32(CLIP0_MAX, 0x0fff0fff);
	OUTREG32(CLIP1_MIN, 0);
	OUTREG32(CLIP1_MAX, 0x0fff0fff);
	OUTREG32(VIDEO_PROC_CONFIG, videoConfig);
	OUTREG32(SRC_BASE_ADDR, si.frameBufferOffset);
	OUTREG32(DST_BASE_ADDR, si.frameBufferOffset);

	TDFX_WaitForIdle();

	TDFX_AdjustFrame(mode);

	return B_OK;
}


void
TDFX_AdjustFrame(const DisplayModeEx& mode)
{
	// Adjust start address in frame buffer.

	SharedInfo& si = *gInfo.sharedInfo;

	int address = (mode.v_display_start * mode.virtual_width
			+ mode.h_display_start) * mode.bytesPerPixel;

	address &= ~0x07;
	address += si.frameBufferOffset;

	TDFX_WaitForFifo(1);
	OUTREG32(VIDEO_DESKTOP_START_ADDR, address);
	return;
}


void
TDFX_SetIndexedColors(uint count, uint8 first, uint8* colorData, uint32 flags)
{
	// Set the indexed color palette for 8-bit color depth mode.

	(void)flags;		// avoid compiler warning for unused arg

	if (gInfo.sharedInfo->displayMode.space != B_CMAP8)
		return ;

	uint32 index = first;

	while (count--) {
		uint32 color = ((colorData[0] << 16) | (colorData[1] << 8) | colorData[2]);
		TDFX_WaitForFifo(2);
		OUTREG32(DAC_ADDR, index++);
		INREG32(DAC_ADDR);		// color not always set unless we read after write
		OUTREG32(DAC_DATA, color);

		colorData += 3;
	}
}
