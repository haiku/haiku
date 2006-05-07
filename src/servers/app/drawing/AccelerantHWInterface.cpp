/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/**	Accelerant based HWInterface implementation */


#include "AccelerantHWInterface.h"

#include "AccelerantBuffer.h"
#include "MallocBuffer.h"
#include "Overlay.h"
#include "RGBColor.h"
#include "ServerConfig.h"
#include "ServerCursor.h"
#include "ServerProtocol.h"
#include "SystemPalette.h"

#include <Accelerant.h>
#include <Cursor.h>
#include <FindDirectory.h>
#include <graphic_driver.h>
#include <image.h>

#include <dirent.h>
#include <new>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

using std::nothrow;


#ifdef DEBUG_DRIVER_MODULE
#	include <stdio.h>
#	define ATRACE(x) printf x
#else
#	define ATRACE(x) ;
#endif


// This call updates the frame buffer used by the on-screen KDL
#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
extern "C" status_t _kern_frame_buffer_update(void *baseAddress,
						int32 width, int32 height,
						int32 depth, int32 bytesPerRow);
#endif


bool
operator==(const display_mode& a, const display_mode& b)
{
	return memcmp(&a, &b, sizeof(display_mode)) == 0;
}

const int32 kDefaultParamsCount = 64;

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
		fAccGetDisplayMode(NULL),
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
		fFrontBuffer(new (nothrow) AccelerantBuffer()),

		fRectParams(new (nothrow) fill_rect_params[kDefaultParamsCount]),
		fRectParamsCount(kDefaultParamsCount),
		fBlitParams(new (nothrow) blit_params[kDefaultParamsCount]),
		fBlitParamsCount(kDefaultParamsCount)
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

	delete[] fRectParams;
	delete[] fBlitParams;

	delete[] fModeList;
}


/*!
	\brief Opens the first available graphics device and initializes it
	\return B_OK on success or an appropriate error message on failure.
*/
status_t
AccelerantHWInterface::Initialize()
{
	status_t ret = HWInterface::Initialize();

	if (!fRectParams || !fBlitParams)
		return B_NO_MEMORY;

	if (ret >= B_OK) {
		for (int32 i = 1; fCardFD != B_ENTRY_NOT_FOUND; i++) {
			fCardFD = _OpenGraphicsDevice(i);
			if (fCardFD < 0) {
				ATRACE(("Failed to open graphics device\n"));
				continue;
			}
	
			if (_OpenAccelerant(fCardFD) == B_OK)
				break;
	
			close(fCardFD);
			// _OpenAccelerant() failed, try to open next graphics card
		}
	
		return fCardFD >= 0 ? B_OK : fCardFD;
	}
	return ret;
}


/*!
	\brief Opens a graphics device for read-write access
	\param deviceNumber Number identifying which graphics card to open (1 for first card)
	\return The file descriptor for the opened graphics device
	
	The deviceNumber is relative to the number of graphics devices that can be successfully
	opened.  One represents the first card that can be successfully opened (not necessarily
	the first one listed in the directory).
	Graphics drivers must be able to be opened more than once, so we really get
	the first working entry.
*/
int
AccelerantHWInterface::_OpenGraphicsDevice(int deviceNumber)
{
	DIR *directory = opendir("/dev/graphics");
	if (!directory)
		return -1;

	// ToDo: the former R5 "stub" driver is called "vesa" under Haiku; however,
	//	we do not need to avoid this driver this way when is has been ported
	//	to the new driver architecture - the special case here can then be
	//	removed.
	int count = 0;
	struct dirent *entry;
	int device = -1;
	char path[PATH_MAX];
	while (count < deviceNumber && (entry = readdir(directory)) != NULL) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") ||
			!strcmp(entry->d_name, "stub") || !strcmp(entry->d_name, "vesa"))
			continue;

		if (device >= 0) {
			close(device);
			device = -1;
		}

		sprintf(path, "/dev/graphics/%s", entry->d_name);
		device = open(path, B_READ_WRITE);
		if (device >= 0)
			count++;
	}

	// Open VESA driver if we were not able to get a better one
	if (count < deviceNumber) {
		if (deviceNumber == 1) {
			sprintf(path, "/dev/graphics/vesa");
			device = open(path, B_READ_WRITE);
		} else {
			close(device);
			device = B_ENTRY_NOT_FOUND;
		}
	}

	return device;
}


