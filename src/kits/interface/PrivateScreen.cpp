//------------------------------------------------------------------------------
//	Copyright (c) 2002-2005, Haiku
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
#include "AppServerLink.h"
#include "PrivateScreen.h"
#include "ServerProtocol.h"

#include <Bitmap.h>
#include <Locker.h>
#include <Window.h>

#include <new>

#include <stdlib.h>


// TODO: We should define this somewhere else
// We could find how R5 defines this, though it's not strictly necessary
// AFAIK, R5 here keeps also the screen width, height, colorspace, and some other things
// (it's 48 bytes big)
struct screen_desc {
	void *base_address;
	uint32 bytes_per_row;
};


static BPrivateScreen *sScreen;

// used to synchronize creation/deletion of the sScreen object
static BLocker sScreenLock("screen lock");


using namespace BPrivate;

BPrivateScreen *
BPrivateScreen::CheckOut(BWindow *win)
{
	sScreenLock.Lock();
	
	if (sScreen == NULL) {
		// TODO: If we start supporting multiple monitors, we
		// should return the right screen for the passed BWindow
		sScreen = new BPrivateScreen();
	}
	
	sScreenLock.Unlock();
	
	return sScreen;
}


BPrivateScreen *
BPrivateScreen::CheckOut(screen_id id)
{
	sScreenLock.Lock();
	
	if (sScreen == NULL) {
		// TODO: If we start supporting multiple monitors, we
		// should return the right object for the given screen_id
		sScreen = new BPrivateScreen();
	}
	
	sScreenLock.Unlock();
	
	return sScreen;
}


void
BPrivateScreen::Return(BPrivateScreen *screen)
{
	// Never delete the sScreen object.
	// system_colors() expects the colormap to be 
	// permanently stored within libbe.
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
	if (GetMode(B_CURRENT_WORKSPACE, &mode) == B_OK)
		return (color_space)mode.space;
		
	return B_NO_COLOR_SPACE;
}


BRect
BPrivateScreen::Frame()
{
	// If something goes wrong, we just return this rectangle.
	BRect rect(0, 0, 0, 0);
	display_mode mode;
	if (GetMode(B_CURRENT_WORKSPACE, &mode) == B_OK)
		rect.Set(0, 0, (float)mode.virtual_width - 1, (float)mode.virtual_height - 1);
		
	return rect;
}


screen_id 
BPrivateScreen::ID()
{
	// TODO: Change this if we start supporting multiple screens
	return B_MAIN_SCREEN_ID;
}
	
	
status_t
BPrivateScreen::WaitForRetrace(bigtime_t timeout)
{
	// Get the retrace semaphore if it's the first time 
	// we are called. Cache the value then.
	status_t status;
	if (fRetraceSem < 0)
		fRetraceSem = RetraceSemaphore();

	do {
		status = acquire_sem_etc(fRetraceSem, 1, B_RELATIVE_TIMEOUT, timeout);
	} while (status == B_INTERRUPTED);

	return status;
}

	
uint8
BPrivateScreen::IndexForColor(uint8 red, uint8 green, uint8 blue, uint8 alpha)
{
	// Looks like this check is necessary
	if (red == B_TRANSPARENT_COLOR.red && green == B_TRANSPARENT_COLOR.green &&
		blue == B_TRANSPARENT_COLOR.blue && alpha == B_TRANSPARENT_COLOR.alpha)
		return B_TRANSPARENT_8_BIT;

	uint16 index = ((blue & 0xf8) << 7) | ((green & 0xf8) << 2) | (red >> 3);
	if (fColorMap)
		return fColorMap->index_map[index];

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
BPrivateScreen::GetBitmap(BBitmap **_bitmap, bool drawCursor, BRect *bounds)
{
	if (_bitmap == NULL)
		return B_BAD_VALUE;

	BBitmap* bitmap = new (std::nothrow) BBitmap(Frame(), ColorSpace());
	if (bitmap == NULL)
		return B_NO_MEMORY;

	status_t status = bitmap->InitCheck();
	if (status == B_OK)
		status = ReadBitmap(bitmap, drawCursor, bounds);
	if (status != B_OK) {
		delete bitmap;
		return status;
	}

	*_bitmap = bitmap;
	return B_OK;
}


status_t
BPrivateScreen::ReadBitmap(BBitmap *bitmap, bool drawCursor, BRect *bound)
{
	if (bitmap == NULL)
		return B_BAD_VALUE;
	
	BRect rect;
	if (bound != NULL)
		rect = *bound;
	else
		rect = Frame();
		
	BPrivate::AppServerLink link;
	link.StartMessage(AS_READ_BITMAP);
	link.Attach<int32>(bitmap->get_server_token());
	link.Attach<bool>(drawCursor);
	link.Attach<BRect>(rect);
	
	int32 code;
	if (link.FlushWithReply(code) < B_OK || code != SERVER_TRUE)
		return B_ERROR;
	
	return B_OK;
}

		
rgb_color
BPrivateScreen::DesktopColor(uint32 workspace)
{
	rgb_color color = { 51, 102, 152, 255 };
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_DESKTOP_COLOR);
	link.Attach<uint32>(workspace);

	int32 code;
	if (link.FlushWithReply(code) == B_OK
		&& code == SERVER_TRUE)
		link.Read<rgb_color>(&color);

	return color;
}


