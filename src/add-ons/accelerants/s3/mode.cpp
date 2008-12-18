/*
	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007-2008
*/

#include "accel.h"

#include <create_display_modes.h>		// common accelerant header file
#include <string.h>
#include <unistd.h>


void 
InitCrtcTimingValues(const DisplayModeEx& mode, int horzScaleFactor, uint8 crtc[],
					 uint8& cr3b, uint8& cr3c, uint8& cr5d, uint8& cr5e)
{
	// Initialize the timing values for CRTC registers cr00 to cr18 and cr3a,
	// cr3b, cr5d, and cr5e.  Note that the number following the letters 'cr'
	// is a hexadecimal number.  Argument crtc will contain registers cr00 to
	// cr18;  thus, it must contain at least 25 (0x19) elements.

	// Normally the horizontal timing values are divided by 8;  however, some
	// chips require the horizontal timings to be doubled when the color depth
	// is 16 bpp.  The horizontal scale factor is used for this purpose.

	int hTotal = (mode.timing.h_total * horzScaleFactor) / 8 - 5;
	int hDisp_e = (mode.timing.h_display * horzScaleFactor) / 8 - 1;
	int hSync_s = (mode.timing.h_sync_start * horzScaleFactor) / 8;
	int hSync_e = (mode.timing.h_sync_end * horzScaleFactor) / 8;
	int hBlank_s = hDisp_e + 1;		// start of horizontal blanking
	int hBlank_e = hTotal;			// end of horizontal blanking

	int vTotal = mode.timing.v_total - 2;
	int vDisp_e = mode.timing.v_display - 1;
	int vSync_s = mode.timing.v_sync_start;
	int vSync_e = mode.timing.v_sync_end;
	int vBlank_s = vDisp_e;			// start of vertical blanking
	int vBlank_e = vTotal;			// end of vertical blanking

	// CRTC Controller values

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
	crtc[0x13] = mode.bytesPerRow / 8;
	crtc[0x14] = 0x00;
	crtc[0x15] = vBlank_s;
	crtc[0x16] = vBlank_e;
	crtc[0x17] = 0xc3;
	crtc[0x18] = 0xff;

	int i = ((hTotal & 0x100) >> 8) |
			((hDisp_e & 0x100) >> 7) |
			((hBlank_s & 0x100) >> 6) |
			((hSync_s & 0x100) >> 4);

	if (hSync_e - hSync_s > 64)
		i |= 0x08;		// add another 64 DCLKs to blank pulse width

	if (hSync_e - hSync_s > 32)
		i |= 0x20;		// add another 32 DCLKs to hsync pulse width

	int j = (crtc[0] + ((i & 0x01) << 8) + crtc[4] + ((i & 0x10) << 4) + 1) / 2;

	if (j - (crtc[4] + ((i & 0x10) << 4)) < 4) {
		if (crtc[4] + ((i & 0x10) << 4) + 4 <= crtc[0] + ((i & 0x01) << 8))
			j = crtc[4] + ((i & 0x10) << 4) + 4;
		else
			j = crtc[0] + ((i & 0x01) << 8) + 1;
	}

	cr3b = j & 0xff;
	i |= (j & 0x100) >> 2;
	cr3c = (crtc[0] + ((i & 0x01) << 8)) / 2;
	cr5d = i;

	cr5e =	((vTotal & 0x400) >> 10) |
			((vDisp_e & 0x400) >> 9) |
			((vBlank_s & 0x400) >> 8) |
			((vSync_s & 0x400) >> 6) | 0x40;
}


static display_mode*
FindDisplayMode(int width, int height, int refreshRate, uint32 colorDepth)
{
	// Search the mode list for the mode with specified width, height,
	// refresh rate, and color depth.  If argument colorDepth is zero, this
	// function will search for a mode satisfying the other 3 arguments, and
	// if more than one matching mode is found, the one with the greatest color
	// depth will be selected.
	//
	// If successful, return a pointer to the selected display_mode object;
	// else return NULL.

	display_mode* selectedMode = NULL;
	uint32 modeCount = gInfo.sharedInfo->modeCount;

	for (uint32 j = 0; j < modeCount; j++) {
		display_mode& mode = gInfo.modeList[j];

		if (mode.timing.h_display == width && mode.timing.v_display == height) {
			int modeRefreshRate = int(((mode.timing.pixel_clock * 1000.0 /
					mode.timing.h_total) / mode.timing.v_total) + 0.5);
			if (modeRefreshRate == refreshRate) {
				if (colorDepth == 0) {
					if (selectedMode == NULL || mode.space > selectedMode->space)
						selectedMode = &mode;
				} else {
					if (mode.space == colorDepth)
						return &mode;
				}
			}
		}
	}

	return selectedMode;
}



