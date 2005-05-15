//------------------------------------------------------------------------------
//
// Copyright 2002-2005, Haiku, Inc.
// Distributed under the terms of the MIT License.
//
//
//	File Name:		AccelerantHWInterface.cpp
//	Authors:		Michael Lotz <mmlr@mlotz.ch>
//					DarkWyrm <bpmagic@columbus.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Accelerant based HWInterface implementation
//  
//------------------------------------------------------------------------------

#include <stdio.h>

#include <Cursor.h>

#include <Accelerant.h>
#include <graphic_driver.h>
#include <FindDirectory.h>
#include <image.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <unistd.h>

//#include "PortLink.h"
#include "RGBColor.h"
#include "ServerConfig.h"
#include "ServerCursor.h"
#include "ServerProtocol.h"

#include "AccelerantHWInterface.h"
#include "AccelerantBuffer.h"
#include "MallocBuffer.h"


#ifdef DEBUG_DRIVER_MODULE
#	include <stdio.h>
#	define ATRACE(x) printf x
#else
#	define ATRACE(x) ;
#endif

// constructor
AccelerantHWInterface::AccelerantHWInterface()
	:	HWInterface(),
		fCardFD(-1),
		fAccelerantImage(-1),
		fAccelerantHook(NULL),
		fEngineToken(NULL),
		fSyncToken(),
		
		// required hooks
		fAccAcquireEngine(NULL),
		fAccReleaseEngine(NULL),
		fAccSyncToToken(NULL),
		fAccGetModeCount(NULL),
		fAccGetModeList(NULL),
		fAccGetFrameBufferConfig(NULL),
		fAccSetDisplayMode(NULL),
		fAccGetPixelClockLimits(NULL),
		
		// optional accelerant hooks
		fAccGetTimingConstraints(NULL),
		fAccProposeDisplayMode(NULL),
		fAccFillRect(NULL),
		fAccInvertRect(NULL),
		fAccScreenBlit(NULL),
		fAccSetCursorShape(NULL),
		fAccMoveCursor(NULL),
		fAccShowCursor(NULL),
		
		// dpms hooks
		fAccDPMSCapabilities(NULL),
		fAccDPMSMode(NULL),
		fAccSetDPMSMode(NULL),		
		
		fModeCount(0),
		fModeList(NULL),
		
		fBackBuffer(NULL),
		fFrontBuffer(new AccelerantBuffer())
{
	fDisplayMode.virtual_width = 640;
	fDisplayMode.virtual_height = 480;
	fDisplayMode.space = B_RGB32;

	// NOTE: I have no clue what I'm doing here.
//	fSyncToken.counter = 0;
//	fSyncToken.engine_id = 0;
	memset(&fSyncToken, 0, sizeof(sync_token));
}

// destructor
AccelerantHWInterface::~AccelerantHWInterface()
{
	delete fBackBuffer;
	delete fFrontBuffer;
	delete fModeList;
}