status_t
AccelerantHWInterface::_OpenAccelerant(int device)
{
	char signature[1024];
	if (ioctl(device, B_GET_ACCELERANT_SIGNATURE, 
			&signature, sizeof(signature)) != B_OK)
		return B_ERROR;

	ATRACE(("accelerant signature is: %s\n", signature));

	struct stat accelerant_stat;
	const static directory_which dirs[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY
	};

	fAccelerantImage = -1;

	for (int32 i = 0; i < 3; i++) {
		char path[PATH_MAX];
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

			init_accelerant initAccelerant;
			initAccelerant = (init_accelerant)fAccelerantHook(B_INIT_ACCELERANT, NULL);
			if (!initAccelerant || initAccelerant(device) != B_OK) {
				ATRACE(("InitAccelerant unsuccessful\n"));
				unload_add_on(fAccelerantImage);
				fAccelerantImage = -1;
				return B_ERROR;
			}

			break;
		}
	}

	if (fAccelerantImage < B_OK)
		return B_ERROR;

	if (_SetupDefaultHooks() != B_OK) {
		ATRACE(("cannot setup default hooks\n"));

		uninit_accelerant uninitAccelerant = (uninit_accelerant)
			fAccelerantHook(B_UNINIT_ACCELERANT, NULL);
		if (uninitAccelerant != NULL)
			uninitAccelerant();

		unload_add_on(fAccelerantImage);
		return B_ERROR;
	}

	return B_OK;
}