static bool
IsThereEnoughFBMemory(const display_mode* mode, uint32 bitsPerPixel)
{
	// Test if there is enough Frame Buffer memory for the mode and color depth
	// specified by the caller, and return true if there is sufficient memory.

	uint32 maxWidth = mode->virtual_width;
	if (mode->timing.h_display > maxWidth)
		maxWidth = mode->timing.h_display;

	uint32 maxHeight = mode->virtual_height;
	if (mode->timing.v_display > maxHeight)
		maxHeight = mode->timing.v_display;

	uint32 bytesPerPixel = (bitsPerPixel + 7) / 8;

	return (maxWidth * maxHeight * bytesPerPixel < gInfo.sharedInfo->maxFrameBufferSize);
}



bool
IsModeUsable(const display_mode* mode)
{
	// Test if the display mode is usable by the current video chip.  That is,
	// does the chip have enough memory for the mode and is the pixel clock
	// within the chips allowable range, etc.
	//
	// Return true if the mode is usable.

	SharedInfo& si = *gInfo.sharedInfo;
	uint32 bitsPerPixel;
	uint32 maxPixelClock;

	if ( ! gInfo.GetColorSpaceParams(mode->space, bitsPerPixel, maxPixelClock))
		return false;

	// Is there enough frame buffer memory to handle the mode?

	if ( ! IsThereEnoughFBMemory(mode, bitsPerPixel))
		return false;

	if (mode->timing.pixel_clock > maxPixelClock)
		return false;

	// Is the color space supported?

	bool bColorSpaceSupported = false;
	for (uint32 j = 0; j < si.colorSpaceCount; j++) {
		if (mode->space == uint32(si.colorSpaces[j])) {
			bColorSpaceSupported = true;
			break;
		}
	}

	if ( ! bColorSpaceSupported)
		return false;

	// Reject modes with a width of 640 and a height < 480 since they do not
	// work properly with the S3 chipsets.

	if (mode->timing.h_display == 640 && mode->timing.v_display < 480)
		return false;

	// If the video chip is connected directly to an LCD display (ie, laptop
	// computer), restrict the display mode to resolutions where the width and
	// height of the mode are less than or equal to the width and height of the
	// LCD display.

	if (si.displayType == MT_LCD && si.panelX > 0 && si.panelY > 0
			&& (mode->timing.h_display > si.panelX
			|| mode->timing.v_display > si.panelY)) {
		return false;
	}

	return true;
}


status_t
CreateModeList( bool (*checkMode)(const display_mode* mode),
				bool (*getEdid)(edid1_info& edidInfo))
{
	SharedInfo& si = *gInfo.sharedInfo;

	// Obtain EDID info which is needed for for building the mode list.
	// However, if a laptop's LCD display is active, bypass getting the EDID
	// info since it is not needed, and if the external display supports only
	// resolutions smaller than the size of the laptop LCD display, it would
	// unnecessarily restrict size of the resolutions that could be set for
	// laptop LCD display.

	si.bHaveEDID = false;

	if (si.displayType != MT_LCD) {
		if (getEdid != NULL)
			si.bHaveEDID = getEdid(si.edidInfo);

		if ( ! si.bHaveEDID) {
			S3GetEDID ged;
			ged.magic = S3_PRIVATE_DATA_MAGIC;

			if (ioctl(gInfo.deviceFileDesc, S3_GET_EDID, &ged, sizeof(ged)) == B_OK) {
				if (ged.rawEdid.version.version != 1
						|| ged.rawEdid.version.revision > 4) {
					TRACE("CreateModeList(); EDID version %d.%d out of range\n",
						ged.rawEdid.version.version, ged.rawEdid.version.revision);
				} else {
					edid_decode(&si.edidInfo, &ged.rawEdid);	// decode & save EDID info
					si.bHaveEDID = true;
				}
			}
		}

		if (si.bHaveEDID) {
#ifdef ENABLE_DEBUG_TRACE
			edid_dump(&(si.edidInfo));
#endif
		} else {
			TRACE("CreateModeList(); Unable to get EDID info\n");
		}
	}

	display_mode* list;
	uint32 count = 0;
	area_id listArea;

	listArea = create_display_modes("S3 modes",
		si.bHaveEDID ? &si.edidInfo : NULL,
		NULL, 0, si.colorSpaces, si.colorSpaceCount, 
		(check_display_mode_hook)checkMode, &list, &count);

	if (listArea < 0)
		return listArea;		// listArea has error code

	si.modeArea = gInfo.modeListArea = listArea;
	si.modeCount = count;
	gInfo.modeList = list;

	return B_OK;
}