// Initialize
status_t
AccelerantHWInterface::Initialize()
{
	char path[PATH_MAX];
	
	fCardFD = OpenGraphicsDevice(1);
	if (fCardFD < 0) {
		ATRACE(("Failed to open graphics device\n"));
		return B_ERROR;
	}
	
	char signature[1024];
	if (ioctl(fCardFD, B_GET_ACCELERANT_SIGNATURE, &signature, sizeof(signature)) != B_OK) {
		close(fCardFD);
		return B_ERROR;
	}
	
	ATRACE(("accelerant signature is: %s\n", signature));
	
	struct stat accelerant_stat;
	const static directory_which dirs[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY
	};
	
	fAccelerantImage = -1;
	for (int32 i = 0; i < 3; i++) {
		if (find_directory(dirs[i], -1, false, path, PATH_MAX) != B_OK)
			continue;
		
		strcat(path, "/accelerants/");
		strcat(path, signature);
		if (stat(path, &accelerant_stat) != 0)
			continue;
		
		fAccelerantImage = load_add_on(path);
		if (fAccelerantImage >= 0) {
			if (get_image_symbol(fAccelerantImage, B_ACCELERANT_ENTRY_POINT,
				B_SYMBOL_TYPE_ANY, (void**)(&fAccelerantHook)) != B_OK ) {
				ATRACE(("unable to get B_ACCELERANT_ENTRY_POINT\n"));
				unload_add_on(fAccelerantImage);
				fAccelerantImage = -1;
				return B_ERROR;
			}
			
			init_accelerant InitAccelerant;
			InitAccelerant = (init_accelerant)fAccelerantHook(B_INIT_ACCELERANT, NULL);
			if (!InitAccelerant || InitAccelerant(fCardFD) != B_OK) {
				// TODO: continue / go on to the next graphics card?
				ATRACE(("InitAccelerant unsuccessful\n"));
				unload_add_on(fAccelerantImage);
				fAccelerantImage = -1;
				return B_ERROR;
			}
			
			break;
		}
	}
	
	if (fAccelerantImage < 0)
		return B_ERROR;
	
	if (SetupDefaultHooks() != B_OK) {
		ATRACE(("cannot setup default hooks\n"));
		return B_ERROR;
	}
	
	return B_OK;
}

/*!
	\brief Opens a graphics device for read-write access
	\param deviceNumber Number identifying which graphics card to open (1 for first card)
	\return The file descriptor for the opened graphics device
	
	The deviceNumber is relative to the number of graphics devices that can be successfully
	opened.  One represents the first card that can be successfully opened (not necessarily
	the first one listed in the directory).
*/
int
AccelerantHWInterface::OpenGraphicsDevice(int deviceNumber)
{
	DIR *directory = opendir("/dev/graphics");
	if (!directory)
		return -1;
	
	int count = 0;
	struct dirent *entry;
	int current_card_fd = -1;
	char path[PATH_MAX];
	while ((count < deviceNumber) && ((entry = readdir(directory)) != NULL)) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") ||
			!strcmp(entry->d_name, "stub"))
			continue;
		if (current_card_fd >= 0) {
			close(current_card_fd);
			current_card_fd = -1;
		}
		
		sprintf(path, "/dev/graphics/%s", entry->d_name);
		current_card_fd = open(path, B_READ_WRITE);
		if (current_card_fd >= 0)
			count++;
	}
	
	// open stub if we were not able to get a "real" card
	if (count < deviceNumber) {
		if (deviceNumber == 1) {
			sprintf(path, "/dev/graphics/stub");
			current_card_fd = open(path, B_READ_WRITE);
		} else {
			close(current_card_fd);
			current_card_fd = -1;
		}
	}
	
	return current_card_fd;
}

