/*
 * Copyright 2005-2016, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef ACCELERANT_HW_INTERFACE_H
#define ACCELERANT_HW_INTERFACE_H


#include "HWInterface.h"

#include <AutoDeleter.h>
#include <image.h>
#include <video_overlay.h>


class AccelerantBuffer;
class RenderingBuffer;


class AccelerantHWInterface : public HWInterface {
public:
								AccelerantHWInterface();
	virtual						~AccelerantHWInterface();

	virtual	status_t			Initialize();
	virtual	status_t			Shutdown();

	virtual	status_t			SetMode(const display_mode& mode);
	virtual	void				GetMode(display_mode* mode);

	virtual status_t			GetDeviceInfo(accelerant_device_info* info);
	virtual status_t			GetFrameBufferConfig(
									frame_buffer_config& config);

	virtual status_t			GetModeList(display_mode** _modeList,
									uint32* _count);
	virtual status_t			GetPixelClockLimits(display_mode* mode,
									uint32* _low, uint32* _high);
	virtual status_t			GetTimingConstraints(display_timing_constraints*
									constraints);
	virtual status_t			ProposeMode(display_mode* candidate,
									const display_mode* low,
									const display_mode* high);
	virtual	status_t			GetPreferredMode(display_mode* mode);
	virtual status_t			GetMonitorInfo(monitor_info* info);

	virtual sem_id				RetraceSemaphore();
	virtual status_t			WaitForRetrace(
									bigtime_t timeout = B_INFINITE_TIMEOUT);

	virtual status_t			SetDPMSMode(uint32 state);
	virtual uint32				DPMSMode();
	virtual uint32				DPMSCapabilities();

	virtual status_t			SetBrightness(float);
	virtual status_t			GetBrightness(float*);

	virtual status_t			GetAccelerantPath(BString& path);
	virtual status_t			GetDriverPath(BString& path);

	// query for available hardware accleration
	virtual	uint32				AvailableHWAcceleration() const;

	// accelerated drawing
	virtual	void				CopyRegion(const clipping_rect* sortedRectList,
									uint32 count, int32 xOffset, int32 yOffset);
	virtual	void				FillRegion(/*const*/ BRegion& region,
									const rgb_color& color, bool autoSync);
	virtual	void				InvertRegion(/*const*/ BRegion& region);

	virtual	void				Sync();

	// overlay support
	virtual overlay_token		AcquireOverlayChannel();
	virtual void				ReleaseOverlayChannel(overlay_token token);

	virtual status_t			GetOverlayRestrictions(const Overlay* overlay,
									overlay_restrictions* restrictions);
	virtual bool				CheckOverlayRestrictions(int32 width,
									int32 height, color_space colorSpace);
	virtual const overlay_buffer* AllocateOverlayBuffer(int32 width,
									int32 height, color_space space);
	virtual void				FreeOverlayBuffer(const overlay_buffer* buffer);

	virtual void				ConfigureOverlay(Overlay* overlay);
	virtual void				HideOverlay(Overlay* overlay);

	// cursor handling
	virtual	void				SetCursor(ServerCursor* cursor);
	virtual	void				SetCursorVisible(bool visible);
	virtual	void				MoveCursorTo(float x, float y);

	// frame buffer access
	virtual	RenderingBuffer*	FrontBuffer() const;
	virtual	RenderingBuffer*	BackBuffer() const;
	virtual	bool				IsDoubleBuffered() const;

protected:
	virtual	void				_CopyBackToFront(/*const*/ BRegion& region);

	virtual	void				_DrawCursor(IntRect area) const;

private:
			int					_OpenGraphicsDevice(int deviceNumber);
			status_t			_OpenAccelerant(int device);
			status_t			_SetupDefaultHooks();
			void				_UpdateHooksAfterModeChange();
			status_t			_UpdateModeList();
			status_t			_UpdateFrameBufferConfig();
			void				_RegionToRectParams(/*const*/ BRegion* region,
									uint32* count) const;
			void				_CopyRegion(const clipping_rect* sortedRectList,
									uint32 count, int32 xOffset, int32 yOffset,
									bool inBackBuffer);
			uint32				_NativeColor(const rgb_color& color) const;
			status_t			_FindBestMode(const display_mode& compareMode,
									float compareAspectRatio,
									display_mode& modeFound,
									int32 *_diff = NULL) const;
			status_t			_SetFallbackMode(display_mode& mode) const;
			void				_SetSystemPalette();
			void				_SetGrayscalePalette();

private:
			int					fCardFD;
			image_id			fAccelerantImage;
			GetAccelerantHook	fAccelerantHook;
			engine_token*		fEngineToken;
			sync_token			fSyncToken;

			// required hooks - guaranteed to be valid
			acquire_engine			fAccAcquireEngine;
			release_engine			fAccReleaseEngine;
			sync_to_token			fAccSyncToToken;
			accelerant_mode_count	fAccGetModeCount;
			get_mode_list			fAccGetModeList;
			get_frame_buffer_config	fAccGetFrameBufferConfig;
			set_display_mode		fAccSetDisplayMode;
			get_display_mode		fAccGetDisplayMode;
			get_pixel_clock_limits	fAccGetPixelClockLimits;

			// optional accelerant hooks
			get_timing_constraints	fAccGetTimingConstraints;
			propose_display_mode	fAccProposeDisplayMode;
			get_preferred_display_mode fAccGetPreferredDisplayMode;
			get_monitor_info		fAccGetMonitorInfo;
			get_edid_info			fAccGetEDIDInfo;
			fill_rectangle			fAccFillRect;
			invert_rectangle		fAccInvertRect;
			screen_to_screen_blit	fAccScreenBlit;
			set_cursor_shape		fAccSetCursorShape;
			set_cursor_bitmap		fAccSetCursorBitmap;
			move_cursor				fAccMoveCursor;
			show_cursor				fAccShowCursor;

			// dpms hooks
			dpms_capabilities	fAccDPMSCapabilities;
			dpms_mode			fAccDPMSMode;
			set_dpms_mode		fAccSetDPMSMode;

			// brightness hooks
			set_brightness		fAccSetBrightness;
			get_brightness		fAccGetBrightness;

			// overlay hooks
			overlay_count				fAccOverlayCount;
			overlay_supported_spaces	fAccOverlaySupportedSpaces;
			overlay_supported_features	fAccOverlaySupportedFeatures;
			allocate_overlay_buffer		fAccAllocateOverlayBuffer;
			release_overlay_buffer		fAccReleaseOverlayBuffer;
			get_overlay_constraints		fAccGetOverlayConstraints;
			allocate_overlay			fAccAllocateOverlay;
			release_overlay				fAccReleaseOverlay;
			configure_overlay			fAccConfigureOverlay;

			frame_buffer_config	fFrameBufferConfig;
			int					fModeCount;
			display_mode*		fModeList;

			ObjectDeleter<RenderingBuffer>
								fBackBuffer;
			ObjectDeleter<AccelerantBuffer>
								fFrontBuffer;
			bool				fOffscreenBackBuffer;

			display_mode		fDisplayMode;
			bool				fInitialModeSwitch;

			sem_id				fRetraceSemaphore;

	mutable	fill_rect_params*	fRectParams;
	mutable	uint32				fRectParamsCount;
	mutable	blit_params*		fBlitParams;
	mutable	uint32				fBlitParamsCount;
};

#endif // ACCELERANT_HW_INTERFACE_H