status_t
ProposeDisplayMode(display_mode *target, const display_mode *low,
	const display_mode *high)
{
	(void)low;		// avoid compiler warning for unused arg
	(void)high;		// avoid compiler warning for unused arg

	TRACE("ProposeDisplayMode()  %dx%d, pixel clock: %d kHz, space: 0x%X\n",
		target->timing.h_display, target->timing.v_display,
		target->timing.pixel_clock, target->space);

	// Search the mode list for the specified mode.

	uint32 modeCount = gInfo.sharedInfo->modeCount;

	for (uint32 j = 0; j < modeCount; j++) {
		display_mode& mode = gInfo.modeList[j];

		if (target->timing.h_display == mode.timing.h_display
			&& target->timing.v_display == mode.timing.v_display
			&& target->space == mode.space)
			return B_OK;	// mode found in list
	}

	return B_BAD_VALUE;		// mode not found in list
}



status_t 
SetDisplayMode(display_mode* pMode)
{
	// First validate the mode, then call a function to set the registers.

	TRACE("SetDisplayMode() begin\n");

	SharedInfo& si = *gInfo.sharedInfo;
	DisplayModeEx mode;
	(display_mode&)mode = *pMode;

	uint32 maxPixelClock;
	if ( ! gInfo.GetColorSpaceParams(mode.space, mode.bpp, maxPixelClock))
		return B_BAD_VALUE;

	if (ProposeDisplayMode(&mode, pMode, pMode) != B_OK)
		return B_BAD_VALUE;

	// Note that for some Savage chips, accelerated drawing is badly messed up
	// when the display width is 1400 because 1400 is not evenly divisible by 32.
	// For those chips, set the width to 1408 so that accelerated drawing will
	// draw properly.

	if (mode.timing.h_display == 1400 && (si.chipType == S3_PROSAVAGE
			|| si.chipType == S3_PROSAVAGE_DDR
			|| si.chipType == S3_TWISTER
			|| si.chipType == S3_SUPERSAVAGE
			|| si.chipType == S3_SAVAGE2000)) {
		mode.timing.h_display = 1408;
		mode.virtual_width = 1408;
	}

	int bytesPerPixel = (mode.bpp + 7) / 8;
	mode.bytesPerRow = mode.timing.h_display * bytesPerPixel;

	// Is there enough frame buffer memory for this mode?

	if ( ! IsThereEnoughFBMemory(&mode, mode.bpp))
		return B_NO_MEMORY;

	TRACE("Set display mode: %dx%d  virtual size: %dx%d  color depth: %d bpp\n",
		mode.timing.h_display, mode.timing.v_display,
		mode.virtual_width, mode.virtual_height, mode.bpp);

	TRACE("   mode timing: %d  %d %d %d %d  %d %d %d %d\n",
		mode.timing.pixel_clock,
		mode.timing.h_display,
		mode.timing.h_sync_start, mode.timing.h_sync_end,
		mode.timing.h_total,
		mode.timing.v_display,
		mode.timing.v_sync_start, mode.timing.v_sync_end,
		mode.timing.v_total);

	TRACE("   mode hFreq: %.1f kHz  vFreq: %.1f Hz  %chSync %cvSync\n",
		double(mode.timing.pixel_clock) / mode.timing.h_total,
		((double(mode.timing.pixel_clock) / mode.timing.h_total) * 1000.0)
		/ mode.timing.v_total,
		(mode.timing.flags & B_POSITIVE_HSYNC) ? '+' : '-',
		(mode.timing.flags & B_POSITIVE_VSYNC) ? '+' : '-');

	if ( ! gInfo.SetDisplayMode(mode))
		return B_ERROR;

	si.displayMode = mode;

	TRACE("SetDisplayMode() done\n");
	return B_OK;
}



