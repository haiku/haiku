/*
 * Copyright 2002-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*!
	BPrivateScreen is the class which does the real work for
	the proxy class BScreen (it interacts with the app_server).
*/


#include "AppMisc.h"
#include "AppServerLink.h"
#include "PrivateScreen.h"
#include "ServerProtocol.h"

#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Locker.h>
#include <ObjectList.h>
#include <Window.h>

#include <new>

#include <stdlib.h>


static BObjectList<BPrivateScreen> sScreens(2, true);

// used to synchronize creation/deletion of the sScreen object
static BLocker sScreenLock("screen lock");


using namespace BPrivate;

BPrivateScreen *
BPrivateScreen::Get(BWindow *window)
{
	screen_id id = B_MAIN_SCREEN_ID;

	if (window != NULL) {
		BPrivate::AppServerLink link;
		link.StartMessage(AS_GET_SCREEN_ID_FROM_WINDOW);
		link.Attach<int32>(_get_object_token_(window));

		status_t status;
		if (link.FlushWithReply(status) == B_OK && status == B_OK)
			link.Read<screen_id>(&id);
	}

	return _Get(id, false);
}


BPrivateScreen *
BPrivateScreen::Get(screen_id id)
{
	return _Get(id, true);
}


BPrivateScreen *
BPrivateScreen::_Get(screen_id id, bool check)
{
	// Nothing works without an app_server connection
	if (be_app == NULL)
		return NULL;

	BAutolock locker(sScreenLock);

	// search for the screen ID

	for (int32 i = sScreens.CountItems(); i-- > 0;) {
		BPrivateScreen* screen = sScreens.ItemAt(i);

		if (screen->ID().id == id.id) {
			screen->_Acquire();
			return screen;
		}
	}

	if (check) {
		// check if ID is valid
		if (!_IsValid(id))
			return NULL;
	}

	// we need to allocate a new one

	BPrivateScreen* screen = new (std::nothrow) BPrivateScreen(id);
	if (screen == NULL)
		return NULL;

	sScreens.AddItem(screen);
	return screen;
}


void
BPrivateScreen::Put(BPrivateScreen* screen)
{
	if (screen == NULL)
		return;

	BAutolock locker(sScreenLock);

	if (screen->_Release()) {
		if (screen->ID().id != B_MAIN_SCREEN_ID.id) {
			// we always keep the main screen object around - it will
			// never go away, even if you disconnect all monitors.
			sScreens.RemoveItem(screen);
		}
	}
}


BPrivateScreen*
BPrivateScreen::GetNext(BPrivateScreen* screen)
{
	BAutolock locker(sScreenLock);

	screen_id id;
	status_t status = screen->GetNextID(id);
	if (status < B_OK)
		return NULL;

	BPrivateScreen* nextScreen = Get(id);
	if (nextScreen == NULL)
		return NULL;

	Put(screen);
	return nextScreen;
}


bool
BPrivateScreen::_IsValid(screen_id id)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_VALID_SCREEN_ID);
	link.Attach<screen_id>(id);

	status_t status;
	if (link.FlushWithReply(status) != B_OK || status < B_OK)
		return false;

	return true;
}


//	#pragma mark -


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

	if (system_time() > fLastUpdate + 100000) {
		// invalidate the settings after 0.1 secs
		display_mode mode;
		if (GetMode(B_CURRENT_WORKSPACE, &mode) == B_OK) {
			fFrame.Set(0, 0, (float)mode.virtual_width - 1,
				(float)mode.virtual_height - 1);
			fLastUpdate = system_time();
		}
	}

	return fFrame;
}


bool
BPrivateScreen::IsValid() const
{
	return BPrivateScreen::_IsValid(ID());
}


status_t
BPrivateScreen::GetNextID(screen_id& id)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_NEXT_SCREEN_ID);
	link.Attach<screen_id>(ID());

	status_t status;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		link.Read<screen_id>(&id);
		return B_OK;
	}

	return status;
}


