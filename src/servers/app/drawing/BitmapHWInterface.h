//------------------------------------------------------------------------------
// Copyright 2005, Haiku, Inc. All rights reserved.
// Distributed under the terms of the MIT License.
//
//	Author:			Stephan Aßmus, <superstippi@gmx.de>
//------------------------------------------------------------------------------

#ifndef BITMAP_HW_INTERFACE_H
#define BITMAP_HW_INTERFACE_H

#include "HWInterface.h"

class BitmapBuffer;
class MallocBuffer;
class ServerBitmap;
class BBitmapBuffer;

class BitmapHWInterface : public HWInterface {
 public:
								BitmapHWInterface(ServerBitmap* bitmap);
virtual							~BitmapHWInterface();

	virtual	status_t			Initialize();
	virtual	status_t			Shutdown();

	// overwrite all the meaningless functions with empty code
	virtual	status_t			SetMode(const display_mode &mode);
	virtual	void				GetMode(display_mode *mode);
	
	virtual status_t			GetDeviceInfo(accelerant_device_info *info);
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

private:
		BBitmapBuffer*			fBackBuffer;
		BitmapBuffer*			fFrontBuffer;
};

#endif // BITMAP_HW_INTERFACE_H
