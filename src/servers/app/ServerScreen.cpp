//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ServerScreen.cpp
//	Author:			Adi Oanca <adioanca@myrealbox.com>
//					Axel Dörfler, axeld@pinc-software.de
//	Description:	Handles individual screens
//
//------------------------------------------------------------------------------


#include <Accelerant.h>
#include <Point.h>
#include <GraphicsDefs.h>

#include <stdlib.h>
#include <stdio.h>

#include "ServerScreen.h"
#include "DisplayDriver.h"


Screen::Screen(DisplayDriver *driver, int32 id)
	:
	fID(id),
	fDriver(driver)
{
}


Screen::~Screen(void)
{
//printf("~Screen( %ld )\n", fID);
	// TODO: Who is supposed to delete the display driver? It's ours, no?
//	delete fDDriver;
}


bool
Screen::SupportsMode(uint16 width, uint16 height, uint32 colorspace, float frequency)
{
	// TODO: remove/improve
	return true;
	display_mode *dm = NULL;
	uint32 count;

	status_t err = fDriver->GetModeList(&dm, &count);
	if (err < B_OK) {
		// We've run into quite a problem here! This is a function which is a requirement
		// for a graphics module. The best thing that we can hope for is 640x480x8 without
		// knowing anything else. While even this seems like insanity to assume that we
		// can support this, the only lower mode supported is 640x400, but we shouldn't even
		// bother with such a pathetic possibility.
		if (width == 640 && height == 480 && colorspace == B_CMAP8)
			return true;

		return false;
	}

	for (uint32 i = 0; i < count; i++) {
		if (dm[i].virtual_width == width
			&& dm[i].virtual_height == height
			&& dm[i].space == colorspace)
			return true;
	}

	free(dm);
	return false;
}


bool
Screen::SetMode(uint16 width, uint16 height, uint32 colorspace, float frequency)
{
	if (!SupportsMode(width, height, colorspace, frequency))
		return false;

	display_mode mode;
	mode.virtual_width = width;
	mode.virtual_height = height;
	mode.space = colorspace;
	// ToDo: frequency!
	//	(we need to search the mode list for this)

	return fDriver->SetMode(mode) >= B_OK;
}


void
Screen::GetMode(uint16 &width, uint16 &height, uint32 &colorspace, float &frequency) const
{
	display_mode mode;
	fDriver->GetMode(mode);

	width = mode.virtual_width;
	height = mode.virtual_height;
	colorspace = mode.space;
	frequency = mode.timing.pixel_clock * 1000.f
		/ (mode.timing.h_total * mode.timing.v_total);
}


int32
Screen::ScreenNumber(void) const
{
	return fID;
}

