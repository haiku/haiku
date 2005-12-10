/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef ACCELERANT_HW_INTERFACE_H
#define ACCELERANT_HW_INTERFACE_H


#include <image.h>

#include <Accelerant.h>
#include <Locker.h>
#include <Region.h>
#include <String.h>

//class MallocBuffer;
//class AccelerantBuffer;


class AccelerantHWInterface : public BLocker/* : public HWInterface*/ {
public:
								AccelerantHWInterface();
	virtual						~AccelerantHWInterface();

	virtual	status_t			Initialize();
	virtual	status_t			Shutdown();

/*	virtual	status_t			SetMode(const display_mode &mode);
	virtual	void				GetMode(display_mode *mode);

	virtual status_t			GetDeviceInfo(accelerant_device_info *info);
	virtual status_t			GetFrameBufferConfig(frame_buffer_config& config);

	virtual status_t			GetModeList(display_mode **mode_list,
									uint32 *count);
	virtual status_t			GetPixelClockLimits(display_mode *mode,
									uint32 *low, uint32 *high);
	virtual status_t			GetTimingConstraints(display_timing_constraints *dtc);
	virtual status_t			ProposeMode(display_mode *candidate,
									const display_mode *low,
									const display_mode *high);

	virtual sem_id				RetraceSemaphore();
	virtual status_t			WaitForRetrace(bigtime_t timeout = B_INFINITE_TIMEOUT);

	virtual status_t			SetDPMSMode(const uint32 &state);
	virtual uint32				DPMSMode();
	virtual uint32				DPMSCapabilities();*/

	// query for available hardware accleration and perform it
	virtual	uint32				AvailableHWAcceleration() const;

	virtual	void				CopyRegion(const clipping_rect* sortedRectList,
										   uint32 count,
										   int32 xOffset, int32 yOffset);
	virtual	void				FillRegion(/*const*/ BRegion& region,
										   const rgb_color& color);
	virtual	void				InvertRegion(/*const*/ BRegion& region);

	// cursor handling
/*virtual	void					SetCursor(ServerCursor* cursor);
virtual	void					SetCursorVisible(bool visible);
virtual	void					MoveCursorTo(const float& x,
											 const float& y);*/

	// frame buffer access
//virtual	RenderingBuffer*		FrontBuffer() const;
//virtual	RenderingBuffer*		BackBuffer() const;
//virtual	bool					IsDoubleBuffered() const;

protected:
//virtual	void					_DrawCursor(BRect area) const;

private:
		int						_OpenGraphicsDevice(int deviceNumber);
		status_t				_OpenAccelerant(int device);
		status_t				_SetupDefaultHooks();
		status_t				_UpdateModeList();
		status_t				_UpdateFrameBufferConfig();
		void					_RegionToRectParams(/*const*/ BRegion* region,
													fill_rect_params** params,
													uint32* count) const;
		uint32					_NativeColor(const rgb_color& color) const;

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

		BString					fCardNameInDevFS;
		// dpms hooks
/*		dpms_capabilities		fAccDPMSCapabilities;
		dpms_mode				fAccDPMSMode;
		set_dpms_mode			fAccSetDPMSMode;

		frame_buffer_config		fFrameBufferConfig;
		int						fModeCount;
		display_mode			*fModeList;*/


//		MallocBuffer			*fBackBuffer;
//		AccelerantBuffer		*fFrontBuffer;


//		display_mode			fDisplayMode;
};

#endif // ACCELERANT_HW_INTERFACE_H
