//
// Copyright 2005, Stephan Aßmus <superstippi@gmx.de>
// Distributed under the terms of the MIT License.
//
// Contains an abstract base class HWInterface that provides the
// basic functionality for frame buffer acces in the DisplayDriverPainter
// implementation.

#ifndef HW_INTERFACE_H
#define HW_INTERFACE_H

#include <Accelerant.h>
#include <GraphicsCard.h>
#include <OS.h>

class RenderingBuffer;
class BRect;

class HWInterface {
 public:
								HWInterface();
	virtual						~HWInterface();

	virtual	status_t			SetMode(const display_mode &mode) = 0;
//	virtual	void				GetMode(display_mode *mode) = 0;


	virtual status_t			GetDeviceInfo(accelerant_device_info *info) = 0;
	virtual status_t			GetModeList(display_mode **mode_list,
											uint32 *count) = 0;
	virtual status_t			GetPixelClockLimits(display_mode *mode,
													uint32 *low,
													uint32 *high) = 0;
	virtual status_t			GetTimingConstraints(display_timing_constraints *dtc) = 0;
	virtual status_t			ProposeMode(display_mode *candidate,
											const display_mode *low,
											const display_mode *high) = 0;

	virtual status_t			WaitForRetrace(bigtime_t timeout = B_INFINITE_TIMEOUT) = 0;

	// frame buffer access
	virtual	RenderingBuffer*	FrontBuffer() const = 0;
	virtual	RenderingBuffer*	BackBuffer() const = 0;

	// Invalidate is planned to be used for scheduling an area for updating
	virtual	status_t			Invalidate(const BRect& frame) = 0;
	// while as CopyBackToFront() actually performs the operation
	virtual	status_t			CopyBackToFront(const BRect& frame) = 0;
};

#endif // HW_INTERFACE_H
