/*
 * Copyright 2005-2012, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef HW_INTERFACE_H
#define HW_INTERFACE_H


#include <AutoDeleter.h>
#include <Accelerant.h>
#include <GraphicsCard.h>
#include <List.h>
#include <Locker.h>
#include <OS.h>
#include <Region.h>

#include <video_overlay.h>

#include <new>

#include "IntRect.h"
#include "MultiLocker.h"
#include "ServerCursor.h"


class BString;
class DrawingEngine;
class EventStream;
class Overlay;
class RenderingBuffer;
class ServerBitmap;
class UpdateQueue;


enum {
	HW_ACC_COPY_REGION					= 0x00000001,
	HW_ACC_FILL_REGION					= 0x00000002,
	HW_ACC_INVERT_REGION				= 0x00000004,
};


class HWInterfaceListener {
public:
								HWInterfaceListener();
	virtual						~HWInterfaceListener();

	virtual	void				FrameBufferChanged() {};
		// Informs a downstream DrawingEngine of a changed framebuffer.

	virtual	void				ScreenChanged(HWInterface* interface) {};
		// Informs an upstream client of a changed screen configuration.
};


class HWInterface : protected MultiLocker {
public:
								HWInterface(bool doubleBuffered = false,
									bool enableUpdateQueue = true);
	virtual						~HWInterface();

	// locking
			bool				LockParallelAccess() { return ReadLock(); }
#if DEBUG
			bool				IsParallelAccessLocked() const
									{ return IsReadLocked(); }
#endif
			void				UnlockParallelAccess() { ReadUnlock(); }

			bool				LockExclusiveAccess() { return WriteLock(); }
			bool				IsExclusiveAccessLocked()
									{ return IsWriteLocked(); }
			void				UnlockExclusiveAccess() { WriteUnlock(); }

	// You need to WriteLock
	virtual	status_t			Initialize();
	virtual	status_t			Shutdown() = 0;

	// allocating a DrawingEngine attached to this HWInterface
	virtual	DrawingEngine*		CreateDrawingEngine();

	// creating an event stream specific for this HWInterface
	// returns NULL when there is no specific event stream necessary
	virtual	EventStream*		CreateEventStream();

	// screen mode stuff
	virtual	status_t			SetMode(const display_mode& mode) = 0;
	virtual	void				GetMode(display_mode* mode) = 0;

	virtual status_t			GetDeviceInfo(accelerant_device_info* info) = 0;
	virtual status_t			GetFrameBufferConfig(
									frame_buffer_config& config) = 0;
	virtual status_t			GetModeList(display_mode** _modeList,
									uint32* _count) = 0;
	virtual status_t			GetPixelClockLimits(display_mode* mode,
									uint32* _low, uint32* _high) = 0;
	virtual status_t			GetTimingConstraints(display_timing_constraints*
									constraints) = 0;
	virtual status_t			ProposeMode(display_mode* candidate,
									const display_mode* low,
									const display_mode* high) = 0;
	virtual	status_t			GetPreferredMode(display_mode* mode);
	virtual status_t			GetMonitorInfo(monitor_info* info);

	virtual sem_id				RetraceSemaphore() = 0;
	virtual status_t			WaitForRetrace(
									bigtime_t timeout = B_INFINITE_TIMEOUT) = 0;

	virtual status_t			SetDPMSMode(uint32 state) = 0;
	virtual uint32				DPMSMode() = 0;
	virtual uint32				DPMSCapabilities() = 0;

	virtual status_t			SetBrightness(float) = 0;
	virtual status_t			GetBrightness(float*) = 0;

	virtual status_t			GetAccelerantPath(BString& path);
	virtual status_t			GetDriverPath(BString& path);

	// query for available hardware accleration and perform it
	// (Initialize() must have been called already)
	virtual	uint32				AvailableHWAcceleration() const
									{ return 0; }

	virtual	void				CopyRegion(const clipping_rect* sortedRectList,
									uint32 count, int32 xOffset, int32 yOffset)
									{}
	virtual	void				FillRegion(/*const*/ BRegion& region,
									const rgb_color& color, bool autoSync) {}
	virtual	void				InvertRegion(/*const*/ BRegion& region) {}

	virtual	void				Sync() {}

	// cursor handling (these do their own Read/Write locking)
			ServerCursorReference Cursor() const;
			ServerCursorReference CursorAndDragBitmap() const;
	virtual	void				SetCursor(ServerCursor* cursor);
	virtual	void				SetCursorVisible(bool visible);
			bool				IsCursorVisible();
	virtual	void				ObscureCursor();
	virtual	void				MoveCursorTo(float x, float y);
			BPoint				CursorPosition();

	virtual	void				SetDragBitmap(const ServerBitmap* bitmap,
									const BPoint& offsetFromCursor);

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

	// frame buffer access (you need to ReadLock!)
			RenderingBuffer*	DrawingBuffer() const;
	virtual	RenderingBuffer*	FrontBuffer() const = 0;
	virtual	RenderingBuffer*	BackBuffer() const = 0;
			void				SetAsyncDoubleBuffered(bool doubleBuffered);
	virtual	bool				IsDoubleBuffered() const;

	// Invalidate is used for scheduling an area for updating
	virtual	status_t			InvalidateRegion(const BRegion& region);
	virtual	status_t			Invalidate(const BRect& frame);
	// while as CopyBackToFront() actually performs the operation
	// either directly or asynchronously by the UpdateQueue thread
	virtual	status_t			CopyBackToFront(const BRect& frame);