status_t
AccelerantHWInterface::SetupDefaultHooks()
{
	// required
	fAccAcquireEngine = (acquire_engine)fAccelerantHook(B_ACQUIRE_ENGINE, NULL);
	fAccReleaseEngine = (release_engine)fAccelerantHook(B_RELEASE_ENGINE, NULL);
	fAccSyncToToken = (sync_to_token)fAccelerantHook(B_SYNC_TO_TOKEN, NULL);
	fAccGetModeCount = (accelerant_mode_count)fAccelerantHook(B_ACCELERANT_MODE_COUNT, NULL);
	fAccGetModeList = (get_mode_list)fAccelerantHook(B_GET_MODE_LIST, NULL);
	fAccGetFrameBufferConfig = (get_frame_buffer_config)fAccelerantHook(B_GET_FRAME_BUFFER_CONFIG, NULL);
	fAccSetDisplayMode = (set_display_mode)fAccelerantHook(B_SET_DISPLAY_MODE, NULL);
	fAccGetPixelClockLimits = (get_pixel_clock_limits)fAccelerantHook(B_GET_PIXEL_CLOCK_LIMITS, NULL);
	
	if (!fAccAcquireEngine || !fAccReleaseEngine || !fAccGetFrameBufferConfig
		|| !fAccGetModeCount || !fAccGetModeList || !fAccSetDisplayMode
		|| !fAccGetPixelClockLimits) {
		return B_ERROR;
	}
	
	// optional
	fAccGetTimingConstraints = (get_timing_constraints)fAccelerantHook(B_GET_TIMING_CONSTRAINTS, NULL);
	fAccProposeDisplayMode = (propose_display_mode)fAccelerantHook(B_PROPOSE_DISPLAY_MODE, NULL);
	
	fAccFillRect = (fill_rectangle)fAccelerantHook(B_FILL_RECTANGLE, NULL);
	fAccInvertRect = (invert_rectangle)fAccelerantHook(B_INVERT_RECTANGLE, NULL);
	fAccScreenBlit = (screen_to_screen_blit)fAccelerantHook(B_SCREEN_TO_SCREEN_BLIT, NULL);
	
	fAccSetCursorShape = (set_cursor_shape)fAccelerantHook(B_SET_CURSOR_SHAPE, NULL);
	fAccMoveCursor = (move_cursor)fAccelerantHook(B_MOVE_CURSOR, NULL);
	fAccShowCursor = (show_cursor)fAccelerantHook(B_SHOW_CURSOR, NULL);
	
	// dpms
	fAccDPMSCapabilities = (dpms_capabilities)fAccelerantHook(B_DPMS_CAPABILITIES, NULL);
	fAccDPMSMode = (dpms_mode)fAccelerantHook(B_DPMS_MODE, NULL);
	fAccSetDPMSMode = (set_dpms_mode)fAccelerantHook(B_SET_DPMS_MODE, NULL);
	
	return B_OK;
}

// Shutdown
status_t
AccelerantHWInterface::Shutdown()
{
	if (fAccelerantHook) {
		uninit_accelerant UninitAccelerant = (uninit_accelerant)fAccelerantHook(B_UNINIT_ACCELERANT, NULL);
		if (UninitAccelerant)
			UninitAccelerant();
	}
	
	if (fAccelerantImage >= 0)
		unload_add_on(fAccelerantImage);
	
	if (fCardFD >= 0)
		close(fCardFD);
	
	return B_OK;
}

// SetMode
status_t
AccelerantHWInterface::SetMode(const display_mode &mode)
{
	// TODO: There are places this function can fail,
	// maybe it needs to roll back changes in case of an
	// error.

	// prevent from doing the unnecessary
	if (fModeCount > 0 && fBackBuffer && fFrontBuffer
		&& fDisplayMode.virtual_width == mode.virtual_width
		&& fDisplayMode.virtual_height == mode.virtual_height
		&& fDisplayMode.space == mode.space) {
		return B_OK;
	}
	
	// TODO: check if the mode is valid even (ie complies to the modes we said we would support)
	// or else ret = B_BAD_VALUE
	
	if (fModeCount <= 0 || !fModeList) {
		if (UpdateModeList() != B_OK || fModeCount <= 0) {
			ATRACE(("unable to update mode list\n"));
			return B_ERROR;
		}
	}
	
	for (int32 i = 0; i < fModeCount; i++) {
		if (fModeList[i].virtual_width == mode.virtual_width
			&& fModeList[i].virtual_height == mode.virtual_height
			&& fModeList[i].space == mode.space) {
			
			fDisplayMode = fModeList[i];
			break;
		}
	}
	
	if (fAccSetDisplayMode(&fDisplayMode) != B_OK) {
		ATRACE(("setting display mode failed\n"));
		return B_ERROR;
	}
	
	// update frontbuffer
	fFrontBuffer->SetDisplayMode(fDisplayMode);
	if (UpdateFrameBufferConfig() != B_OK)
		return B_ERROR;
	
	// update backbuffer if neccessary
	if (!fBackBuffer || fBackBuffer->Width() != fDisplayMode.virtual_width
		|| fBackBuffer->Height() != fDisplayMode.virtual_height) {
		// NOTE: backbuffer is always B_RGBA32, this simplifies the
		// drawing backend implementation tremendously for the time
		// being. The color space conversion is handled in CopyBackToFront()

		delete fBackBuffer;
		fBackBuffer = NULL;

		// TODO: Above not true anymore for single buffered mode!!!
		// -> fall back to double buffer for fDisplayMode.space != B_RGB32
		// as intermediate solution...
		bool doubleBuffered = HWInterface::IsDoubleBuffered();
		if ((color_space)fDisplayMode.space != B_RGB32 &&
			(color_space)fDisplayMode.space != B_RGBA32)
			doubleBuffered = true;
		
		if (doubleBuffered) {
			fBackBuffer = new MallocBuffer(fDisplayMode.virtual_width,
										   fDisplayMode.virtual_height);
			
			status_t ret = fBackBuffer->InitCheck();
			if (ret < B_OK) {
				delete fBackBuffer;
				fBackBuffer = NULL;
				return ret;
			}
			// clear out backbuffer, alpha is 255 this way
			memset(fBackBuffer->Bits(), 255, fBackBuffer->BitsLength());
		}
	}
	
	return B_OK;
}