void
BPrivateScreen::SetDesktopColor(rgb_color color, uint32 workspace, bool makeDefault)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_SET_DESKTOP_COLOR);
	link.Attach<rgb_color>(color);
	link.Attach<int32>(workspace);
	link.Attach<bool>(makeDefault);
	link.Flush();
}


status_t
BPrivateScreen::ProposeMode(display_mode *target,
				const display_mode *low, const display_mode *high)
{
	// We can't return B_BAD_VALUE here, because it's used to indicate
	// that the mode returned is supported, but it doesn't fall
	// within the limit (see ProposeMode() documentation) 
	if (target == NULL || low == NULL || high == NULL)
		return B_ERROR;
		
	BPrivate::AppServerLink link;
	link.StartMessage(AS_PROPOSE_MODE);
	link.Attach<screen_id>(ID());
	link.Attach<display_mode>(*target);
	link.Attach<display_mode>(*low);
	link.Attach<display_mode>(*high);
	
	int32 reply;
	status_t status = B_ERROR;
	if (link.FlushWithReply(reply) == B_OK && reply == SERVER_TRUE) {
		link.Read<display_mode>(target);	
		// The status is important here. See documentation 
		link.Read<status_t>(&status);	
	}
	
	return status;
}
		

status_t
BPrivateScreen::GetModeList(display_mode **modeList, uint32 *count)
{
	if (modeList == NULL || count == NULL)
		return B_BAD_VALUE;
		
	status_t status = B_ERROR;
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_MODE_LIST);
	link.Attach<screen_id>(ID());
	int32 reply;
	if (link.FlushWithReply(reply) == B_OK && reply == SERVER_TRUE) {
		link.Read<uint32>(count);
		int32 size = *count * sizeof(display_mode);
		*modeList = (display_mode *)malloc(size);
		link.Read(*modeList, size);
		// TODO: get status from app_server
		status = B_OK;	
	}
	
	return status;
}


status_t
BPrivateScreen::GetMode(uint32 workspace, display_mode *mode)
{
	if (mode == NULL)
		return B_BAD_VALUE;
		
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SCREEN_GET_MODE);
	link.Attach<screen_id>(ID());
	link.Attach<uint32>(workspace);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != SERVER_TRUE)
		return B_ERROR;

	display_mode currentMode;
	link.Read<display_mode>(&currentMode);

	status_t status = B_ERROR;
	link.Read<status_t>(&status);
	if (status == B_OK && mode)
		*mode = currentMode; 
	
	return status;
}


status_t
BPrivateScreen::SetMode(uint32 workspace, display_mode *mode, bool makeDefault)
{
	if (mode == NULL)
		return B_BAD_VALUE;
		
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SCREEN_SET_MODE);
	link.Attach<screen_id>(ID());
	link.Attach<uint32>(workspace);
	link.Attach<display_mode>(*mode);
	link.Attach<bool>(makeDefault);
	
	status_t status = B_ERROR;
	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == SERVER_TRUE)
		link.Read<status_t>(&status);

	return status;
}


