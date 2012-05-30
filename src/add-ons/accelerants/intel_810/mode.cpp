/*
 * Copyright 2007-2012 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */


#include "accelerant.h"

#include <create_display_modes.h>		// common accelerant header file
#include <string.h>
#include <unistd.h>


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

	return (maxWidth * maxHeight * bytesPerPixel
		<= gInfo.sharedInfo->maxFrameBufferSize);
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
	uint8 bitsPerPixel;
	uint32 maxPixelClock;

	if (!I810_GetColorSpaceParams(mode->space, bitsPerPixel, maxPixelClock))
		return false;

	// Is there enough frame buffer memory to handle the mode?

	if (!IsThereEnoughFBMemory(mode, bitsPerPixel))
		return false;

	if (mode->timing.pixel_clock > maxPixelClock)
		return false;

	// Is the color space supported?

	bool colorSpaceSupported = false;
	for (uint32 j = 0; j < si.colorSpaceCount; j++) {
		if (mode->space == uint32(si.colorSpaces[j])) {
			colorSpaceSupported = true;
			break;
		}
	}

	if (!colorSpaceSupported)
		return false;

	// Reject modes with a width of 640 and a height < 480 since they do not
	// work properly with the i810 chipsets.

	if (mode->timing.h_display == 640 && mode->timing.v_display < 480)
		return false;

	return true;
}


status_t
CreateModeList(bool (*checkMode)(const display_mode* mode))
{
	SharedInfo& si = *gInfo.sharedInfo;

	// Obtain EDID info which is needed for for building the mode list.

	si.bHaveEDID = false;

	if (!si.bHaveEDID) {
		edid1_raw rawEdid;	// raw EDID info to obtain

		if (ioctl(gInfo.deviceFileDesc, INTEL_GET_EDID, &rawEdid,
				sizeof(rawEdid)) == B_OK) {
			if (rawEdid.version.version != 1 || rawEdid.version.revision > 4) {
				TRACE("CreateModeList(); EDID version %d.%d out of range\n",
					rawEdid.version.version, rawEdid.version.revision);
			} else {
				edid_decode(&si.edidInfo, &rawEdid); // decode & save EDID info
				si.bHaveEDID = true;
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

	listArea = create_display_modes("i810 modes",
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
ProposeDisplayMode(display_mode* target, const display_mode* low,
	const display_mode* high)
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
	if (!I810_GetColorSpaceParams(mode.space, mode.bitsPerPixel, maxPixelClock))
		return B_BAD_VALUE;

	if (ProposeDisplayMode(&mode, pMode, pMode) != B_OK)
		return B_BAD_VALUE;

	mode.bytesPerPixel = (mode.bitsPerPixel + 7) / 8;
	mode.bytesPerRow = mode.timing.h_display * mode.bytesPerPixel;

	// Is there enough frame buffer memory for this mode?

	if ( ! IsThereEnoughFBMemory(&mode, mode.bitsPerPixel))
		return B_NO_MEMORY;

	TRACE("Set display mode: %dx%d  virtual size: %dx%d  "
		"color depth: %d bits/pixel\n",
		mode.timing.h_display, mode.timing.v_display,
		mode.virtual_width, mode.virtual_height, mode.bitsPerPixel);

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

	status_t status = I810_SetDisplayMode(mode);
	if (status != B_OK) {
		TRACE("SetDisplayMode() failed;  status 0x%x\n", status);
		return status;
	}

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

	I810_AdjustFrame(mode);
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

	memcpy(dmList, gInfo.modeList,
		gInfo.sharedInfo->modeCount * sizeof(display_mode));
	return B_OK;
}


status_t
GetDisplayMode(display_mode* current_mode)
{
	*current_mode = gInfo.sharedInfo->displayMode; // current display mode
	return B_OK;
}


status_t
GetFrameBufferConfig(frame_buffer_config* pFBC)
{
	SharedInfo& si = *gInfo.sharedInfo;

	pFBC->frame_buffer = (void*)((addr_t)(si.videoMemAddr));
	pFBC->frame_buffer_dma = (void*)((addr_t)(si.videoMemPCI));
	pFBC->bytes_per_row = si.displayMode.virtual_width
		* si.displayMode.bytesPerPixel;

	return B_OK;
}


status_t
GetPixelClockLimits(display_mode* mode, uint32* low, uint32* high)
{
	// Return the maximum and minium pixel clock limits for the specified mode.

	uint8 bitsPerPixel;
	uint32 maxPixelClock;

	if (!I810_GetColorSpaceParams(mode->space, bitsPerPixel, maxPixelClock))
		return B_ERROR;

	if (low != NULL) {
		// lower limit of about 48Hz vertical refresh
		uint32 totalClocks = (uint32)mode->timing.h_total
			* (uint32)mode->timing.v_total;
		uint32 lowClock = (totalClocks * 48L) / 1000L;
		if (lowClock > maxPixelClock)
			return B_ERROR;

		*low = lowClock;
	}

	if (high != NULL)
		*high = maxPixelClock;

	return B_OK;
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
