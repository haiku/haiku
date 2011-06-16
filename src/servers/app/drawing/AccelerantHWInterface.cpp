/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */


/*!	Accelerant based HWInterface implementation */


#include "AccelerantHWInterface.h"

#include <dirent.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <unistd.h>

#include <Accelerant.h>
#include <Cursor.h>
#include <driver_settings.h>
#include <FindDirectory.h>
#include <graphic_driver.h>
#include <image.h>
#include <String.h>

#include <edid.h>
#include <safemode_defs.h>
#include <syscalls.h>

#include "AccelerantBuffer.h"
#include "MallocBuffer.h"
#include "Overlay.h"
#include "RGBColor.h"
#include "ServerConfig.h"
#include "ServerCursor.h"
#include "ServerProtocol.h"
#include "SystemPalette.h"


using std::nothrow;


#ifdef DEBUG_DRIVER_MODULE
#	include <stdio.h>
#	define ATRACE(x) printf x
#else
#	define ATRACE(x) ;
#endif

#define USE_ACCELERATION		0
#define OFFSCREEN_BACK_BUFFER	0


const int32 kDefaultParamsCount = 64;


bool
operator==(const display_mode& a, const display_mode& b)
{
	return memcmp(&a, &b, sizeof(display_mode)) == 0;
}


bool
use_fail_safe_video_mode()
{
	char buffer[B_FILE_NAME_LENGTH];
	size_t size = sizeof(buffer);

	status_t status = _kern_get_safemode_option(
		B_SAFEMODE_FAIL_SAFE_VIDEO_MODE, buffer, &size);
	if (status == B_OK) {
		if (!strncasecmp(buffer, "true", size)
			|| !strncasecmp(buffer, "yes", size)
			|| !strncasecmp(buffer, "on", size)
			|| !strncasecmp(buffer, "enabled", size)) {
			return true;
		}
	}

	return false;
}


//	#pragma mark -


AccelerantHWInterface::AccelerantHWInterface()
	:
	HWInterface(),
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
	fOffscreenBackBuffer(false),

	fInitialModeSwitch(true),

	fRetraceSemaphore(-1),

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


AccelerantHWInterface::~AccelerantHWInterface()
{
	delete fBackBuffer;
	delete fFrontBuffer;

	delete[] fRectParams;
	delete[] fBlitParams;

	delete[] fModeList;
}


