/*
 * Copyright (c) 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Adi Oanca <adioanca@myrealbox.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus, <superstippi@gmx.de>
 */


#include <Accelerant.h>
#include <Point.h>
#include <GraphicsDefs.h>

#include <stdlib.h>
#include <stdio.h>

#include "DrawingEngine.h"
#include "HWInterface.h"

#include "ServerScreen.h"


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
	: fID(id),
	  fDriver(interface ? new DrawingEngine(interface) : NULL),
	  fHWInterface(interface),
	  fIsDefault(true)
{
}


Screen::Screen()
	: fID(-1),
	  fDriver(NULL),
	  fHWInterface(NULL),
	  fIsDefault(true)
{
}


Screen::~Screen()
{
	Shutdown();
	delete fDriver;
	delete fHWInterface;
}


status_t
Screen::Initialize()
{
	if (fHWInterface) {
		// init the graphics hardware
		return fHWInterface->Initialize();
	}

	return B_NO_INIT;
}


void
Screen::Shutdown()
{
	if (fHWInterface)
		fHWInterface->Shutdown();
}


status_t
Screen::SetMode(display_mode mode, bool makeDefault)
{
	status_t status = fHWInterface->SetMode(mode);
		// any attached DrawingEngines will be notified

	if (status >= B_OK)
		fIsDefault = makeDefault;

	return status;
}


status_t
Screen::SetMode(uint16 width, uint16 height, uint32 colorspace,
	float frequency, bool makeDefault)
{
	// search for a matching mode
	display_mode mode;
	status_t status = _FindMode(width, height, colorspace, frequency, &mode);
	if (status < B_OK) {
		// TODO: Move fallback elsewhere, this function should simply
		// fail if requested to set unsupported mode.
		// Ups. Not good. Ignore the requested mode and use fallback params.
		status = _FindMode(640, 480, B_CMAP8, 60.0, &mode);
	}

	if (status >= B_OK) {
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

		status = SetMode(mode, makeDefault);
		if (status < B_OK) {
			// try again with the unchanged mode
			status = SetMode(originalMode, makeDefault);
		}
	}

	return status;
}


void
Screen::GetMode(display_mode* mode) const
{
	fHWInterface->GetMode(mode);
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


status_t
Screen::_FindMode(uint16 width, uint16 height, uint32 colorspace,
				  float frequency, display_mode* mode) const
{
	display_mode* modes = NULL;
	uint32 count;

	status_t status = fHWInterface->GetModeList(&modes, &count);
	if (status < B_OK || count <= 0) {
		// We've run into quite a problem here! This is a function which is a requirement
		// for a graphics module. The best thing that we can hope for is 640x480x8 without
		// knowing anything else. While even this seems like insanity to assume that we
		// can support this, the only lower mode supported is 640x400, but we shouldn't even
		// bother with such a pathetic possibility.
		if (width == 640 && height == 480 && colorspace == B_CMAP8)
			return B_OK;

		return status;
	}

	int32 index = _FindMode(modes, count, width, height, colorspace, frequency);
	if (index < 0) {
		fprintf(stderr, "mode not found (%d, %d, %f) -> using first mode from list!\n",
			width, height, frequency);
		// fallback to first mode from list - TODO: ?!?
		// NOTE: count > 0 is checked above
		*mode = modes[0];	
	} else {
		*mode = modes[index];	
	}

	delete[] modes;

	return B_OK;
}


/*!
	\brief Returns the mode that matches the given criteria. The frequency
		doesn't have to match, though - the closest one found is used.
*/
int32
Screen::_FindMode(const display_mode* modes, uint32 count,
	uint16 width, uint16 height, uint32 colorspace,
	float frequency) const
{
	// we always try to choose the mode with the closest frequency
	float bestFrequencyDiff = 0.0f;
	int32 index = -1;

	for (uint32 i = 0; i < count; i++) {
		if (modes[i].virtual_width == width
			&& modes[i].virtual_height == height
			&& modes[i].space == colorspace) {
			// we have found a mode with the correct width, height and format
			// now see if the frequency matches
			float modeFrequency = get_mode_frequency(modes[i]);
			float frequencyDiff = fabs(modeFrequency - frequency);

			if (index == -1 || bestFrequencyDiff > frequencyDiff) {
				index = i;
				if (frequencyDiff == 0.0f)
					break;

				bestFrequencyDiff = frequencyDiff;
			}
		}
	}

	return index;
}

