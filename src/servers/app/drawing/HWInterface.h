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
#include <Locker.h>
#include <OS.h>
#include <Rect.h>

class RenderingBuffer;
class ServerCursor;
class UpdateQueue;

class HWInterface : public BLocker {
 public:
								HWInterface();
	virtual						~HWInterface();

	virtual	status_t			Initialize() = 0;
	virtual	status_t			Shutdown() = 0;

	// screen mode stuff
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

	virtual status_t			SetDPMSMode(const uint32 &state) = 0;
	virtual uint32				DPMSMode() const = 0;
	virtual uint32				DPMSCapabilities() const = 0;

	// cursor handling
	virtual	void				SetCursor(ServerCursor* cursor);
	virtual	void				SetCursorVisible(bool visible);
	virtual	bool				IsCursorVisible();
	virtual	void				MoveCursorTo(const float& x,
											 const float& y);
			BPoint				GetCursorPosition();

	// frame buffer access
	virtual	RenderingBuffer*	FrontBuffer() const = 0;
	virtual	RenderingBuffer*	BackBuffer() const = 0;

	// Invalidate is planned to be used for scheduling an area for updating
			status_t			Invalidate(const BRect& frame);
	// while as CopyBackToFront() actually performs the operation
	virtual	status_t			CopyBackToFront(const BRect& frame);

 protected:
	// implement this in derived classes
	virtual	void				_DrawCursor(BRect area) const = 0;

	// does the actual transfer and handles color space conversion
			void				_CopyToFront(uint8* src, uint32 srcBPR,
											 int32 x, int32 y,
											 int32 right, int32 bottom) const;

			BRect				_CursorFrame() const;

			ServerCursor*		fCursor;
			bool				fCursorVisible;
			BPoint				fCursorLocation;

 private:
			UpdateQueue*		fUpdateExecutor;
};

#endif // HW_INTERFACE_H
