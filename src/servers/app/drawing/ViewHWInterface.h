/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef VIEW_GRAPHICS_CARD_H
#define VIEW_GRAPHICS_CARD_H


#include "HWInterface.h"

class BBitmap;
class BBitmapBuffer;
class CardWindow;
class UpdateQueue;


class ViewHWInterface : public HWInterface {
 public:
								ViewHWInterface();
	virtual						~ViewHWInterface();

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

	// frame buffer access
	virtual	RenderingBuffer*	FrontBuffer() const;
	virtual	RenderingBuffer*	BackBuffer() const;
	virtual	bool				IsDoubleBuffered() const;

	virtual	status_t			Invalidate(const BRect& frame);

 private:
			BBitmapBuffer*		fBackBuffer;
			BBitmapBuffer*		fFrontBuffer;

			CardWindow*			fWindow;

			display_mode		fDisplayMode;
};

#endif // VIEW_GRAPHICS_CARD_H