void
AccelerantHWInterface::GetMode(display_mode *mode)
{
	if (mode)
		*mode = fDisplayMode;
}

status_t
AccelerantHWInterface::UpdateModeList()
{
	fModeCount = fAccGetModeCount();
	if (fModeCount <= 0)
		return B_ERROR;
	
	fModeList = new display_mode[fModeCount];;
	if (!fModeList)
		return B_NO_MEMORY;
	
	if (fAccGetModeList(fModeList) != B_OK) {
		ATRACE(("unable to get mode list\n"));
		return B_ERROR;
	}
	
	return B_OK;
}

status_t
AccelerantHWInterface::UpdateFrameBufferConfig()
{
	if (fAccGetFrameBufferConfig(&fFrameBufferConfig) != B_OK) {
		ATRACE(("unable to get frame buffer config\n"));
		return B_ERROR;
	}
	
	fFrontBuffer->SetFrameBufferConfig(fFrameBufferConfig);
	return B_OK;
}

// GetDeviceInfo
status_t
AccelerantHWInterface::GetDeviceInfo(accelerant_device_info *info)
{
	get_accelerant_device_info GetAccelerantDeviceInfo = (get_accelerant_device_info)fAccelerantHook(B_GET_ACCELERANT_DEVICE_INFO, NULL);
	if (!GetAccelerantDeviceInfo) {
		ATRACE(("No B_GET_ACCELERANT_DEVICE_INFO hook found\n"));
		return B_UNSUPPORTED;
	}
	
	return GetAccelerantDeviceInfo(info);
}

// GetModeList
status_t
AccelerantHWInterface::GetModeList(display_mode **modes, uint32 *count)
{
	if (!count || !modes)
		return B_BAD_VALUE;
	
	*modes = new display_mode[fModeCount];
	*count = fModeCount;
	
	memcpy(*modes, fModeList, sizeof(display_mode) * fModeCount);
	return B_OK;
}

status_t
AccelerantHWInterface::GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high)
{
	if (!mode || !low || !high)
		return B_BAD_VALUE;
	
	return fAccGetPixelClockLimits(mode, low, high);
}

status_t
AccelerantHWInterface::GetTimingConstraints(display_timing_constraints *dtc)
{
	if (!dtc)
		return B_BAD_VALUE;
	
	if (fAccGetTimingConstraints)
		return fAccGetTimingConstraints(dtc);
	
	return B_UNSUPPORTED;
}