status_t
AccelerantHWInterface::_SetupDefaultHooks()
{
	// required
	fAccAcquireEngine = (acquire_engine)fAccelerantHook(B_ACQUIRE_ENGINE, NULL);
	fAccReleaseEngine = (release_engine)fAccelerantHook(B_RELEASE_ENGINE, NULL);
	fAccSyncToToken = (sync_to_token)fAccelerantHook(B_SYNC_TO_TOKEN, NULL);
	fAccGetModeCount = (accelerant_mode_count)fAccelerantHook(B_ACCELERANT_MODE_COUNT, NULL);
	fAccGetModeList = (get_mode_list)fAccelerantHook(B_GET_MODE_LIST, NULL);
	fAccGetFrameBufferConfig = (get_frame_buffer_config)fAccelerantHook(B_GET_FRAME_BUFFER_CONFIG, NULL);
	fAccSetDisplayMode = (set_display_mode)fAccelerantHook(B_SET_DISPLAY_MODE, NULL);
	fAccGetDisplayMode = (get_display_mode)fAccelerantHook(B_GET_DISPLAY_MODE, NULL);
	fAccGetPixelClockLimits = (get_pixel_clock_limits)fAccelerantHook(B_GET_PIXEL_CLOCK_LIMITS, NULL);

	if (!fAccAcquireEngine || !fAccReleaseEngine || !fAccGetFrameBufferConfig
		|| !fAccGetModeCount || !fAccGetModeList || !fAccSetDisplayMode
		|| !fAccGetDisplayMode || !fAccGetPixelClockLimits) {
		return B_ERROR;
	}

	// optional
	fAccGetTimingConstraints = (get_timing_constraints)fAccelerantHook(B_GET_TIMING_CONSTRAINTS, NULL);
	fAccProposeDisplayMode = (propose_display_mode)fAccelerantHook(B_PROPOSE_DISPLAY_MODE, NULL);

	// cursor
	fAccSetCursorShape = (set_cursor_shape)fAccelerantHook(B_SET_CURSOR_SHAPE, NULL);
	fAccMoveCursor = (move_cursor)fAccelerantHook(B_MOVE_CURSOR, NULL);
	fAccShowCursor = (show_cursor)fAccelerantHook(B_SHOW_CURSOR, NULL);

	// dpms
	fAccDPMSCapabilities = (dpms_capabilities)fAccelerantHook(B_DPMS_CAPABILITIES, NULL);
	fAccDPMSMode = (dpms_mode)fAccelerantHook(B_DPMS_MODE, NULL);
	fAccSetDPMSMode = (set_dpms_mode)fAccelerantHook(B_SET_DPMS_MODE, NULL);

	// overlay
	fAccOverlayCount = (overlay_count)fAccelerantHook(B_OVERLAY_COUNT, NULL);
	fAccOverlaySupportedSpaces = (overlay_supported_spaces)fAccelerantHook(B_OVERLAY_SUPPORTED_SPACES, NULL);
	fAccOverlaySupportedFeatures = (overlay_supported_features)fAccelerantHook(B_OVERLAY_SUPPORTED_FEATURES, NULL);
	fAccAllocateOverlayBuffer = (allocate_overlay_buffer)fAccelerantHook(B_ALLOCATE_OVERLAY_BUFFER, NULL);
	fAccReleaseOverlayBuffer = (release_overlay_buffer)fAccelerantHook(B_RELEASE_OVERLAY_BUFFER, NULL);
	fAccGetOverlayConstraints = (get_overlay_constraints)fAccelerantHook(B_GET_OVERLAY_CONSTRAINTS, NULL);
	fAccAllocateOverlay = (allocate_overlay)fAccelerantHook(B_ALLOCATE_OVERLAY, NULL);
	fAccReleaseOverlay = (release_overlay)fAccelerantHook(B_RELEASE_OVERLAY, NULL);
	fAccConfigureOverlay = (configure_overlay)fAccelerantHook(B_CONFIGURE_OVERLAY, NULL);

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
	AutoWriteLocker _(this);
	// TODO: There are places this function can fail,
	// maybe it needs to roll back changes in case of an
	// error.

	// prevent from doing the unnecessary
	if (fModeCount > 0 && fBackBuffer && fFrontBuffer
		&& fDisplayMode == mode) {
		// TODO: better comparison of display modes
		return B_OK;
	}

	// some safety checks
	// TODO: more of those!
	if (mode.virtual_width < 320
		|| mode.virtual_height < 200)
		return B_BAD_VALUE;

	// just try to set the mode - we let the graphics driver
	// approve or deny the request, as it should know best
	display_mode new_mode = mode;
	if (fAccSetDisplayMode(&new_mode) != B_OK) {
		ATRACE(("setting display mode failed\n"));
		return B_ERROR;
	}
	fDisplayMode = new_mode;

	// update frontbuffer
	fFrontBuffer->SetDisplayMode(fDisplayMode);
	if (_UpdateFrameBufferConfig() != B_OK)
		return B_ERROR;

	// Update the frame buffer used by the on-screen KDL
#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
	uint32 depth = (fFrameBufferConfig.bytes_per_row / fDisplayMode.virtual_width) << 3;
	if (fDisplayMode.space == B_RGB15)
		depth = 15;

	_kern_frame_buffer_update(fFrameBufferConfig.frame_buffer,
		fDisplayMode.virtual_width, fDisplayMode.virtual_height,
		depth, fFrameBufferConfig.bytes_per_row);
#endif

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
		if ((color_space)fDisplayMode.space != B_RGB32
			&& (color_space)fDisplayMode.space != B_RGBA32)
			doubleBuffered = true;

		if (doubleBuffered) {
			fBackBuffer = new(nothrow) MallocBuffer(fDisplayMode.virtual_width,
													fDisplayMode.virtual_height);

			status_t ret = fBackBuffer ? fBackBuffer->InitCheck() : B_NO_MEMORY;
			if (ret < B_OK) {
				delete fBackBuffer;
				fBackBuffer = NULL;
				return ret;
			}
			// clear out backbuffer, alpha is 255 this way
			memset(fBackBuffer->Bits(), 255, fBackBuffer->BitsLength());
		}
	}

	if (fDisplayMode.space == B_CMAP8)
		_SetSystemPalette();

	// update acceleration hooks
	fAccFillRect = (fill_rectangle)fAccelerantHook(B_FILL_RECTANGLE, (void *)&fDisplayMode);
	fAccInvertRect = (invert_rectangle)fAccelerantHook(B_INVERT_RECTANGLE,
		(void *)&fDisplayMode);
	fAccScreenBlit = (screen_to_screen_blit)fAccelerantHook(B_SCREEN_TO_SCREEN_BLIT,
		(void *)&fDisplayMode);

	return B_OK;
}


void
AccelerantHWInterface::GetMode(display_mode *mode)
{
	if (mode && ReadLock()) {
		*mode = fDisplayMode;
		ReadUnlock();
	}
}


status_t
AccelerantHWInterface::_UpdateModeList()
{
	fModeCount = fAccGetModeCount();
	if (fModeCount <= 0)
		return B_ERROR;

	delete[] fModeList;
	fModeList = new(nothrow) display_mode[fModeCount];
	if (!fModeList)
		return B_NO_MEMORY;

	if (fAccGetModeList(fModeList) != B_OK) {
		ATRACE(("unable to get mode list\n"));
		return B_ERROR;
	}
	
	return B_OK;
}


