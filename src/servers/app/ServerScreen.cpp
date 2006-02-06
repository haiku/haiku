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
	if (fDriver) {
		// this will also init the graphics hardware the driver is attached to
		return fDriver->Initialize();
	}

	return B_NO_INIT;
}


void
Screen::Shutdown()
{
	if (fDriver)
		fDriver->Shutdown();
}


status_t
Screen::SetMode(display_mode mode, bool makeDefault)
{
	status_t status = fHWInterface->SetMode(mode);

	// the DrawingEngine needs to adjust itself
	if (status >= B_OK) {
		fDriver->Update();
		fIsDefault = makeDefault;
	}

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

	if (status >= B_OK)
		status = SetMode(mode, makeDefault);

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
	frequency = mode.timing.pixel_clock * 1000.f
		/ (mode.timing.h_total * mode.timing.v_total);
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
	display_mode* dm = NULL;
	uint32 count;

	status_t status = fHWInterface->GetModeList(&dm, &count);
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

#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
	int32 index = _FindMode(dm, count, width, height, colorspace, frequency);
	if (index < 0) {
		fprintf(stderr, "mode not found (%d, %d, %f) -> ignoring frequency and using B_CMAP8!\n", width, height, frequency);
		index = _FindMode(dm, count, width, height, B_CMAP8, frequency, true);
	}
#else
	// Ignore frequency in test mode
	int32 index = _FindMode(dm, count, width, height, colorspace, frequency, true);
#endif

	if (index < 0) {
		fprintf(stderr, "mode not found (%d, %d, %f) -> using first mode from list!\n", width, height, frequency);
		// fallback to first mode from list - TODO: ?!?
		// NOTE: count > 0 is checked above
		*mode = dm[0];	
	} else {
		*mode = dm[index];	
	}

	delete[] dm;

	return B_OK;
}


int32
Screen::_FindMode(const display_mode* dm, uint32 count,
				  uint16 width, uint16 height, uint32 colorspace,
				  float frequency, bool ignoreFrequency) const
{
	int32 index = -1;
	for (uint32 i = 0; i < count; i++) {
		if (dm[i].virtual_width == width
			&& dm[i].virtual_height == height
			&& dm[i].space == colorspace) {
			if (!ignoreFrequency) {
				// we have found a mode with the correct width, height and format
				// now see if the frequency matches (don't be too picky)
				float modeFrequency = 0.0;
				float t = dm[i].timing.h_total * dm[i].timing.v_total;
				if (t != 0.0)
					modeFrequency = dm[i].timing.pixel_clock * 1000.f / t;
	
				if (int(modeFrequency/* * 10.0*/ + 0.5) == int(frequency/* * 10.0*/ + 0.5)) {
					index = i;
					break;
				}
			} else {
				// ignore frequency
				index = i;
				break;
			}
		}
	}
	return index;
}