status_t
BPrivateScreen::GetDeviceInfo(accelerant_device_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;
	
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_ACCELERANT_INFO);
	link.Attach<screen_id>(ID());
	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == SERVER_TRUE) {
		link.Read<accelerant_device_info>(info);
		return B_OK;
	}
	
	return B_ERROR;
}


status_t
BPrivateScreen::GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high)
{
	if (mode == NULL || low == NULL || high == NULL)
		return B_BAD_VALUE;
	
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_PIXEL_CLOCK_LIMITS);
	link.Attach<screen_id>(ID());
	link.Attach<display_mode>(*mode);
	
	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == SERVER_TRUE) {
		link.Read<uint32>(low);
		link.Read<uint32>(high);

		return B_OK;
	}
	
	return B_ERROR;
}


status_t
BPrivateScreen::GetTimingConstraints(display_timing_constraints *dtc)
{
	if (dtc == NULL)
		return B_BAD_VALUE;
		
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_TIMING_CONSTRAINTS);
	link.Attach<screen_id>(ID());
	
	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == SERVER_TRUE) {
		link.Read<display_timing_constraints>(dtc);	
		return B_OK;
	}
	
	return B_ERROR;
}


status_t
BPrivateScreen::SetDPMS(uint32 dpmsState)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_DPMS);
	link.Attach<screen_id>(ID());
	link.Attach<uint32>(dpmsState);
	int32 reply;
	if (link.FlushWithReply(reply) == B_OK && reply == SERVER_TRUE)
		return B_OK;
	
	return B_ERROR;
}


uint32
BPrivateScreen::DPMSState()
{
	uint32 state = 0;
	
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_DPMS_STATE);
	link.Attach<screen_id>(ID());
	int32 reply;
	if (link.FlushWithReply(reply) == B_OK && reply == SERVER_TRUE)
		link.Read<uint32>(&state);
	
	return state;
}


uint32
BPrivateScreen::DPMSCapabilites()
{
	uint32 capabilities = 0;
	
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_DPMS_CAPABILITIES);
	link.Attach<screen_id>(ID());
	int32 reply;
	if (link.FlushWithReply(reply) == B_OK && reply == SERVER_TRUE)
		link.Read<uint32>(&capabilities);
	
	return capabilities;
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
	status_t status = B_ERROR;
	/*
	BPrivate::AppServerLink link;
	PortMessage reply;
	link.SetOpCode(AS_GET_SCREEN_DESC);
	link.Attach<screen_id>(ID());
	link.FlushWithReply(&reply);
	reply.Read<screen_desc>(*desc);
	reply.Read<status_t>(&status);
	*/
	return status;
}


// private methods
sem_id
BPrivateScreen::RetraceSemaphore()
{
	sem_id id = B_BAD_SEM_ID;
	
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_RETRACE_SEMAPHORE);
	link.Attach<screen_id>(ID());
	int32 reply;
	if (link.FlushWithReply(reply) == B_OK
		&& reply == SERVER_TRUE) 
		link.Read<sem_id>(&id);
	
	return id;
}


BPrivateScreen::BPrivateScreen()
	:
	fColorMap(NULL),
	fRetraceSem(-1),
	fOwnsColorMap(false)
{
	// TODO: BeOS R5 here gets the colormap pointer
	// (with BApplication::ro_offset_to_ptr() ?)
	// which is contained in a shared area created by the server.
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SCREEN_GET_COLORMAP);
	link.Attach<screen_id>(ID());

	int32 reply;
	if (link.FlushWithReply(reply) == B_OK && reply == SERVER_TRUE) {
		fColorMap = (color_map *)malloc(sizeof(color_map));
		fOwnsColorMap = true;
		link.Read<color_map>(fColorMap);
	}		
}


BPrivateScreen::~BPrivateScreen()
{
	if (fOwnsColorMap)
		free(fColorMap);
}
