/*
 * Copyright 2001-2013, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Adi Oanca <adioanca@myrealbox.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus, <superstippi@gmx.de>
 */


#include "Screen.h"

#include "BitmapManager.h"
#include "DrawingEngine.h"
#include "HWInterface.h"

#include <Accelerant.h>
#include <Point.h>
#include <GraphicsDefs.h>

#include <stdlib.h>
#include <stdio.h>


static float
get_mode_frequency(const display_mode& mode)
{
	// Taken from Screen preferences
	float timing = float(mode.timing.h_total * mode.timing.v_total);
	if (timing == 0.0f)
		return 0.0f;

	return rint(10 * float(mode.timing.pixel_clock * 1000)
		/ timing) / 10.0;
}


//	#pragma mark -


Screen::Screen(::HWInterface *interface, int32 id)
	:
	fID(id),
	fDriver(interface ? interface->CreateDrawingEngine() : NULL),
	fHWInterface(interface)
{
}


Screen::Screen()
	:
	fID(-1)
{
}


Screen::~Screen()
{
	Shutdown();
}


/*! Finds the mode in the mode list that is closest to the mode specified.
	As long as the mode list is not empty, this method will always succeed.
*/
status_t
Screen::Initialize()
{
	if (fHWInterface.Get() != NULL) {
		// init the graphics hardware
		return fHWInterface->Initialize();
	}

	return B_NO_INIT;
}


void
Screen::Shutdown()
{
	if (fHWInterface.Get() != NULL)
		fHWInterface->Shutdown();
}


status_t
Screen::SetMode(const display_mode& mode)
{
	display_mode current;
	GetMode(current);
	if (!memcmp(&mode, &current, sizeof(display_mode)))
		return B_OK;

	gBitmapManager->SuspendOverlays();

	status_t status = fHWInterface->SetMode(mode);
		// any attached DrawingEngines will be notified

	gBitmapManager->ResumeOverlays();

	return status;
}


status_t
Screen::SetMode(uint16 width, uint16 height, uint32 colorSpace,
	const display_timing& timing)
{
	display_mode mode;
	mode.timing = timing;
	mode.space = colorSpace;
	mode.virtual_width = width;
	mode.virtual_height = height;
	mode.h_display_start = 0;
	mode.v_display_start = 0;
	mode.flags = 0;

	return SetMode(mode);
}


status_t
Screen::SetBestMode(uint16 width, uint16 height, uint32 colorSpace,
	float frequency, bool strict)
{
	// search for a matching mode
	display_mode* modes = NULL;
	uint32 count;
	status_t status = fHWInterface->GetModeList(&modes, &count);
	if (status < B_OK)
		return status;
	if (count <= 0)
		return B_ERROR;

	int32 index = _FindBestMode(modes, count, width, height, colorSpace,
		frequency);
	if (index < 0) {
		debug_printf("app_server: Finding best mode for %ux%u (%" B_PRIu32
			", %g Hz%s) failed\n", width, height, colorSpace, frequency,
			strict ? ", strict" : "");

		if (strict) {
			delete[] modes;
			return B_ERROR;
		} else {
			index = 0;
			// Just use the first mode in the list
			debug_printf("app_server: Use %ux%u (%" B_PRIu32 ") instead.\n",
				modes[0].timing.h_total, modes[0].timing.v_total, modes[0].space);
		}
	}

	display_mode mode = modes[index];
	delete[] modes;

	float modeFrequency = get_mode_frequency(mode);
	display_mode originalMode = mode;
	bool adjusted = false;

	if (modeFrequency != frequency) {
		// adjust timing to fit the requested frequency if needed
		// (taken from Screen preferences application)
		mode.timing.pixel_clock = ((uint32)mode.timing.h_total
			* mode.timing.v_total / 10 * int32(frequency * 10)) / 1000;
		adjusted = true;
	}
	status = SetMode(mode);
	if (status != B_OK && adjusted) {
		// try again with the unchanged mode
		status = SetMode(originalMode);
	}

	return status;
}


status_t
Screen::SetPreferredMode()
{
	display_mode mode;
	status_t status = fHWInterface->GetPreferredMode(&mode);
	if (status != B_OK)
		return status;

	return SetMode(mode);
}


void
Screen::GetMode(display_mode& mode) const
{
	fHWInterface->GetMode(&mode);
}


void
Screen::GetMode(uint16 &width, uint16 &height, uint32 &colorspace,
	float &frequency) const
{
	display_mode mode;
	fHWInterface->GetMode(&mode);

	width = mode.virtual_width;
	height = mode.virtual_height;
	colorspace = mode.space;
	frequency = get_mode_frequency(mode);
}


status_t
Screen::GetMonitorInfo(monitor_info& info) const
{
	return fHWInterface->GetMonitorInfo(&info);
}


void
Screen::SetFrame(const BRect& rect)
{
	// TODO: multi-monitor support...
}


BRect
Screen::Frame() const
{
	display_mode mode;
	fHWInterface->GetMode(&mode);

	return BRect(0, 0, mode.virtual_width - 1, mode.virtual_height - 1);
}


color_space
Screen::ColorSpace() const
{
	display_mode mode;
	fHWInterface->GetMode(&mode);

	return (color_space)mode.space;
}


/*!	\brief Returns the mode that matches the given criteria best.
	The "width" argument is the only hard argument, the rest will be adapted
	as needed.
*/
int32
Screen::_FindBestMode(const display_mode* modes, uint32 count,
	uint16 width, uint16 height, uint32 colorSpace, float frequency) const
{
	int32 bestDiff = 0;
	int32 bestIndex = -1;
	for (uint32 i = 0; i < count; i++) {
		const display_mode& mode = modes[i];
		if (mode.virtual_width != width)
			continue;

		// compute some random equality score
		// TODO: check if these scores make sense
		int32 diff = 1000 * abs(mode.timing.v_display - height)
			+ int32(fabs(get_mode_frequency(mode) - frequency) * 10)
			+ 100 * abs((int)(mode.space - colorSpace));

		if (bestIndex == -1 || diff < bestDiff) {
			bestDiff = diff;
			bestIndex = i;
		}
	}

	return bestIndex;
}