status_t
BPrivateScreen::WaitForRetrace(bigtime_t timeout)
{
	// Get the retrace semaphore if it's the first time 
	// we are called. Cache the value then.
	if (!fRetraceSemValid)
		fRetraceSem = _RetraceSemaphore();

	if (fRetraceSem < 0) {
		// syncing to retrace is not supported by the accelerant
		return fRetraceSem;
	}

	status_t status;
	do {
		status = acquire_sem_etc(fRetraceSem, 1, B_RELATIVE_TIMEOUT, timeout);
	} while (status == B_INTERRUPTED);

	return status;
}

	
uint8
BPrivateScreen::IndexForColor(uint8 red, uint8 green, uint8 blue, uint8 alpha)
{
	// Looks like this check is necessary
	if (red == B_TRANSPARENT_COLOR.red
		&& green == B_TRANSPARENT_COLOR.green
		&& blue == B_TRANSPARENT_COLOR.blue
		&& alpha == B_TRANSPARENT_COLOR.alpha)
		return B_TRANSPARENT_8_BIT;

	uint16 index = ((red & 0xf8) << 7) | ((green & 0xf8) << 2) | (blue >> 3);
	if (ColorMap())
		return fColorMap->index_map[index];

	return 0;
}


rgb_color
BPrivateScreen::ColorForIndex(const uint8 index)
{
	if (ColorMap())
		return fColorMap->color_list[index];

	return rgb_color();
}


uint8
BPrivateScreen::InvertIndex(uint8 index)
{
	if (ColorMap())
		return fColorMap->inversion_map[index];

	return 0;
}
	

const color_map *
BPrivateScreen::ColorMap()
{
	if (fColorMap == NULL) {
		BAutolock locker(sScreenLock);

		if (fColorMap != NULL) {
			// someone could have been faster than us
			return fColorMap;
		}

		// TODO: BeOS R5 here gets the colormap pointer
		// (with BApplication::ro_offset_to_ptr() ?)
		// which is contained in a shared area created by the server.
		BPrivate::AppServerLink link;
		link.StartMessage(AS_SCREEN_GET_COLORMAP);
		link.Attach<screen_id>(ID());

		status_t status;
		if (link.FlushWithReply(status) == B_OK && status == B_OK) {
			fColorMap = (color_map *)malloc(sizeof(color_map));
			fOwnsColorMap = true;
			link.Read<color_map>(fColorMap);
		}
	}

	return fColorMap;
}


status_t
BPrivateScreen::GetBitmap(BBitmap **_bitmap, bool drawCursor, BRect *bounds)
{
	if (_bitmap == NULL)
		return B_BAD_VALUE;

	BRect rect;
	if (bounds != NULL)
		rect = *bounds;
	else
		rect = Frame();

	BBitmap* bitmap = new (std::nothrow) BBitmap(rect, ColorSpace());
	if (bitmap == NULL)
		return B_NO_MEMORY;

	status_t status = bitmap->InitCheck();
	if (status == B_OK)
		status = ReadBitmap(bitmap, drawCursor, &rect);
	if (status != B_OK) {
		delete bitmap;
		return status;
	}

	*_bitmap = bitmap;
	return B_OK;
}


status_t
BPrivateScreen::ReadBitmap(BBitmap *bitmap, bool drawCursor, BRect *bounds)
{
	if (bitmap == NULL)
		return B_BAD_VALUE;

	BRect rect;
	if (bounds != NULL)
		rect = *bounds;
	else
		rect = Frame();

	BPrivate::AppServerLink link;
	link.StartMessage(AS_READ_BITMAP);
	link.Attach<int32>(bitmap->_ServerToken());
	link.Attach<bool>(drawCursor);
	link.Attach<BRect>(rect);

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) < B_OK || status != B_OK)
		return status;

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
		&& code == B_OK)
		link.Read<rgb_color>(&color);

	return color;
}