status_t
AccelerantHWInterface::_UpdateFrameBufferConfig()
{
	if (fAccGetFrameBufferConfig(&fFrameBufferConfig) != B_OK) {
		ATRACE(("unable to get frame buffer config\n"));
		return B_ERROR;
	}

	fFrontBuffer->SetFrameBufferConfig(fFrameBufferConfig);
	return B_OK;
}


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


status_t
AccelerantHWInterface::GetFrameBufferConfig(frame_buffer_config& config)
{
	config = fFrameBufferConfig;
	return B_OK;
}


status_t
AccelerantHWInterface::GetModeList(display_mode** modes, uint32 *count)
{
	AutoReadLocker _(this);

	if (!count || !modes)
		return B_BAD_VALUE;

	status_t ret = fModeList ? B_OK : _UpdateModeList();	

	if (ret >= B_OK) {
		*modes = new(nothrow) display_mode[fModeCount];
		if (*modes) {
			*count = fModeCount;
			memcpy(*modes, fModeList, sizeof(display_mode) * fModeCount);
		} else {
			*count = 0;
			ret = B_NO_MEMORY;
		}
	}
	return ret;
}


status_t
AccelerantHWInterface::GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high)
{
	AutoReadLocker _(this);

	if (!mode || !low || !high)
		return B_BAD_VALUE;
	
	return fAccGetPixelClockLimits(mode, low, high);
}


status_t
AccelerantHWInterface::GetTimingConstraints(display_timing_constraints *dtc)
{
	AutoReadLocker _(this);

	if (!dtc)
		return B_BAD_VALUE;
	
	if (fAccGetTimingConstraints)
		return fAccGetTimingConstraints(dtc);
	
	return B_UNSUPPORTED;
}


