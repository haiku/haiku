//------------------------------------------------------------------------------
//	Copyright (c) 2002-2004, Haiku
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
//	File Name:		PrivateScreen.cpp
//	Author:			Stefano Ceccherini (burton666@libero.it)
//	Description:	BPrivateScreen is the class which does the real work
//					for the proxy class BScreen (it interacts with the app server).
//------------------------------------------------------------------------------

#include <Window.h>


#include "PrivateScreen.h"


// TODO: We should define this somewhere else
// We could find how R5 defines this, though it's not strictly necessary
struct screen_desc {
	void *base_address;
	uint32 bytes_per_row;
};


// Defined in Application.cpp
extern BPrivateScreen *gPrivateScreen;


BPrivateScreen *
BPrivateScreen::CheckOut(BWindow *win)
{
	return gPrivateScreen;
}


BPrivateScreen *
BPrivateScreen::CheckOut(screen_id id)
{
	return gPrivateScreen;
}


void
BPrivateScreen::Return(BPrivateScreen *screen)
{
}

	
status_t
BPrivateScreen::SetToNext()
{
	// This function always returns B_ERROR
	return B_ERROR;
}

	
color_space
BPrivateScreen::ColorSpace()
{
	display_mode mode;
	if (GetMode(B_ALL_WORKSPACES, &mode) == B_OK)
		return (color_space)mode.space;
		
	return B_NO_COLOR_SPACE;
}


BRect
BPrivateScreen::Frame()
{
	display_mode mode;
	if (GetMode(B_ALL_WORKSPACES, &mode) == B_OK) {
		BRect frame(0, 0, (float)mode.virtual_width - 1, (float)mode.virtual_height - 1);
		return frame;
	}
	return BRect();
}


screen_id 
BPrivateScreen::ID()
{
	return B_MAIN_SCREEN_ID;
}
	
	
status_t
BPrivateScreen::WaitForRetrace(bigtime_t timeout)
{
	// Get the retrace semaphore if it's the first time 
	// we are called. Cache the value then.
	status_t status;
	if (fRetraceSem == -1)
		fRetraceSem = RetraceSemaphore();

	do {
		status = acquire_sem_etc(fRetraceSem, 1, B_RELATIVE_TIMEOUT, timeout);
	} while (status == B_INTERRUPTED);

	return status;
}

	
sem_id
BPrivateScreen::RetraceSemaphore()
{
	sem_id id = B_BAD_SEM_ID;
	// TODO: Implement
	return id;
}



uint8
BPrivateScreen::IndexForColor(uint8 red, uint8 green, uint8 blue, uint8 alpha)
{
	// Looks like this check is necessary
	if (red == B_TRANSPARENT_COLOR.red && green == B_TRANSPARENT_COLOR.green &&
		blue == B_TRANSPARENT_COLOR.blue && alpha == B_TRANSPARENT_COLOR.alpha)
		return B_TRANSPARENT_8_BIT;

	
	if (fColorMap)
		return fColorMap->index_map[((red & 0xf8) << 7)
								| ((green & 0xf8) << 2)
								| (blue >> 3)];

	return 0;
}


rgb_color
BPrivateScreen::ColorForIndex(const uint8 index)
{
	if (fColorMap)
		return fColorMap->color_list[index];

	return rgb_color();
}


uint8
BPrivateScreen::InvertIndex(uint8 index)
{
	if (fColorMap)
		return fColorMap->inversion_map[index];

	return 0;
}
	

const color_map *
BPrivateScreen::ColorMap()
{
	return fColorMap;
}


status_t
BPrivateScreen::GetBitmap(BBitmap **bitmap, bool drawCursor, BRect *bound)
{
	// TODO: Implement
	return B_ERROR;
}


status_t
BPrivateScreen::ReadBitmap(BBitmap *bitmap, bool drawCursor, BRect *bound)
{
	// TODO: Implement
	return B_ERROR;
}

		
rgb_color
BPrivateScreen::DesktopColor(uint32 index)
{
	if (fColorMap) {
	
	}

	return rgb_color();
}


void
BPrivateScreen::SetDesktopColor(rgb_color, uint32, bool)
{
	if (fColorMap) {
	
	}
}


status_t
BPrivateScreen::ProposeMode(display_mode *target, const display_mode *low, const display_mode *high)
{
	// TODO: Implement
	return B_ERROR;
}
		

status_t
BPrivateScreen::GetModeList(display_mode **mode_list, uint32 *count)
{
	// TODO: Implement
	return B_ERROR;
}


status_t
BPrivateScreen::GetMode(uint32 workspace, display_mode *mode)
{
	// TODO: Implement
	return B_ERROR;
}


status_t
BPrivateScreen::SetMode(uint32 workspace, display_mode *mode, bool makeDefault)
{
	// TODO: Implement
	return B_ERROR;
}


status_t
BPrivateScreen::GetDeviceInfo(accelerant_device_info *info)
{
	// TODO: Implement
	return B_ERROR;
}


status_t
BPrivateScreen::GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high)
{
	// TODO: Implement
	return B_ERROR;
}


status_t
BPrivateScreen::GetTimingConstraints(display_timing_constraints *dtc)
{
	// TODO: Implement
	return B_ERROR;
}


status_t
BPrivateScreen::SetDPMS(uint32 dpmsState)
{
	// TODO: Implement
	return B_ERROR;
}


uint32
BPrivateScreen::DPMSState()
{
	// TODO: Implement
	return B_ERROR;
}


uint32
BPrivateScreen::DPMSCapabilites()
{
	// TODO: Implement
	return B_ERROR;
}


void *
BPrivateScreen::BaseAddress()
{
	screen_desc desc;
	if (get_screen_desc(&desc) == B_OK)
		return desc.base_address;
	
	return NULL;
}


uint32
BPrivateScreen::BytesPerRow()
{
	screen_desc desc;
	if (get_screen_desc(&desc) == B_OK)
		return desc.bytes_per_row;
	return 0;
}

	
status_t
BPrivateScreen::get_screen_desc(screen_desc *desc)
{
	return B_ERROR;
}

	
// Private, called by BApplication::get_scs()
BPrivateScreen::BPrivateScreen()
	:
	fColorMap(NULL),
	fRetraceSem(-1),
	fOwnsColorMap(false)
{
	// TODO: Get a pointer or a copy of the colormap from the app server
}


BPrivateScreen::~BPrivateScreen()
{
	if (fOwnsColorMap)
		free(fColorMap);
}