void
BPrivateScreen::SetDesktopColor(rgb_color color, uint32 workspace,
	bool makeDefault)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_SET_DESKTOP_COLOR);
	link.Attach<rgb_color>(color);
	link.Attach<uint32>(workspace);
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

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		link.Read<display_mode>(target);

		bool withinLimits;
		link.Read<bool>(&withinLimits);
		if (!withinLimits)
			status = B_BAD_VALUE;
	}

	return status;
}
		

status_t
BPrivateScreen::GetModeList(display_mode **_modeList, uint32 *_count)
{
	if (_modeList == NULL || _count == NULL)
		return B_BAD_VALUE;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_MODE_LIST);
	link.Attach<screen_id>(ID());

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		uint32 count;
		if (link.Read<uint32>(&count) < B_OK)
			return B_ERROR;

		// TODO: this could get too big for the link
		int32 size = count * sizeof(display_mode);
		display_mode* modeList = (display_mode *)malloc(size);
		if (modeList == NULL)
			return B_NO_MEMORY;

		if (link.Read(modeList, size) < B_OK) {
			free(modeList);
			return B_ERROR;
		}

		*_modeList = modeList;
		*_count = count;
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

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK
		|| status != B_OK)
		return status;

	link.Read<display_mode>(mode);
	return B_OK;
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
	link.FlushWithReply(status);

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

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		link.Read<accelerant_device_info>(info);
		return B_OK;
	}

	return status;
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

	status_t status;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		link.Read<uint32>(low);
		link.Read<uint32>(high);
		return B_OK;
	}

	return status;
}


status_t
BPrivateScreen::GetTimingConstraints(display_timing_constraints *constraints)
{
	if (constraints == NULL)
		return B_BAD_VALUE;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_TIMING_CONSTRAINTS);
	link.Attach<screen_id>(ID());

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		link.Read<display_timing_constraints>(constraints);
		return B_OK;
	}

	return status;
}


status_t
BPrivateScreen::SetDPMS(uint32 dpmsState)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_DPMS);
	link.Attach<screen_id>(ID());
	link.Attach<uint32>(dpmsState);

	status_t status = B_ERROR;
	link.FlushWithReply(status);

	return status;
}


uint32
BPrivateScreen::DPMSState()
{
	uint32 state = 0;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_DPMS_STATE);
	link.Attach<screen_id>(ID());

	status_t status;
	if (link.FlushWithReply(status) == B_OK && status == B_OK)
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

	status_t status;
	if (link.FlushWithReply(status) == B_OK && status == B_OK)
		link.Read<uint32>(&capabilities);

	return capabilities;
}


void *
BPrivateScreen::BaseAddress()
{
	frame_buffer_config config;
	if (_GetFrameBufferConfig(config) != B_OK)
		return NULL;

	return config.frame_buffer;
}


uint32
BPrivateScreen::BytesPerRow()
{
	frame_buffer_config config;
	if (_GetFrameBufferConfig(config) != B_OK)
		return 0;

	return config.bytes_per_row;
}

	
// #pragma mark - private methods


sem_id
BPrivateScreen::_RetraceSemaphore()
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_RETRACE_SEMAPHORE);
	link.Attach<screen_id>(ID());

	sem_id id = B_BAD_SEM_ID;
	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		link.Read<sem_id>(&id);
		fRetraceSemValid = true;
	}

	return id;
}


status_t
BPrivateScreen::_GetFrameBufferConfig(frame_buffer_config& config)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_FRAME_BUFFER_CONFIG);
	link.Attach<screen_id>(ID());

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		link.Read<frame_buffer_config>(&config);
		return B_OK;
	}

	return status;
}


BPrivateScreen::BPrivateScreen(screen_id id)
	:
	fID(id),
	fColorMap(NULL),
	fRetraceSem(-1),
	fRetraceSemValid(false),
	fOwnsColorMap(false),
	fFrame(0, 0, 0, 0),
	fLastUpdate(0)
{
}


BPrivateScreen::~BPrivateScreen()
{
	if (fOwnsColorMap)
		free(fColorMap);
}