status_t
AccelerantHWInterface::ProposeMode(display_mode *candidate, const display_mode *low, const display_mode *high)
{
	if (!candidate || !low || !high)
		return B_BAD_VALUE;
	
	if (!fAccProposeDisplayMode)
		return B_UNSUPPORTED;
	
	// avoid const issues
	display_mode this_high, this_low;
	this_high = *high;
	this_low = *low;
	
	return fAccProposeDisplayMode(candidate, &this_low, &this_high);
}

// WaitForRetrace
status_t
AccelerantHWInterface::WaitForRetrace(bigtime_t timeout = B_INFINITE_TIMEOUT)
{
	accelerant_retrace_semaphore AccelerantRetraceSemaphore = (accelerant_retrace_semaphore)fAccelerantHook(B_ACCELERANT_RETRACE_SEMAPHORE, NULL);
	if (!AccelerantRetraceSemaphore)
		return B_UNSUPPORTED;
	
	sem_id sem = AccelerantRetraceSemaphore();
	if (sem < 0)
		return B_ERROR;
	
	return acquire_sem_etc(sem, 1, B_RELATIVE_TIMEOUT, timeout);
}

// SetDPMSMode
status_t
AccelerantHWInterface::SetDPMSMode(const uint32 &state)
{
	if (!fAccSetDPMSMode)
		return B_UNSUPPORTED;
	
	return fAccSetDPMSMode(state);
}

// DPMSMode
uint32
AccelerantHWInterface::DPMSMode() const
{
	if (!fAccDPMSMode)
		return B_UNSUPPORTED;
	
	return fAccDPMSMode();
}

// DPMSCapabilities
uint32
AccelerantHWInterface::DPMSCapabilities() const
{
	if (!fAccDPMSCapabilities)
		return B_UNSUPPORTED;
	
	return fAccDPMSCapabilities();
}

// AvailableHardwareAcceleration
uint32
AccelerantHWInterface::AvailableHWAcceleration() const
{
	uint32 flags = 0;

	if (fAccScreenBlit)
		flags |= HW_ACC_COPY_REGION;
	if (fAccFillRect)
		flags |= HW_ACC_FILL_REGION;
	if (fAccInvertRect)
		flags |= HW_ACC_INVERT_REGION;

	return flags;
}

// CopyRegion
void
AccelerantHWInterface::CopyRegion(const clipping_rect* sortedRectList,
								  uint32 count, int32 xOffset, int32 yOffset)
{
	if (fAccScreenBlit && fAccAcquireEngine) {
		if (fAccAcquireEngine(B_2D_ACCELERATION, 0xff, &fSyncToken, &fEngineToken) >= B_OK) {

			// convert the rects
			blit_params* params = new blit_params[count];
			for (uint32 i = 0; i < count; i++) {
				params[i].src_left = (uint16)sortedRectList[i].left;
				params[i].src_top = (uint16)sortedRectList[i].top;

				params[i].dest_left = (uint16)sortedRectList[i].left + xOffset;
				params[i].dest_top = (uint16)sortedRectList[i].top + yOffset;

				// NOTE: width and height are expressed as distance, not pixel count!
				params[i].width = (uint16)(sortedRectList[i].right - sortedRectList[i].left);
				params[i].height = (uint16)(sortedRectList[i].bottom - sortedRectList[i].top);
			}

			// go
			fAccScreenBlit(fEngineToken, params, count);

			// done
			if (fAccReleaseEngine)
				fAccReleaseEngine(fEngineToken, &fSyncToken);

			// sync
			if (fAccSyncToToken)
				fAccSyncToToken(&fSyncToken);

			delete[] params;
		}
	}
}

// FillRegion
void
AccelerantHWInterface::FillRegion(/*const*/ BRegion& region, const RGBColor& color)
{
	if (fAccFillRect && fAccAcquireEngine) {
		if (fAccAcquireEngine(B_2D_ACCELERATION, 0xff, &fSyncToken, &fEngineToken) >= B_OK) {

			// convert the region
			uint32 count;
			fill_rect_params* fillParams;
			_RegionToRectParams(&region, &fillParams, &count);

			// go
			fAccFillRect(fEngineToken, _NativeColor(color), fillParams, count);

			// done
			if (fAccReleaseEngine)
				fAccReleaseEngine(fEngineToken, &fSyncToken);

			// sync
			if (fAccSyncToToken)
				fAccSyncToToken(&fSyncToken);

			delete[] fillParams;
		}
	}
}