status_t 
MoveDisplay(uint16 horizontalStart, uint16 verticalStart)
{
	// Set which pixel of the virtual frame buffer will show up in the
	// top left corner of the display device.	Used for page-flipping
	// games and virtual desktops.

	DisplayModeEx& mode = gInfo.sharedInfo->displayMode;

	if (mode.timing.h_display + horizontalStart > mode.virtual_width
		|| mode.timing.v_display + verticalStart > mode.virtual_height)
		return B_ERROR;

	mode.h_display_start = horizontalStart;
	mode.v_display_start = verticalStart;

	gInfo.AdjustFrame(mode);
	return B_OK;
}


uint32 
AccelerantModeCount(void)
{
	// Return the number of display modes in the mode list.

	return gInfo.sharedInfo->modeCount;
}


status_t 
GetModeList(display_mode* dmList)
{
	// Copy the list of supported video modes to the location pointed at
	// by dmList.

	memcpy(dmList, gInfo.modeList, gInfo.sharedInfo->modeCount * sizeof(display_mode));
	return B_OK;
}


status_t 
GetDisplayMode(display_mode* current_mode)
{
	*current_mode = gInfo.sharedInfo->displayMode;	// return current display mode
	return B_OK;
}


status_t 
GetFrameBufferConfig(frame_buffer_config* pFBC)
{
	SharedInfo& si = *gInfo.sharedInfo;

	pFBC->frame_buffer = (void*)((addr_t)si.videoMemAddr + si.frameBufferOffset);
	pFBC->frame_buffer_dma = (void*)((addr_t)si.videoMemPCI + si.frameBufferOffset);
	uint32 bytesPerPixel = (si.displayMode.bpp + 7) / 8;
	pFBC->bytes_per_row = si.displayMode.virtual_width * bytesPerPixel;

	return B_OK;
}


status_t 
GetPixelClockLimits(display_mode* mode, uint32* low, uint32* high)
{
	// Return the maximum and minium pixel clock limits for the specified mode.

	uint32 bitsPerPixel;
	uint32 maxPixelClock;

	if ( ! gInfo.GetColorSpaceParams(mode->space, bitsPerPixel, maxPixelClock))
		return B_ERROR;

	if (low != NULL) {
		// lower limit of about 48Hz vertical refresh
		uint32 totalClocks = (uint32)mode->timing.h_total * (uint32)mode->timing.v_total;
		uint32 lowClock = (totalClocks * 48L) / 1000L;
		if (lowClock > maxPixelClock)
			return B_ERROR;

		*low = lowClock;
	}

	if (high != NULL)
		*high = maxPixelClock;

	return B_OK;
}


status_t
GetTimingConstraints(display_timing_constraints *constraints)
{
	(void)constraints;		// avoid compiler warning for unused arg

	return B_ERROR;
}


status_t
GetPreferredDisplayMode(display_mode* preferredMode)
{
	// If the chip is connected to a laptop LCD panel, find the mode with
	// matching width and height, 60 Hz refresh rate, and greatest color depth.

	SharedInfo& si = *gInfo.sharedInfo;

	if (si.displayType == MT_LCD) {
		display_mode* mode = FindDisplayMode(si.panelX, si.panelY, 60, 0);

		if (mode != NULL) {
			*preferredMode = *mode;
			return B_OK;
		}
	}

	return B_ERROR;
}



#ifdef __HAIKU__

status_t
GetEdidInfo(void* info, size_t size, uint32* _version)
{
	SharedInfo& si = *gInfo.sharedInfo;

	if ( ! si.bHaveEDID)
		return B_ERROR;

	if (size < sizeof(struct edid1_info))
		return B_BUFFER_OVERFLOW;

	memcpy(info, &si.edidInfo, sizeof(struct edid1_info));
	*_version = EDID_VERSION_1;
	return B_OK;
}

#endif	// __HAIKU__