protected:
	virtual	void				_CopyBackToFront(/*const*/ BRegion& region);

public:
	// TODO: Just a quick and primitive way to get single buffered mode working.
	// Later, the implementation should be smarter, right now, it will
	// draw the cursor for almost every drawing operation.
	// It seems to me BeOS hides the cursor (in laymans words) before
	// BView::Draw() is called (if the cursor is within that views clipping region),
	// then, after all drawing commands that triggered have been caried out,
	// it shows the cursor again. This approach would have the advantage of
	// the code not cluttering/slowing down DrawingEngine.
	// For now, we hide the cursor for any drawing operation that has
	// a bounding box containing the cursor (in DrawingEngine) so
	// the cursor hiding is completely transparent from code using DrawingEngine.
	// ---
	// NOTE: Investigate locking for these! The client code should already hold a
	// ReadLock, but maybe these functions should acquire a WriteLock!
			bool				HideFloatingOverlays(const BRect& area);
			bool				HideFloatingOverlays();
			void				ShowFloatingOverlays();

	// Listener support
			bool				AddListener(HWInterfaceListener* listener);
			void				RemoveListener(HWInterfaceListener* listener);

protected:
	// implement this in derived classes
	virtual	void				_DrawCursor(IntRect area) const;

	// does the actual transfer and handles color space conversion
			void				_CopyToFront(uint8* src, uint32 srcBPR, int32 x,
									int32 y, int32 right, int32 bottom) const;

			IntRect				_CursorFrame() const;
			void				_RestoreCursorArea() const;
			void				_AdoptDragBitmap();

			void				_NotifyFrameBufferChanged();
			void				_NotifyScreenChanged();

	static	bool				_IsValidMode(const display_mode& mode);

			// If we draw the cursor somewhere in the drawing buffer,
			// we need to backup its contents before drawing, so that
			// we can restore that area when the cursor needs to be
			// drawn somewhere else.
			struct buffer_clip {
				buffer_clip(int32 width, int32 height)
				{
					bpr = width * 4;
					if (bpr > 0 && height > 0)
						buffer = new(std::nothrow) uint8[bpr * height];
					else
						buffer = NULL;
					left = 0;
					top = 0;
					right = -1;
					bottom = -1;
					cursor_hidden = true;
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
				bool			cursor_hidden;
			};

			ObjectDeleter<buffer_clip>
								fCursorAreaBackup;
	mutable	BLocker				fFloatingOverlaysLock;

			ServerCursorReference
								fCursor;
			BReference<ServerBitmap>
								fDragBitmap;
			BPoint				fDragBitmapOffset;
			ServerCursorReference
								fCursorAndDragBitmap;
			bool				fCursorVisible;
			bool				fCursorObscured;
			bool				fHardwareCursorEnabled;
			BPoint				fCursorLocation;

			BRect				fTrackingRect;

			bool				fDoubleBuffered;
			int					fVGADevice;

private:
			ObjectDeleter<UpdateQueue>
								fUpdateExecutor;

			BList				fListeners;
};

#endif // HW_INTERFACE_H