// InvertRegion
void
AccelerantHWInterface::InvertRegion(/*const*/ BRegion& region)
{
	if (fAccInvertRect && fAccAcquireEngine) {
		if (fAccAcquireEngine(B_2D_ACCELERATION, 0xff, &fSyncToken, &fEngineToken) >= B_OK) {

			// convert the region
			uint32 count;
			fill_rect_params* fillParams;
			_RegionToRectParams(&region, &fillParams, &count);

			// go
			fAccInvertRect(fEngineToken, fillParams, count);

			// done
			if (fAccReleaseEngine)
				fAccReleaseEngine(fEngineToken, &fSyncToken);

			// sync
			if (fAccSyncToToken)
				fAccSyncToToken(&fSyncToken);

			delete[] fillParams;
		}
	}
}

// SetCursor
void
AccelerantHWInterface::SetCursor(ServerCursor* cursor)
{
	if (Lock()) {
		HWInterface::SetCursor(cursor);
		// TODO: implement setting the hard ware cursor
		// NOTE: cursor should be always B_RGBA32
		// NOTE: The HWInterface implementation should
		// still be called, since it takes ownership of
		// the cursor.
		Unlock();
	}
}

// SetCursorVisible
void
AccelerantHWInterface::SetCursorVisible(bool visible)
{
	if (Lock()) {
		HWInterface::SetCursorVisible(visible);
		// TODO: update graphics hardware
		Unlock();
	}
}

// MoveCursorTo
void
AccelerantHWInterface::MoveCursorTo(const float& x, const float& y)
{
	if (Lock()) {
		HWInterface::MoveCursorTo(x, y);
		// TODO: update graphics hardware
		Unlock();
	}
}

// FrontBuffer
RenderingBuffer *
AccelerantHWInterface::FrontBuffer() const
{
	if (!fModeList)
		return NULL;
	
	return fFrontBuffer;
}

// BackBuffer
RenderingBuffer *
AccelerantHWInterface::BackBuffer() const
{
	if (!fModeList)
		return NULL;
	
	return fBackBuffer;
}

// _DrawCursor
void
AccelerantHWInterface::_DrawCursor(BRect area) const
{
	// use the default implementation for now,
	// until we have a hardware cursor
	HWInterface::_DrawCursor(area);
	// TODO: this would only be called, if we don't have
	// a hardware cursor for some reason
}

// _RegionToRectParams
void
AccelerantHWInterface::_RegionToRectParams(/*const*/ BRegion* region,
										   fill_rect_params** params,
										   uint32* count) const
{
	*count = region->CountRects();
	*params = new fill_rect_params[*count];

	for (uint32 i = 0; i < *count; i++) {
		clipping_rect r = region->RectAtInt(i);
		(*params)[i].left = (uint16)r.left;
		(*params)[i].top = (uint16)r.top;
		(*params)[i].right = (uint16)r.right;
		(*params)[i].bottom = (uint16)r.bottom;
	}
}

// _NativeColor
uint32
AccelerantHWInterface::_NativeColor(const RGBColor& color) const
{
	// NOTE: This functions looks somehow suspicios to me.
	// It assumes that all graphics cards have the same native endianess, no?
	switch (fDisplayMode.space) {
		case B_CMAP8:
		case B_GRAY8:
			return color.GetColor8();

		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			return color.GetColor15();

		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			return color.GetColor16();

		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE: {
			rgb_color c = color.GetColor32();
			uint32 native = (c.alpha << 24) |
							(c.red << 16) |
							(c.green << 8) |
							(c.blue);
			return native;
		}
	}
	return 0;
}