/*!	\brief Opens the first available graphics device and initializes it
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


/*!	\brief Opens a graphics device for read-write access
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

	int device = -1;
	int count = 0;
	if (!use_fail_safe_video_mode()) {
		// TODO: We do not need to avoid the "vesa" driver this way once it has
		// been ported to the new driver architecture - the special case here
		// can then be removed.
		struct dirent *entry;
		char path[PATH_MAX];
		while (count < deviceNumber && (entry = readdir(directory)) != NULL) {
			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")
				|| !strcmp(entry->d_name, "vesa"))
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
	}

	// Open VESA driver if we were not able to get a better one
	if (count < deviceNumber) {
		if (deviceNumber == 1) {
			device = open("/dev/graphics/vesa", B_READ_WRITE);
			fVGADevice = device;
				// store the device, so that we can access the planar blitter
		} else {
			close(device);
			device = B_ENTRY_NOT_FOUND;
		}
	}

	closedir(directory);

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
		B_USER_NONPACKAGED_ADDONS_DIRECTORY,
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_NONPACKAGED_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY
	};

	fAccelerantImage = -1;

	for (uint32 i = 0; i < sizeof(dirs) / sizeof(directory_which); i++) {
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
			initAccelerant = (init_accelerant)fAccelerantHook(
				B_INIT_ACCELERANT, NULL);
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
		syslog(LOG_ERR, "Accelerant %s does not export the required hooks.\n",
			signature);

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
	fAccGetModeCount
		= (accelerant_mode_count)fAccelerantHook(B_ACCELERANT_MODE_COUNT, NULL);
	fAccGetModeList = (get_mode_list)fAccelerantHook(B_GET_MODE_LIST, NULL);
	fAccGetFrameBufferConfig = (get_frame_buffer_config)fAccelerantHook(
		B_GET_FRAME_BUFFER_CONFIG, NULL);
	fAccSetDisplayMode
		= (set_display_mode)fAccelerantHook(B_SET_DISPLAY_MODE, NULL);
	fAccGetDisplayMode
		= (get_display_mode)fAccelerantHook(B_GET_DISPLAY_MODE, NULL);
	fAccGetPixelClockLimits = (get_pixel_clock_limits)fAccelerantHook(
		B_GET_PIXEL_CLOCK_LIMITS, NULL);

	if (!fAccAcquireEngine || !fAccReleaseEngine || !fAccGetFrameBufferConfig
		|| !fAccGetModeCount || !fAccGetModeList || !fAccSetDisplayMode
		|| !fAccGetDisplayMode || !fAccGetPixelClockLimits) {
		return B_ERROR;
	}

	// optional
	fAccGetTimingConstraints = (get_timing_constraints)fAccelerantHook(
		B_GET_TIMING_CONSTRAINTS, NULL);
	fAccProposeDisplayMode = (propose_display_mode)fAccelerantHook(
		B_PROPOSE_DISPLAY_MODE, NULL);
	fAccGetPreferredDisplayMode = (get_preferred_display_mode)fAccelerantHook(
		B_GET_PREFERRED_DISPLAY_MODE, NULL);
	fAccGetMonitorInfo
		= (get_monitor_info)fAccelerantHook(B_GET_MONITOR_INFO, NULL);
	fAccGetEDIDInfo = (get_edid_info)fAccelerantHook(B_GET_EDID_INFO, NULL);

	// cursor
	fAccSetCursorShape
		= (set_cursor_shape)fAccelerantHook(B_SET_CURSOR_SHAPE, NULL);
	fAccMoveCursor = (move_cursor)fAccelerantHook(B_MOVE_CURSOR, NULL);
	fAccShowCursor = (show_cursor)fAccelerantHook(B_SHOW_CURSOR, NULL);

	// dpms
	fAccDPMSCapabilities
		= (dpms_capabilities)fAccelerantHook(B_DPMS_CAPABILITIES, NULL);
	fAccDPMSMode = (dpms_mode)fAccelerantHook(B_DPMS_MODE, NULL);
	fAccSetDPMSMode = (set_dpms_mode)fAccelerantHook(B_SET_DPMS_MODE, NULL);

	// overlay
	fAccOverlayCount = (overlay_count)fAccelerantHook(B_OVERLAY_COUNT, NULL);
	fAccOverlaySupportedSpaces = (overlay_supported_spaces)fAccelerantHook(
		B_OVERLAY_SUPPORTED_SPACES, NULL);
	fAccOverlaySupportedFeatures = (overlay_supported_features)fAccelerantHook(
		B_OVERLAY_SUPPORTED_FEATURES, NULL);
	fAccAllocateOverlayBuffer = (allocate_overlay_buffer)fAccelerantHook(
		B_ALLOCATE_OVERLAY_BUFFER, NULL);
	fAccReleaseOverlayBuffer = (release_overlay_buffer)fAccelerantHook(
		B_RELEASE_OVERLAY_BUFFER, NULL);
	fAccGetOverlayConstraints = (get_overlay_constraints)fAccelerantHook(
		B_GET_OVERLAY_CONSTRAINTS, NULL);
	fAccAllocateOverlay
		= (allocate_overlay)fAccelerantHook(B_ALLOCATE_OVERLAY, NULL);
	fAccReleaseOverlay
		= (release_overlay)fAccelerantHook(B_RELEASE_OVERLAY, NULL);
	fAccConfigureOverlay
		= (configure_overlay)fAccelerantHook(B_CONFIGURE_OVERLAY, NULL);

	return B_OK;
}


status_t
AccelerantHWInterface::Shutdown()
{
	if (fAccelerantHook != NULL) {
		uninit_accelerant uninitAccelerant
			= (uninit_accelerant)fAccelerantHook(B_UNINIT_ACCELERANT, NULL);
		if (uninitAccelerant != NULL)
			uninitAccelerant();

		fAccelerantHook = NULL;
	}

	if (fAccelerantImage >= 0) {
		unload_add_on(fAccelerantImage);
		fAccelerantImage = -1;
	}

	if (fCardFD >= 0) {
		close(fCardFD);
		fCardFD = -1;
	}

	return B_OK;
}


/*! Finds the mode in the mode list that is closest to the mode specified.
	As long as the mode list is not empty, this method will always succeed.
*/
status_t
AccelerantHWInterface::_FindBestMode(const display_mode& compareMode,
	float compareAspectRatio, display_mode& modeFound, int32 *_diff) const
{
	int32 bestDiff = 0;
	int32 bestIndex = -1;
	for (int32 i = 0; i < fModeCount; i++) {
		display_mode& mode = fModeList[i];
		float aspectRatio = 0;

		if (compareAspectRatio != 0 && mode.timing.v_display != 0)
			aspectRatio = mode.timing.h_display / mode.timing.v_display;

		// compute some random equality score
		// TODO: check if these scores make sense
		int32 diff
			= 1000 * abs(mode.timing.h_display - compareMode.timing.h_display)
			+ 1000 * abs(mode.timing.v_display - compareMode.timing.v_display)
			+ abs(mode.timing.h_total * mode.timing.v_total
					- compareMode.timing.h_total * compareMode.timing.v_total)
				/ 100
			+ abs(mode.timing.pixel_clock - compareMode.timing.pixel_clock)
				/ 100
			+ (int32)(500 * fabs(aspectRatio - compareAspectRatio))
			+ 100 * abs(mode.space - compareMode.space);

		if (bestIndex == -1 || diff < bestDiff) {
			bestDiff = diff;
			bestIndex = i;
		}
	}

	if (bestIndex < 0)
		return B_ERROR;

	modeFound = fModeList[bestIndex];
	if (_diff != 0)
		*_diff = bestDiff;

	return B_OK;
}


