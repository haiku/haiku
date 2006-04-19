/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef D_WINDOW_HW_INTERACE_H
#define D_WINDOW_HW_INTERACE_H


#include <Accelerant.h>
#include <Region.h>
#include <String.h>

#include "HWInterface.h"

class DWindowBuffer;
class DWindow;
class UpdateQueue;


class DWindowHWInterface : public HWInterface {
 public:
								DWindowHWInterface();
	virtual						~DWindowHWInterface();

	virtual	status_t			Initialize();
	virtual	status_t			Shutdown();

	virtual	status_t			SetMode(const display_mode &mode);
	virtual	void				GetMode(display_mode *mode);

	virtual status_t			GetDeviceInfo(accelerant_device_info *info);
	virtual status_t			GetFrameBufferConfig(frame_buffer_config& config);

	virtual status_t			GetModeList(display_mode **mode_list,
											uint32 *count);
	virtual status_t			GetPixelClockLimits(display_mode *mode,
													uint32 *low,
													uint32 *high);
	virtual status_t			GetTimingConstraints(display_timing_constraints *dtc);
	virtual status_t			ProposeMode(display_mode *candidate,
											const display_mode *low,
											const display_mode *high);

	virtual sem_id				RetraceSemaphore();
	virtual status_t			WaitForRetrace(bigtime_t timeout = B_INFINITE_TIMEOUT);

	virtual status_t			SetDPMSMode(const uint32 &state);
	virtual uint32				DPMSMode();
	virtual uint32				DPMSCapabilities();

	// query for available hardware accleration and perform it
	virtual	uint32				AvailableHWAcceleration() const;

	virtual	void				CopyRegion(const clipping_rect* sortedRectList,
										   uint32 count,
										   int32 xOffset, int32 yOffset);
	virtual	void				FillRegion(/*const*/ BRegion& region,
										   const RGBColor& color,
										   bool autoSync);
	virtual	void				InvertRegion(/*const*/ BRegion& region);

	virtual	void				Sync();

	// frame buffer access
	virtual	RenderingBuffer*	FrontBuffer() const;
	virtual	RenderingBuffer*	BackBuffer() const;
	virtual	bool				IsDoubleBuffered() const;

	virtual	status_t			Invalidate(const BRect& frame);

								// DWindowHWInterface
			void				SetOffset(int32 left, int32 top);

 private:
			DWindowBuffer*		fFrontBuffer;

			DWindow*			fWindow;

			display_mode		fDisplayMode;
			int32				fXOffset;
			int32				fYOffset;

			// accelerant stuff
			int					_OpenGraphicsDevice(int deviceNumber);
			status_t			_OpenAccelerant(int device);
			status_t			_SetupDefaultHooks();
			status_t			_UpdateFrameBufferConfig();

			void				_RegionToRectParams(/*const*/ BRegion* region,
													uint32* count) const;
			uint32				_NativeColor(const RGBColor& color) const;



		int						fCardFD;
		image_id				fAccelerantImage;
		GetAccelerantHook		fAccelerantHook;
		engine_token			*fEngineToken;
		sync_token				fSyncToken;

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
		fill_rectangle			fAccFillRect;
		invert_rectangle		fAccInvertRect;
		screen_to_screen_blit	fAccScreenBlit;
		set_cursor_shape		fAccSetCursorShape;
		move_cursor				fAccMoveCursor;
		show_cursor				fAccShowCursor;					

		frame_buffer_config		fFrameBufferConfig;

		BString					fCardNameInDevFS;

mutable	fill_rect_params*		fRectParams;
mutable	uint32					fRectParamsCount;
mutable	blit_params*			fBlitParams;
mutable	uint32					fBlitParamsCount;
};

#endif // D_WINDOW_HW_INTERACE_H
