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
								HWInterface(bool doubleBuffered = false);
	virtual						~HWInterface();

	virtual	status_t			Initialize() = 0;
	virtual	status_t			Shutdown() = 0;

	// screen mode stuff
	virtual	status_t			SetMode(const display_mode &mode) = 0;
	virtual	void				GetMode(display_mode *mode) = 0;

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
			bool				IsCursorVisible();
	virtual	void				MoveCursorTo(const float& x,
											 const float& y);
			BPoint				GetCursorPosition();

	// frame buffer access
			RenderingBuffer*	DrawingBuffer() const;
	virtual	RenderingBuffer*	FrontBuffer() const = 0;
	virtual	RenderingBuffer*	BackBuffer() const = 0;
	virtual	bool				IsDoubleBuffered() const;

	// Invalidate is planned to be used for scheduling an area for updating
	virtual	status_t			Invalidate(const BRect& frame);
	// while as CopyBackToFront() actually performs the operation
			status_t			CopyBackToFront(const BRect& frame);

	// TODO: Just a quick and primitive way to get single buffered mode working.
	// Later, the implementation should be smarter, right now, it will
	// draw the cursor for almost every drawing operation.
	// It seems to me, BeOS hides the cursor (in laymans words) before
	// BView::Draw() is called, then, after all drawing commands that triggered
	// have been caried out, it shows the cursor again. This approach would
	// have the adventage of the code not cluttering/slowing down DisplayDriverPainter.
			void				HideSoftwareCursor(const BRect& area);
			void				HideSoftwareCursor();
			void				ShowSoftwareCursor();

 protected:
	// implement this in derived classes
	virtual	void				_DrawCursor(BRect area) const;

	// does the actual transfer and handles color space conversion
			void				_CopyToFront(uint8* src, uint32 srcBPR,
											 int32 x, int32 y,
											 int32 right, int32 bottom) const;

			BRect				_CursorFrame() const;
			void				_RestoreCursorArea(const BRect& frame) const;

			// If we draw the cursor somewhere in the drawing buffer,
			// we need to backup its contents before drawing, so that
			// we can restore that area when the cursor needs to be
			// drawn somewhere else.
			struct buffer_clip {
								buffer_clip(int32 width, int32 height)
								{
									bpr = width * 4;
									if (bpr > 0 && height > 0)
										buffer = new uint8[bpr * height];
									else
										buffer = NULL;
									left = 0;
									top = 0;
									right = -1;
									bottom = -1;
								}
								~buffer_clip()
								{
									delete[] buffer;
								}
				uint8*			buffer;
				int32			left;
				int32			top;
				int32			right;
				int32			bottom;
				int32			bpr;
			};

			buffer_clip*		fCursorAreaBackup;
			bool				fSoftwareCursorHidden;

			ServerCursor*		fCursor;
			bool				fCursorVisible;
			BPoint				fCursorLocation;
			bool				fDoubleBuffered;

 private:
			UpdateQueue*		fUpdateExecutor;
};

#endif // HW_INTERFACE_H