/*!	This method is used for the initial mode set only - because that one
	should really not fail.
	Basically we try to set all modes as found in the mode list the driver
	returned, but we start with the one that best fits the originally
	desired mode.
	The mode list must have been retrieved already.
*/
status_t
AccelerantHWInterface::_SetFallbackMode(display_mode& newMode) const
{
	// At first, we search the closest display mode from the list of
	// supported modes - if that fails, we just take one

	if (_FindBestMode(newMode, 0, newMode) == B_OK
		&& fAccSetDisplayMode(&newMode) == B_OK)
		return B_OK;

	// That failed as well, this looks like a bug in the graphics
	// driver, but we have to try to be as forgiving as possible
	// here - just take the first mode that works!

	for (int32 i = 0; i < fModeCount; i++) {
		newMode = fModeList[i];
		if (fAccSetDisplayMode(&newMode) == B_OK)
			return B_OK;
	}

	// Well, we tried.
	return B_ERROR;
}


status_t
AccelerantHWInterface::SetMode(const display_mode& mode)
{
	AutoWriteLocker _(this);
	// TODO: There are places this function can fail,
	// maybe it needs to roll back changes in case of an
	// error.

	// prevent from doing the unnecessary
	if (fModeCount > 0 && fFrontBuffer && fDisplayMode == mode) {
		// TODO: better comparison of display modes
		return B_OK;
	}

	// some safety checks
	// TODO: more of those!
	if (!_IsValidMode(mode))
		return B_BAD_VALUE;

	if (fFrontBuffer == NULL)
		return B_NO_INIT;

	// just try to set the mode - we let the graphics driver
	// approve or deny the request, as it should know best

	display_mode newMode = mode;

	bool tryOffscreenBackBuffer = false;
	fOffscreenBackBuffer = false;
#if USE_ACCELERATION && OFFSCREEN_BACK_BUFFER
	if (fVGADevice < 0 && (color_space)newMode.space == B_RGB32) {
		// we should have an accelerated graphics driver, try
		// to allocate a frame buffer large enough to contain
		// the back buffer for double buffered drawing
		newMode.virtual_height *= 2;
		tryOffscreenBackBuffer = true;
	}
#endif

	status_t status = B_ERROR;
	if (!use_fail_safe_video_mode() || !fInitialModeSwitch)
		status = fAccSetDisplayMode(&newMode);
	if (status != B_OK) {
		ATRACE(("setting display mode failed\n"));
		if (!fInitialModeSwitch)
			return status;

		// undo the offscreen backbuffer trick when trying the various
		// fall back methods for the initial mode switch
		// TODO: Do it even then, but it is more involved.
		if (tryOffscreenBackBuffer) {
			newMode.virtual_height /= 2;
			tryOffscreenBackBuffer = false;
		}

		if (fModeList == NULL) {
			status = _UpdateModeList();
			if (status != B_OK)
				return status;
		}

		// If this is the initial mode switch, we try a number of fallback
		// modes first, before we have to fail

		status = use_fail_safe_video_mode()
			? B_ERROR : _SetFallbackMode(newMode);
		if (status != B_OK) {
			// The driver doesn't allow us the mode switch - this usually
			// means we have a driver that doesn't allow mode switches at
			// all.
			// All we can do now is to ask the driver which mode we can
			// use - this is always necessary for VESA mode, for example.
			if (fAccGetDisplayMode(&newMode) != B_OK)
				return B_ERROR;

			// TODO: check if the mode returned is valid!
			if (!_IsValidMode(newMode))
				return B_BAD_DATA;

			// TODO: if the mode switch before fails as well, we must forbid
			//	any uses of this class!
			status = B_OK;
		}
	}

	if (tryOffscreenBackBuffer) {
		// The offscreen backbuffer was successfully allocated, since
		// the mode switch succeeded! This should be handled transparently
		// though and not be reflected in the mode structure, so that
		// even real virtual screens should work eventually...
		newMode.virtual_height /= 2;
		fOffscreenBackBuffer = true;
	}

	fDisplayMode = newMode;
	fInitialModeSwitch = false;

	// update frontbuffer
	fFrontBuffer->SetDisplayMode(fDisplayMode);
	if (_UpdateFrameBufferConfig() != B_OK) {
		// TODO: if this fails, we're basically toasted - we need to handle this
		//	differently to crashing later on!
		return B_ERROR;
	}

	// Update the frame buffer used by the on-screen KDL
#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
	uint32 depth = (fFrameBufferConfig.bytes_per_row
		/ fFrontBuffer->Width()) << 3;
	if (fDisplayMode.space == B_RGB15)
		depth = 15;

	_kern_frame_buffer_update((addr_t)fFrameBufferConfig.frame_buffer,
		fFrontBuffer->Width(), fFrontBuffer->Height(),
		depth, fFrameBufferConfig.bytes_per_row);
#endif

	// update acceleration hooks
#if USE_ACCELERATION
	fAccFillRect = (fill_rectangle)fAccelerantHook(B_FILL_RECTANGLE,
		(void *)&fDisplayMode);
	fAccInvertRect = (invert_rectangle)fAccelerantHook(B_INVERT_RECTANGLE,
		(void *)&fDisplayMode);
	fAccScreenBlit = (screen_to_screen_blit)fAccelerantHook(
		B_SCREEN_TO_SCREEN_BLIT, (void *)&fDisplayMode);
#else
	fAccFillRect = NULL;
	fAccInvertRect = NULL;
	fAccScreenBlit = NULL;
#endif

	// in case there is no accelerated blit function, using
	// an offscreen located backbuffer will not be beneficial!
	if (fAccScreenBlit == NULL)
		fOffscreenBackBuffer = false;

	// update backbuffer if neccessary
	if (!fBackBuffer || fBackBuffer->Width() != fFrontBuffer->Width()
		|| fBackBuffer->Height() != fFrontBuffer->Height()
		|| fOffscreenBackBuffer
		|| (fFrontBuffer->ColorSpace() == B_RGB32 && fBackBuffer != NULL
			&& !HWInterface::IsDoubleBuffered())) {
		// NOTE: backbuffer is always B_RGBA32, this simplifies the
		// drawing backend implementation tremendously for the time
		// being. The color space conversion is handled in CopyBackToFront()

		delete fBackBuffer;
		fBackBuffer = NULL;

		// TODO: Above not true anymore for single buffered mode!!!
		// -> fall back to double buffer for fDisplayMode.space != B_RGB32
		// as intermediate solution...
		bool doubleBuffered = HWInterface::IsDoubleBuffered();
		if ((fFrontBuffer->ColorSpace() != B_RGB32
			&& fFrontBuffer->ColorSpace() != B_RGBA32)
			|| fVGADevice >= 0 || fOffscreenBackBuffer)
			doubleBuffered = true;
#if !USE_ACCELERATION
		doubleBuffered = true;
#endif

		if (doubleBuffered) {
			if (fOffscreenBackBuffer) {
				fBackBuffer = new(nothrow) AccelerantBuffer(*fFrontBuffer,
					true);
			} else {
				fBackBuffer = new(nothrow) MallocBuffer(fFrontBuffer->Width(),
					fFrontBuffer->Height());
			}

			status = fBackBuffer ? fBackBuffer->InitCheck() : B_NO_MEMORY;
			if (status < B_OK) {
				delete fBackBuffer;
				fBackBuffer = NULL;
				fOffscreenBackBuffer = false;
				return status;
			}
			// clear out backbuffer, alpha is 255 this way
			memset(fBackBuffer->Bits(), 255, fBackBuffer->BitsLength());
		}
#if 0
// NOTE: Currently disabled, because it make the double buffered mode flicker
// again. See HWInterface::Invalidate() for more information.
		SetAsyncDoubleBuffered(doubleBuffered);
#endif
	}

	// update color palette configuration if necessary
	if (fDisplayMode.space == B_CMAP8)
		_SetSystemPalette();
	else if (fDisplayMode.space == B_GRAY8)
		_SetGrayscalePalette();

	// notify all listeners about the mode change
	_NotifyFrameBufferChanged();

	return status;
}