status_t
AccelerantHWInterface::ProposeMode(display_mode *candidate, const display_mode *low, const display_mode *high)
{
	AutoReadLocker _(this);

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


sem_id
AccelerantHWInterface::RetraceSemaphore()
{
	accelerant_retrace_semaphore AccelerantRetraceSemaphore =
		(accelerant_retrace_semaphore)fAccelerantHook(B_ACCELERANT_RETRACE_SEMAPHORE, NULL);
	if (!AccelerantRetraceSemaphore)
		return B_UNSUPPORTED;
		
	return AccelerantRetraceSemaphore();
}


status_t
AccelerantHWInterface::WaitForRetrace(bigtime_t timeout)
{
	AutoReadLocker _(this);

	accelerant_retrace_semaphore AccelerantRetraceSemaphore = (accelerant_retrace_semaphore)fAccelerantHook(B_ACCELERANT_RETRACE_SEMAPHORE, NULL);
	if (!AccelerantRetraceSemaphore)
		return B_UNSUPPORTED;
	
	sem_id sem = AccelerantRetraceSemaphore();
	if (sem < 0)
		return B_ERROR;
	
	return acquire_sem_etc(sem, 1, B_RELATIVE_TIMEOUT, timeout);
}


status_t
AccelerantHWInterface::SetDPMSMode(const uint32 &state)
{
	AutoWriteLocker _(this);

	if (!fAccSetDPMSMode)
		return B_UNSUPPORTED;
	
	return fAccSetDPMSMode(state);
}

// DPMSMode
uint32
AccelerantHWInterface::DPMSMode()
{
	AutoReadLocker _(this);

	if (!fAccDPMSMode)
		return B_UNSUPPORTED;
	
	return fAccDPMSMode();
}

// DPMSCapabilities
uint32
AccelerantHWInterface::DPMSCapabilities()
{
	AutoReadLocker _(this);

	if (!fAccDPMSCapabilities)
		return B_UNSUPPORTED;
	
	return fAccDPMSCapabilities();
}


status_t
AccelerantHWInterface::GetAccelerantPath(BString &string)
{
	image_info info;
	status_t status = get_image_info(fAccelerantImage, &info);
	if (status == B_OK)
		string = info.name;
	return status;
}


status_t
AccelerantHWInterface::GetDriverPath(BString &string)
{
	// TODO: this currently assumes that the accelerant's clone info
	//	is always the path name of its driver (that's the case for
	//	all of our drivers)
	char path[B_PATH_NAME_LENGTH];
	get_accelerant_clone_info getCloneInfo;
	getCloneInfo = (get_accelerant_clone_info)fAccelerantHook(B_GET_ACCELERANT_CLONE_INFO, NULL);
	
	if (getCloneInfo == NULL)
		return B_NOT_SUPPORTED;

	getCloneInfo((void *)path);
	string.SetTo(path);
	return B_OK;
}


// AvailableHardwareAcceleration
uint32
AccelerantHWInterface::AvailableHWAcceleration() const
{
	uint32 flags = 0;

	if (!IsDoubleBuffered()) {
		if (fAccScreenBlit)
			flags |= HW_ACC_COPY_REGION;
		if (fAccFillRect)
			flags |= HW_ACC_FILL_REGION;
		if (fAccInvertRect)
			flags |= HW_ACC_INVERT_REGION;
	}

	return flags;
}


overlay_token
AccelerantHWInterface::AcquireOverlayChannel()
{
	if (fAccAllocateOverlay == NULL
		|| fAccReleaseOverlay == NULL)
		return NULL;

	// The current display mode only matters at the time we're planning on
	// showing the overlay channel on screen - that's why we can't use
	// the B_OVERLAY_COUNT hook.
	// TODO: remove fAccOverlayCount if we're not going to need it at all.

	return fAccAllocateOverlay();
}


void
AccelerantHWInterface::ReleaseOverlayChannel(overlay_token token)
{
	if (token == NULL)
		return;

	fAccReleaseOverlay(token);
}


bool
AccelerantHWInterface::CheckOverlayRestrictions(int32 width, int32 height,
	color_space colorSpace)
{
	if (fAccOverlaySupportedSpaces == NULL
		|| fAccGetOverlayConstraints == NULL
		|| fAccAllocateOverlayBuffer == NULL
		|| fAccReleaseOverlayBuffer == NULL)
		return false;

	// Note: we can't really check the size of the overlay upfront - we
	// must assume fAccAllocateOverlayBuffer() will fail in that case.
	if (width < 0 || width > 65535 || height < 0 || height > 65535)
		return false;

	// check color space

	const uint32* spaces = fAccOverlaySupportedSpaces(&fDisplayMode);
	if (spaces == NULL)
		return false;

	for (int32 i = 0; spaces[i] != 0; i++) {
		if (spaces[i] == (uint32)colorSpace)
			return true;
	}

	return false;
}


const overlay_buffer*
AccelerantHWInterface::AllocateOverlayBuffer(int32 width, int32 height, color_space space)
{
	if (fAccAllocateOverlayBuffer == NULL)
		return NULL;

	return fAccAllocateOverlayBuffer(space, width, height);
}


void
AccelerantHWInterface::FreeOverlayBuffer(const overlay_buffer* buffer)
{
	if (buffer == NULL || fAccReleaseOverlayBuffer == NULL)
		return;

	fAccReleaseOverlayBuffer(buffer);
}


void
AccelerantHWInterface::ShowOverlay(Overlay* overlay)
{
	UpdateOverlay(overlay);
}


void
AccelerantHWInterface::HideOverlay(Overlay* overlay)
{
	fAccConfigureOverlay(overlay->OverlayToken(), overlay->OverlayBuffer(), NULL, NULL);
}


void
AccelerantHWInterface::UpdateOverlay(Overlay* overlay)
{
	// TODO: this only needs to be done on mode changes!
	overlay->SetColorSpace(fDisplayMode.space);

	fAccConfigureOverlay(overlay->OverlayToken(), overlay->OverlayBuffer(),
		overlay->OverlayWindow(), overlay->OverlayView());
}


// CopyRegion
void
AccelerantHWInterface::CopyRegion(const clipping_rect* sortedRectList,
								  uint32 count, int32 xOffset, int32 yOffset)
{
	if (fAccScreenBlit && fAccAcquireEngine) {
		if (fAccAcquireEngine(B_2D_ACCELERATION, 0xff, &fSyncToken, &fEngineToken) >= B_OK) {

			// make sure the blit_params cache is large enough
			if (fBlitParamsCount < count) {
				fBlitParamsCount = (count / kDefaultParamsCount + 1) * kDefaultParamsCount;
				// NOTE: realloc() could be used instead...
				blit_params* params = new (nothrow) blit_params[fBlitParamsCount];
				if (params) {
					delete[] fBlitParams;
					fBlitParams = params;
				} else {
					count = fBlitParamsCount;
				}
			}
			// convert the rects
			for (uint32 i = 0; i < count; i++) {
				fBlitParams[i].src_left = (uint16)sortedRectList[i].left;
				fBlitParams[i].src_top = (uint16)sortedRectList[i].top;

				fBlitParams[i].dest_left = (uint16)sortedRectList[i].left + xOffset;
				fBlitParams[i].dest_top = (uint16)sortedRectList[i].top + yOffset;

				// NOTE: width and height are expressed as distance, not pixel count!
				fBlitParams[i].width = (uint16)(sortedRectList[i].right - sortedRectList[i].left);
				fBlitParams[i].height = (uint16)(sortedRectList[i].bottom - sortedRectList[i].top);
			}

			// go
			fAccScreenBlit(fEngineToken, fBlitParams, count);

			// done
			if (fAccReleaseEngine)
				fAccReleaseEngine(fEngineToken, &fSyncToken);

			// sync
			if (fAccSyncToToken)
				fAccSyncToToken(&fSyncToken);
		}
	}
}

// FillRegion
void
AccelerantHWInterface::FillRegion(/*const*/ BRegion& region, const RGBColor& color,
								  bool autoSync)
{
	if (fAccFillRect && fAccAcquireEngine) {
		if (fAccAcquireEngine(B_2D_ACCELERATION, 0xff, &fSyncToken, &fEngineToken) >= B_OK) {

			// convert the region
			uint32 count;
			_RegionToRectParams(&region, &count);

			// go
			fAccFillRect(fEngineToken, _NativeColor(color), fRectParams, count);

			// done
			if (fAccReleaseEngine)
				fAccReleaseEngine(fEngineToken, &fSyncToken);

			// sync
			if (autoSync && fAccSyncToToken)
				fAccSyncToToken(&fSyncToken);
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
			_RegionToRectParams(&region, &count);

			// go
			fAccInvertRect(fEngineToken, fRectParams, count);

			// done
			if (fAccReleaseEngine)
				fAccReleaseEngine(fEngineToken, &fSyncToken);

			// sync
			if (fAccSyncToToken)
				fAccSyncToToken(&fSyncToken);
		}
	}
}

// Sync
void
AccelerantHWInterface::Sync()
{
	if (fAccSyncToToken)
		fAccSyncToToken(&fSyncToken);
}

// SetCursor
void
AccelerantHWInterface::SetCursor(ServerCursor* cursor)
{
	HWInterface::SetCursor(cursor);
//	if (WriteLock()) {
		// TODO: implement setting the hard ware cursor
		// NOTE: cursor should be always B_RGBA32
		// NOTE: The HWInterface implementation should
		// still be called, since it takes ownership of
		// the cursor.
//		WriteUnlock();
//	}
}

// SetCursorVisible
void
AccelerantHWInterface::SetCursorVisible(bool visible)
{
	HWInterface::SetCursorVisible(visible);
//	if (WriteLock()) {
		// TODO: update graphics hardware
//		WriteUnlock();
//	}
}

// MoveCursorTo
void
AccelerantHWInterface::MoveCursorTo(const float& x, const float& y)
{
	HWInterface::MoveCursorTo(x, y);
//	if (WriteLock()) {
		// TODO: update graphics hardware
//		WriteUnlock();
//	}
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

// IsDoubleBuffered
bool
AccelerantHWInterface::IsDoubleBuffered() const
{
	if (fModeList)
		return fBackBuffer != NULL;

	return HWInterface::IsDoubleBuffered();
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
										   uint32* count) const
{
	*count = region->CountRects();
	if (fRectParamsCount < *count) {
		fRectParamsCount = (*count / kDefaultParamsCount + 1) * kDefaultParamsCount;
		// NOTE: realloc() could be used instead...
		fill_rect_params* params = new (nothrow) fill_rect_params[fRectParamsCount];
		if (params) {
			delete[] fRectParams;
			fRectParams = params;
		} else {
			*count = fRectParamsCount;
		}
	}

	for (uint32 i = 0; i < *count; i++) {
		clipping_rect r = region->RectAtInt(i);
		fRectParams[i].left = (uint16)r.left;
		fRectParams[i].top = (uint16)r.top;
		fRectParams[i].right = (uint16)r.right;
		fRectParams[i].bottom = (uint16)r.bottom;
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


void
AccelerantHWInterface::_SetSystemPalette()
{
	set_indexed_colors setIndexedColors = (set_indexed_colors)fAccelerantHook(
		B_SET_INDEXED_COLORS, NULL);
	if (setIndexedColors == NULL)
		return;

	const rgb_color* palette = SystemPalette();
	uint8 colors[3 * 256];
		// the color table is an array with 3 bytes per color
	uint32 j = 0;

	for (int32 i = 0; i < 256; i++) {
		colors[j++] = palette[i].red;
		colors[j++] = palette[i].green;
		colors[j++] = palette[i].blue;
	}

	setIndexedColors(256, 0, colors, 0);
}