void
AccelerantHWInterface::GetMode(display_mode* mode)
{
	if (mode && LockParallelAccess()) {
		*mode = fDisplayMode;
		UnlockParallelAccess();
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
AccelerantHWInterface::GetDeviceInfo(accelerant_device_info* info)
{
	get_accelerant_device_info GetAccelerantDeviceInfo
		= (get_accelerant_device_info)fAccelerantHook(
			B_GET_ACCELERANT_DEVICE_INFO, NULL);
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
AccelerantHWInterface::GetModeList(display_mode** _modes, uint32* _count)
{
	AutoReadLocker _(this);

	if (_count == NULL || _modes == NULL)
		return B_BAD_VALUE;

	status_t status = B_OK;

	if (fModeList == NULL)
		status = _UpdateModeList();

	if (status >= B_OK) {
		*_modes = new(nothrow) display_mode[fModeCount];
		if (*_modes) {
			*_count = fModeCount;
			memcpy(*_modes, fModeList, sizeof(display_mode) * fModeCount);
		} else {
			*_count = 0;
			status = B_NO_MEMORY;
		}
	}
	return status;
}


status_t
AccelerantHWInterface::GetPixelClockLimits(display_mode *mode, uint32* _low,
	uint32* _high)
{
	if (mode == NULL || _low == NULL || _high == NULL)
		return B_BAD_VALUE;

	AutoReadLocker _(this);
	return fAccGetPixelClockLimits(mode, _low, _high);
}


status_t
AccelerantHWInterface::GetTimingConstraints(
	display_timing_constraints* constraints)
{
	if (constraints == NULL)
		return B_BAD_VALUE;

	AutoReadLocker _(this);

	if (fAccGetTimingConstraints)
		return fAccGetTimingConstraints(constraints);

	return B_UNSUPPORTED;
}


status_t
AccelerantHWInterface::ProposeMode(display_mode* candidate,
	const display_mode* _low, const display_mode* _high)
{
	if (candidate == NULL || _low == NULL || _high == NULL)
		return B_BAD_VALUE;

	AutoReadLocker _(this);

	if (fAccProposeDisplayMode == NULL)
		return B_UNSUPPORTED;

	// avoid const issues
	display_mode high, low;
	high = *_high;
	low = *_low;

	return fAccProposeDisplayMode(candidate, &low, &high);
}


status_t
AccelerantHWInterface::GetPreferredMode(display_mode* preferredMode)
{
	status_t status = B_NOT_SUPPORTED;

	if (fAccGetPreferredDisplayMode != NULL) {
		status = fAccGetPreferredDisplayMode(preferredMode);
		if (status == B_OK)
			return B_OK;
	}

	if (fAccGetEDIDInfo != NULL) {
		edid1_info info;
		uint32 version;
		status = fAccGetEDIDInfo(&info, sizeof(info), &version);
		if (status < B_OK)
			return status;
		if (version != EDID_VERSION_1)
			return B_NOT_SUPPORTED;

		if (fModeList == NULL) {
			status = _UpdateModeList();
			if (status != B_OK)
				return status;
		}

		status = B_NOT_SUPPORTED;
		display_mode bestMode;
		int32 bestDiff = INT_MAX;

		// find preferred mode from EDID info
		for (uint32 i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i) {
			if (info.detailed_monitor[i].monitor_desc_type
					!= EDID1_IS_DETAILED_TIMING)
				continue;

			// construct basic mode and find it in the mode list
			const edid1_detailed_timing& timing
				= info.detailed_monitor[i].data.detailed_timing;
			if (timing.h_active < 640 || timing.v_active < 350)
				continue;

			float aspectRatio = 0.0f;
			if (timing.h_size > 0 && timing.v_size > 0)
				aspectRatio = 1.0f * timing.h_size / timing.v_size;

			display_mode modeFound;
			display_mode mode;

			mode.timing.pixel_clock = timing.pixel_clock * 10;
			mode.timing.h_display = timing.h_active;
			mode.timing.h_sync_start = timing.h_active + timing.h_sync_off;
			mode.timing.h_sync_end = mode.timing.h_sync_start
				+ timing.h_sync_width;
			mode.timing.h_total = timing.h_active + timing.h_blank;
			mode.timing.v_display = timing.v_active;
			mode.timing.v_sync_start = timing.v_active + timing.v_sync_off;
			mode.timing.v_sync_end = mode.timing.v_sync_start
				+ timing.v_sync_width;
			mode.timing.v_total = timing.v_active + timing.v_blank;

			mode.space = B_RGB32;
			mode.virtual_width = mode.timing.h_display;
			mode.virtual_height = mode.timing.v_display;

			// TODO: eventually ignore detailed modes for the preferred one
			// if there are more than one usable?
			int32 diff;
			if (_FindBestMode(mode, aspectRatio, modeFound, &diff) == B_OK) {
				status = B_OK;
				if (diff < bestDiff) {
					bestMode = modeFound;
					bestDiff = diff;
				}
			}
		}

		if (status == B_OK)
			*preferredMode = bestMode;
	}

	return status;
}


status_t
AccelerantHWInterface::GetMonitorInfo(monitor_info* info)
{
	status_t status = B_NOT_SUPPORTED;

	if (fAccGetMonitorInfo != NULL) {
		status = fAccGetMonitorInfo(info);
		if (status == B_OK)
			return B_OK;
	}

	if (fAccGetEDIDInfo == NULL)
		return status;

	edid1_info edid;
	uint32 version;
	status = fAccGetEDIDInfo(&edid, sizeof(edid), &version);
	if (status < B_OK)
		return status;
	if (version != EDID_VERSION_1)
		return B_NOT_SUPPORTED;

	memset(info, 0, sizeof(monitor_info));
	strlcpy(info->vendor, edid.vendor.manufacturer, sizeof(info->vendor));
	if (edid.vendor.serial != 0) {
		snprintf(info->serial_number, sizeof(info->serial_number), "%lu",
			edid.vendor.serial);
	}
	info->product_id = edid.vendor.prod_id;
	info->produced.week = edid.vendor.week;
	info->produced.year = edid.vendor.year;
	info->width = edid.display.h_size;
	info->height = edid.display.v_size;

	for (uint32 i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i) {
		edid1_detailed_monitor *monitor = &edid.detailed_monitor[i];

		switch (monitor->monitor_desc_type) {
			case EDID1_SERIAL_NUMBER:
				strlcpy(info->serial_number, monitor->data.serial_number,
					sizeof(info->serial_number));
				break;

			case EDID1_MONITOR_NAME:
				// There can be several of these; in this case we'll just
				// overwrite the previous entries
				// TODO: we could append them as well
				strlcpy(info->name, monitor->data.monitor_name,
					sizeof(info->name));
				break;

			case EDID1_MONITOR_RANGES:
			{
				edid1_monitor_range& range = monitor->data.monitor_range;

				info->min_horizontal_frequency = range.min_h;
				info->max_horizontal_frequency = range.max_h;
				info->min_vertical_frequency = range.min_v;
				info->max_vertical_frequency = range.max_v;
				info->max_pixel_clock = range.max_clock * 10000;
				break;
			}

			case EDID1_IS_DETAILED_TIMING:
			{
				edid1_detailed_timing& timing = monitor->data.detailed_timing;
				info->width = timing.h_size / 10.0;
				info->height = timing.v_size / 10.0;
			}

			default:
				break;
		}
	}

	return B_OK;
}


sem_id
AccelerantHWInterface::RetraceSemaphore()
{
	AutoWriteLocker _(this);

	if (fRetraceSemaphore != -1)
		return fRetraceSemaphore;

	accelerant_retrace_semaphore AccelerantRetraceSemaphore =
		(accelerant_retrace_semaphore)fAccelerantHook(
			B_ACCELERANT_RETRACE_SEMAPHORE, NULL);
	if (!AccelerantRetraceSemaphore)
		fRetraceSemaphore = B_UNSUPPORTED;
	else
		fRetraceSemaphore = AccelerantRetraceSemaphore();

	return fRetraceSemaphore;
}


status_t
AccelerantHWInterface::WaitForRetrace(bigtime_t timeout)
{
	sem_id sem = RetraceSemaphore();
	if (sem < 0)
		return sem;

	return acquire_sem_etc(sem, 1, B_RELATIVE_TIMEOUT, timeout);
}


status_t
AccelerantHWInterface::SetDPMSMode(uint32 state)
{
	AutoWriteLocker _(this);

	if (!fAccSetDPMSMode)
		return B_UNSUPPORTED;

	return fAccSetDPMSMode(state);
}


uint32
AccelerantHWInterface::DPMSMode()
{
	AutoReadLocker _(this);

	if (!fAccDPMSMode)
		return B_UNSUPPORTED;

	return fAccDPMSMode();
}


uint32
AccelerantHWInterface::DPMSCapabilities()
{
	AutoReadLocker _(this);

	if (!fAccDPMSCapabilities)
		return B_UNSUPPORTED;

	return fAccDPMSCapabilities();
}


status_t
AccelerantHWInterface::GetAccelerantPath(BString& string)
{
	image_info info;
	status_t status = get_image_info(fAccelerantImage, &info);
	if (status == B_OK)
		string = info.name;
	return status;
}


status_t
AccelerantHWInterface::GetDriverPath(BString& string)
{
	// TODO: this currently assumes that the accelerant's clone info
	//	is always the path name of its driver (that's the case for
	//	all of our drivers)
	char path[B_PATH_NAME_LENGTH];
	get_accelerant_clone_info getCloneInfo;
	getCloneInfo = (get_accelerant_clone_info)fAccelerantHook(
		B_GET_ACCELERANT_CLONE_INFO, NULL);

	if (getCloneInfo == NULL)
		return B_NOT_SUPPORTED;

	getCloneInfo((void*)path);
	string.SetTo(path);
	return B_OK;
}


// #pragma mark - acceleration


uint32
AccelerantHWInterface::AvailableHWAcceleration() const
{
	uint32 flags = 0;

	if (!IsDoubleBuffered() || fOffscreenBackBuffer) {
		if (fAccScreenBlit)
			flags |= HW_ACC_COPY_REGION;
		if (fAccFillRect)
			flags |= HW_ACC_FILL_REGION;
		if (fAccInvertRect)
			flags |= HW_ACC_INVERT_REGION;
	}

	return flags;
}


void
AccelerantHWInterface::CopyRegion(const clipping_rect* sortedRectList,
	uint32 count, int32 xOffset, int32 yOffset)
{
	_CopyRegion(sortedRectList, count, xOffset, yOffset, fOffscreenBackBuffer);
}


void
AccelerantHWInterface::FillRegion(/*const*/ BRegion& region,
	const rgb_color& color, bool autoSync)
{
	if (fAccFillRect && fAccAcquireEngine) {
		if (fAccAcquireEngine(B_2D_ACCELERATION, 0xff, &fSyncToken,
				&fEngineToken) >= B_OK) {
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


void
AccelerantHWInterface::InvertRegion(/*const*/ BRegion& region)
{
	if (fAccInvertRect && fAccAcquireEngine) {
		if (fAccAcquireEngine(B_2D_ACCELERATION, 0xff, &fSyncToken,
				&fEngineToken) >= B_OK) {
			// convert the region
			uint32 count;
			_RegionToRectParams(&region, &count);

			fAccInvertRect(fEngineToken, fRectParams, count);

			if (fAccReleaseEngine)
				fAccReleaseEngine(fEngineToken, &fSyncToken);
			if (fAccSyncToToken)
				fAccSyncToToken(&fSyncToken);
		}
	}
}


void
AccelerantHWInterface::Sync()
{
	if (fAccSyncToToken)
		fAccSyncToToken(&fSyncToken);
}


// #pragma mark - overlays


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


status_t
AccelerantHWInterface::GetOverlayRestrictions(const Overlay* overlay,
	overlay_restrictions* restrictions)
{
	if (overlay == NULL || restrictions == NULL)
		return B_BAD_VALUE;
	if (fAccGetOverlayConstraints == NULL)
		return B_NOT_SUPPORTED;

	overlay_constraints constraints;
	status_t status = fAccGetOverlayConstraints(&fDisplayMode,
		overlay->OverlayBuffer(), &constraints);
	if (status < B_OK)
		return status;

	memset(restrictions, 0, sizeof(overlay_restrictions));
	memcpy(&restrictions->source, &constraints.view, sizeof(overlay_limits));
	memcpy(&restrictions->destination, &constraints.window,
		sizeof(overlay_limits));
	restrictions->min_width_scale = constraints.h_scale.min;
	restrictions->max_width_scale = constraints.h_scale.max;
	restrictions->min_height_scale = constraints.v_scale.min;
	restrictions->max_height_scale = constraints.v_scale.max;

	return B_OK;
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
AccelerantHWInterface::AllocateOverlayBuffer(int32 width, int32 height,
	color_space space)
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
AccelerantHWInterface::ConfigureOverlay(Overlay* overlay)
{
	// TODO: this only needs to be done on mode changes!
	overlay->SetColorSpace(fDisplayMode.space);

	fAccConfigureOverlay(overlay->OverlayToken(), overlay->OverlayBuffer(),
		overlay->OverlayWindow(), overlay->OverlayView());
}


void
AccelerantHWInterface::HideOverlay(Overlay* overlay)
{
	fAccConfigureOverlay(overlay->OverlayToken(), overlay->OverlayBuffer(),
		NULL, NULL);
}


// #pragma mark - cursor



void
AccelerantHWInterface::SetCursor(ServerCursor* cursor)
{
	HWInterface::SetCursor(cursor);
//	if (LockExclusiveAccess()) {
		// TODO: implement setting the hard ware cursor
		// NOTE: cursor should be always B_RGBA32
		// NOTE: The HWInterface implementation should
		// still be called, since it takes ownership of
		// the cursor.
//		UnlockExclusiveAccess();
//	}
}


void
AccelerantHWInterface::SetCursorVisible(bool visible)
{
	HWInterface::SetCursorVisible(visible);
//	if (LockExclusiveAccess()) {
		// TODO: update graphics hardware
//		UnlockExclusiveAccess();
//	}
}


void
AccelerantHWInterface::MoveCursorTo(float x, float y)
{
	HWInterface::MoveCursorTo(x, y);
//	if (LockExclusiveAccess()) {
		// TODO: update graphics hardware
//		UnlockExclusiveAccess();
//	}
}


// #pragma mark - buffer access


RenderingBuffer*
AccelerantHWInterface::FrontBuffer() const
{
	return fFrontBuffer;
}


RenderingBuffer*
AccelerantHWInterface::BackBuffer() const
{
	return fBackBuffer;
}


bool
AccelerantHWInterface::IsDoubleBuffered() const
{
	return fBackBuffer != NULL;
}


void
AccelerantHWInterface::_CopyBackToFront(/*const*/ BRegion& region)
{
	if (fOffscreenBackBuffer) {
		int32 xOffset = 0;
		int32 yOffset = -(int32)fFrontBuffer->Height();

		int32 count = region.CountRects();
		clipping_rect rects[count];
		for (int32 i = 0; i < count; i++) {
			rects[i] = region.RectAtInt(i);
			rects[i].top -= yOffset;
			rects[i].bottom -= yOffset;
		}

		_CopyRegion(rects, count, xOffset, yOffset, false);

		return;
	}

	return HWInterface::_CopyBackToFront(region);
}


// #pragma mark -


void
AccelerantHWInterface::_DrawCursor(IntRect area) const
{
	// use the default implementation for now,
	// until we have a hardware cursor
	HWInterface::_DrawCursor(area);
	// TODO: this would only be called, if we don't have
	// a hardware cursor for some reason
}


void
AccelerantHWInterface::_RegionToRectParams(/*const*/ BRegion* region,
	uint32* count) const
{
	*count = region->CountRects();
	// TODO: locking!!
	if (fRectParamsCount < *count) {
		fRectParamsCount = (*count / kDefaultParamsCount + 1)
			* kDefaultParamsCount;
		// NOTE: realloc() could be used instead...
		fill_rect_params* params
			= new (nothrow) fill_rect_params[fRectParamsCount];
		if (params) {
			delete[] fRectParams;
			fRectParams = params;
		} else {
			*count = fRectParamsCount;
		}
	}

	int32 srcOffsetY = fOffscreenBackBuffer ? fFrontBuffer->Height() : 0;

	for (uint32 i = 0; i < *count; i++) {
		clipping_rect r = region->RectAtInt(i);
		fRectParams[i].left = (uint16)r.left;
		fRectParams[i].top = (uint16)r.top + srcOffsetY;
		fRectParams[i].right = (uint16)r.right;
		fRectParams[i].bottom = (uint16)r.bottom + srcOffsetY;
	}
}


void
AccelerantHWInterface::_CopyRegion(const clipping_rect* sortedRectList,
	uint32 count, int32 xOffset, int32 yOffset, bool inBackBuffer)
{
	if (fAccScreenBlit && fAccAcquireEngine) {
		if (fAccAcquireEngine(B_2D_ACCELERATION, 0xff, &fSyncToken,
				&fEngineToken) >= B_OK) {
			// make sure the blit_params cache is large enough
			// TODO: locking!!
			if (fBlitParamsCount < count) {
				fBlitParamsCount = (count / kDefaultParamsCount + 1)
					* kDefaultParamsCount;
				// NOTE: realloc() could be used instead...
				blit_params* params
					= new (nothrow) blit_params[fBlitParamsCount];
				if (params) {
					delete[] fBlitParams;
					fBlitParams = params;
				} else {
					count = fBlitParamsCount;
				}
			}
			int32 srcOffsetY = inBackBuffer ? fFrontBuffer->Height() : 0;
			// convert the rects
			for (uint32 i = 0; i < count; i++) {
				fBlitParams[i].src_left = (uint16)sortedRectList[i].left;
				fBlitParams[i].src_top = (uint16)sortedRectList[i].top
					+ srcOffsetY;

				fBlitParams[i].dest_left = (uint16)sortedRectList[i].left
					+ xOffset;
				fBlitParams[i].dest_top = (uint16)sortedRectList[i].top
					+ yOffset + srcOffsetY;

				// NOTE: width and height are expressed as distance, not
				// pixel count!
				fBlitParams[i].width = (uint16)(sortedRectList[i].right
					- sortedRectList[i].left);
				fBlitParams[i].height = (uint16)(sortedRectList[i].bottom
					- sortedRectList[i].top);
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


uint32
AccelerantHWInterface::_NativeColor(const rgb_color& color) const
{
	// NOTE: This functions looks somehow suspicios to me.
	// It assumes that all graphics cards have the same native endianess, no?
	switch (fDisplayMode.space) {
		case B_CMAP8:
		case B_GRAY8:
			return RGBColor(color).GetColor8();

		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			return RGBColor(color).GetColor15();

		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			return RGBColor(color).GetColor16();

		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE: {
			return (uint32)((color.alpha << 24) | (color.red << 16)
				| (color.green << 8) | color.blue);
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


void
AccelerantHWInterface::_SetGrayscalePalette()
{
	set_indexed_colors setIndexedColors = (set_indexed_colors)fAccelerantHook(
		B_SET_INDEXED_COLORS, NULL);
	if (setIndexedColors == NULL)
		return;

	uint8 colors[3 * 256];
		// the color table is an array with 3 bytes per color
	uint32 j = 0;

	if (fFrontBuffer->Width() > fFrontBuffer->BytesPerRow()) {
		// VGA 16 color grayscale planar mode
		for (int32 i = 0; i < 256; i++) {
			colors[j++] = (i & 0xf) * 17;
			colors[j++] = (i & 0xf) * 17;
			colors[j++] = (i & 0xf) * 17;
		}

		setIndexedColors(256, 0, colors, 0);
	} else {
		for (int32 i = 0; i < 256; i++) {
			colors[j++] = i;
			colors[j++] = i;
			colors[j++] = i;
		}

		setIndexedColors(256, 0, colors, 0);
	}
}
