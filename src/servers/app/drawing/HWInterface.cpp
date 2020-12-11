/*
 * Copyright 2005-2012, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "HWInterface.h"

#include <new>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <vesa/vesa_info.h>

#include "drawing_support.h"

#include "DrawingEngine.h"
#include "RenderingBuffer.h"
#include "SystemPalette.h"
#include "UpdateQueue.h"


using std::nothrow;


HWInterfaceListener::HWInterfaceListener()
{
}


HWInterfaceListener::~HWInterfaceListener()
{
}


// #pragma mark - HWInterface


HWInterface::HWInterface(bool doubleBuffered, bool enableUpdateQueue)
	:
	MultiLocker("hw interface lock"),
	fFloatingOverlaysLock("floating overlays lock"),
	fCursor(NULL),
	fDragBitmap(NULL),
	fDragBitmapOffset(0, 0),
	fCursorAndDragBitmap(NULL),
	fCursorVisible(false),
	fCursorObscured(false),
	fHardwareCursorEnabled(false),
	fCursorLocation(0, 0),
	fDoubleBuffered(doubleBuffered),
	fVGADevice(-1),
	fUpdateExecutor(NULL),
	fListeners(20)
{
	SetAsyncDoubleBuffered(doubleBuffered && enableUpdateQueue);
}


HWInterface::~HWInterface()
{
	SetAsyncDoubleBuffered(false);
}


status_t
HWInterface::Initialize()
{
	return MultiLocker::InitCheck();
}


DrawingEngine*
HWInterface::CreateDrawingEngine()
{
	return new(std::nothrow) DrawingEngine(this);
}


EventStream*
HWInterface::CreateEventStream()
{
	return NULL;
}


status_t
HWInterface::GetAccelerantPath(BString &path)
{
	return B_ERROR;
}


status_t
HWInterface::GetDriverPath(BString &path)
{
	return B_ERROR;
}


status_t
HWInterface::GetPreferredMode(display_mode* mode)
{
	return B_NOT_SUPPORTED;
}


status_t
HWInterface::GetMonitorInfo(monitor_info* info)
{
	return B_NOT_SUPPORTED;
}


// #pragma mark -


void
HWInterface::SetCursor(ServerCursor* cursor)
{
	if (!fFloatingOverlaysLock.Lock())
		return;

	if (fCursor.Get() != cursor) {
		BRect oldFrame = _CursorFrame();

		fCursor.SetTo(cursor);

		Invalidate(oldFrame);

		_AdoptDragBitmap();
		Invalidate(_CursorFrame());
	}
	fFloatingOverlaysLock.Unlock();
}


ServerCursorReference
HWInterface::Cursor() const
{
	if (!fFloatingOverlaysLock.Lock())
		return ServerCursorReference(NULL);

	fFloatingOverlaysLock.Unlock();
	return fCursor;
}


ServerCursorReference
HWInterface::CursorAndDragBitmap() const
{
	if (!fFloatingOverlaysLock.Lock())
		return ServerCursorReference(NULL);

	fFloatingOverlaysLock.Unlock();
	return fCursorAndDragBitmap;
}


void
HWInterface::SetCursorVisible(bool visible)
{
	if (!fFloatingOverlaysLock.Lock())
		return;

	if (fCursorVisible != visible) {
		// NOTE: _CursorFrame() will
		// return an invalid rect if
		// fCursorVisible == false!
		if (visible) {
			fCursorVisible = visible;
			fCursorObscured = false;
			IntRect r = _CursorFrame();

			_DrawCursor(r);
			Invalidate(r);
		} else {
			IntRect r = _CursorFrame();
			fCursorVisible = visible;

			_RestoreCursorArea();
			Invalidate(r);
		}
	}
	fFloatingOverlaysLock.Unlock();
}


bool
HWInterface::IsCursorVisible()
{
	bool visible = true;
	if (fFloatingOverlaysLock.Lock()) {
		visible = fCursorVisible;
		fFloatingOverlaysLock.Unlock();
	}
	return visible;
}


void
HWInterface::ObscureCursor()
{
	if (!fFloatingOverlaysLock.Lock())
		return;

	if (!fCursorObscured) {
		SetCursorVisible(false);
		fCursorObscured = true;
	}
	fFloatingOverlaysLock.Unlock();
}


void
HWInterface::MoveCursorTo(float x, float y)
{
	if (!fFloatingOverlaysLock.Lock())
		return;

	BPoint p(x, y);
	if (p != fCursorLocation) {
		// unhide cursor if it is obscured only
		if (fCursorObscured) {
			SetCursorVisible(true);
		}
		IntRect oldFrame = _CursorFrame();
		fCursorLocation = p;
		if (fCursorVisible) {
			// Invalidate and _DrawCursor would not draw
			// anything if the cursor is hidden
			// (invalid cursor frame), but explicitly
			// testing for it here saves us some cycles
			if (fCursorAreaBackup.Get() != NULL) {
				// means we have a software cursor which we need to draw
				_RestoreCursorArea();
				_DrawCursor(_CursorFrame());
			}
			IntRect newFrame = _CursorFrame();
			if (newFrame.Intersects(oldFrame))
				Invalidate(oldFrame | newFrame);
			else {
				Invalidate(oldFrame);
				Invalidate(newFrame);
			}
		}
	}
	fFloatingOverlaysLock.Unlock();
}


BPoint
HWInterface::CursorPosition()
{
	BPoint location;
	if (fFloatingOverlaysLock.Lock()) {
		location = fCursorLocation;
		fFloatingOverlaysLock.Unlock();
	}
	return location;
}


void
HWInterface::SetDragBitmap(const ServerBitmap* bitmap,
	const BPoint& offsetFromCursor)
{
	if (fFloatingOverlaysLock.Lock()) {
		fDragBitmap.SetTo((ServerBitmap*)bitmap, false);
		fDragBitmapOffset = offsetFromCursor;
		_AdoptDragBitmap();
		fFloatingOverlaysLock.Unlock();
	}
}


// #pragma mark -


RenderingBuffer*
HWInterface::DrawingBuffer() const
{
	if (IsDoubleBuffered())
		return BackBuffer();
	return FrontBuffer();
}


void
HWInterface::SetAsyncDoubleBuffered(bool doubleBuffered)
{
	if (doubleBuffered) {
		if (fUpdateExecutor.Get() != NULL)
			return;
		fUpdateExecutor.SetTo(new (nothrow) UpdateQueue(this));
		AddListener(fUpdateExecutor.Get());
	} else {
		if (fUpdateExecutor.Get() == NULL)
			return;
		RemoveListener(fUpdateExecutor.Get());
		fUpdateExecutor.Unset();
	}
}


bool
HWInterface::IsDoubleBuffered() const
{
	return fDoubleBuffered;
}


/*! The object needs to be already locked!
*/
status_t
HWInterface::InvalidateRegion(const BRegion& region)
{
	int32 count = region.CountRects();
	for (int32 i = 0; i < count; i++) {
		status_t result = Invalidate(region.RectAt(i));
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


/*! The object needs to be already locked!
*/
status_t
HWInterface::Invalidate(const BRect& frame)
{
	if (IsDoubleBuffered()) {
#if 0
// NOTE: The UpdateQueue works perfectly fine, but it screws the
// flicker-free-ness of the double buffered rendering. The problem being the
// asynchronous nature. The UpdateQueue will transfer regions of the screen
// which have been clean at the time we are in this function, but which have
// been damaged meanwhile by drawing into them again. All in all, the
// UpdateQueue is good for reducing the number of times that the transfer
// is performed, and makes it happen during refresh only, but until there
// is a smarter way to synchronize this all better, I've disabled it.
		if (fUpdateExecutor != NULL) {
			fUpdateExecutor->AddRect(frame);
			return B_OK;
		}
#endif
		return CopyBackToFront(frame);
	}
	return B_OK;
}


/*! The object must already be locked!
*/
status_t
HWInterface::CopyBackToFront(const BRect& frame)
{
	RenderingBuffer* frontBuffer = FrontBuffer();
	RenderingBuffer* backBuffer = BackBuffer();

	if (!backBuffer || !frontBuffer)
		return B_NO_INIT;

	// we need to mess with the area, but it is const
	IntRect area(frame);
	IntRect bufferClip(backBuffer->Bounds());

	if (area.IsValid() && area.Intersects(bufferClip)) {

		// make sure we don't copy out of bounds
		area = bufferClip & area;

		bool cursorLocked = fFloatingOverlaysLock.Lock();

		BRegion region((BRect)area);
		if (IsDoubleBuffered())
			region.Exclude((clipping_rect)_CursorFrame());

		_CopyBackToFront(region);

		_DrawCursor(area);

		if (cursorLocked)
			fFloatingOverlaysLock.Unlock();

		return B_OK;
	}
	return B_BAD_VALUE;
}


void
HWInterface::_CopyBackToFront(/*const*/ BRegion& region)
{
	RenderingBuffer* backBuffer = BackBuffer();

	uint32 srcBPR = backBuffer->BytesPerRow();
	uint8* src = (uint8*)backBuffer->Bits();

	int32 count = region.CountRects();
	for (int32 i = 0; i < count; i++) {
		clipping_rect r = region.RectAtInt(i);
		// offset to left top pixel in source buffer (always B_RGBA32)
		uint8* srcOffset = src + r.top * srcBPR + r.left * 4;
		_CopyToFront(srcOffset, srcBPR, r.left, r.top, r.right, r.bottom);
	}
}


// #pragma mark -


overlay_token
HWInterface::AcquireOverlayChannel()
{
	return NULL;
}


void
HWInterface::ReleaseOverlayChannel(overlay_token token)
{
}


status_t
HWInterface::GetOverlayRestrictions(const Overlay* overlay,
	overlay_restrictions* restrictions)
{
	return B_NOT_SUPPORTED;
}


bool
HWInterface::CheckOverlayRestrictions(int32 width, int32 height,
	color_space colorSpace)
{
	return false;
}


const overlay_buffer*
HWInterface::AllocateOverlayBuffer(int32 width, int32 height, color_space space)
{
	return NULL;
}


void
HWInterface::FreeOverlayBuffer(const overlay_buffer* buffer)
{
}


void
HWInterface::ConfigureOverlay(Overlay* overlay)
{
}


void
HWInterface::HideOverlay(Overlay* overlay)
{
}


// #pragma mark -


bool
HWInterface::HideFloatingOverlays(const BRect& area)
{
	if (IsDoubleBuffered())
		return false;
	if (!fFloatingOverlaysLock.Lock())
		return false;
	if (fCursorAreaBackup.Get() != NULL && !fCursorAreaBackup->cursor_hidden) {
		BRect backupArea(fCursorAreaBackup->left, fCursorAreaBackup->top,
			fCursorAreaBackup->right, fCursorAreaBackup->bottom);
		if (area.Intersects(backupArea)) {
			_RestoreCursorArea();
			// do not unlock the cursor lock
			return true;
		}
	}
	fFloatingOverlaysLock.Unlock();
	return false;
}


bool
HWInterface::HideFloatingOverlays()
{
	if (IsDoubleBuffered())
		return false;
	if (!fFloatingOverlaysLock.Lock())
		return false;

	_RestoreCursorArea();
	return true;
}


void
HWInterface::ShowFloatingOverlays()
{
	if (fCursorAreaBackup.Get() != NULL && fCursorAreaBackup->cursor_hidden)
		_DrawCursor(_CursorFrame());

	fFloatingOverlaysLock.Unlock();
}


// #pragma mark -


bool
HWInterface::AddListener(HWInterfaceListener* listener)
{
	if (listener && !fListeners.HasItem(listener))
		return fListeners.AddItem(listener);
	return false;
}


void
HWInterface::RemoveListener(HWInterfaceListener* listener)
{
	fListeners.RemoveItem(listener);
}


// #pragma mark -


/*!	Default implementation, can be used as fallback or for software cursor.
	\param area is where we potentially draw the cursor, the cursor
		might be somewhere else, in which case this function does nothing
*/
void
HWInterface::_DrawCursor(IntRect area) const
{
	RenderingBuffer* backBuffer = DrawingBuffer();
	if (!backBuffer || !area.IsValid())
		return;

	IntRect cf = _CursorFrame();

	// make sure we don't copy out of bounds
	area = backBuffer->Bounds() & area;

	if (cf.IsValid() && area.Intersects(cf)) {

		// clip to common area
		area = area & cf;

		int32 width = area.right - area.left + 1;
		int32 height = area.bottom - area.top + 1;

		// make a bitmap from the backbuffer
		// that has the cursor blended on top of it

		// blending buffer
		uint8* buffer = new(std::nothrow) uint8[width * height * 4];
			// TODO: cache this buffer
		if (buffer == NULL)
			return;

		// offset into back buffer
		uint8* src = (uint8*)backBuffer->Bits();
		uint32 srcBPR = backBuffer->BytesPerRow();
		src += area.top * srcBPR + area.left * 4;

		// offset into cursor bitmap
		uint8* crs = (uint8*)fCursorAndDragBitmap->Bits();
		uint32 crsBPR = fCursorAndDragBitmap->BytesPerRow();
		// since area is clipped to cf,
		// the diff between area.top and cf.top is always positive,
		// same for diff between area.left and cf.left
		crs += (area.top - (int32)floorf(cf.top)) * crsBPR
				+ (area.left - (int32)floorf(cf.left)) * 4;

		uint8* dst = buffer;

		if (fCursorAreaBackup.Get() != NULL && fCursorAreaBackup->buffer
			&& fFloatingOverlaysLock.Lock()) {
			fCursorAreaBackup->cursor_hidden = false;
			// remember which area the backup contains
			fCursorAreaBackup->left = area.left;
			fCursorAreaBackup->top = area.top;
			fCursorAreaBackup->right = area.right;
			fCursorAreaBackup->bottom = area.bottom;
			uint8* bup = fCursorAreaBackup->buffer;
			uint32 bupBPR = fCursorAreaBackup->bpr;

			// blending and backup of drawing buffer
			for (int32 y = area.top; y <= area.bottom; y++) {
				uint8* s = src;
				uint8* c = crs;
				uint8* d = dst;
				uint8* b = bup;

				for (int32 x = area.left; x <= area.right; x++) {
					*(uint32*)b = *(uint32*)s;
					// assumes backbuffer alpha = 255
					// assuming pre-multiplied cursor bitmap
					int a = 255 - c[3];
					d[0] = ((int)(b[0] * a + 255) >> 8) + c[0];
					d[1] = ((int)(b[1] * a + 255) >> 8) + c[1];
					d[2] = ((int)(b[2] * a + 255) >> 8) + c[2];

					s += 4;
					c += 4;
					d += 4;
					b += 4;
				}
				crs += crsBPR;
				src += srcBPR;
				dst += width * 4;
				bup += bupBPR;
			}
			fFloatingOverlaysLock.Unlock();
		} else {
			// blending
			for (int32 y = area.top; y <= area.bottom; y++) {
				uint8* s = src;
				uint8* c = crs;
				uint8* d = dst;
				for (int32 x = area.left; x <= area.right; x++) {
					// assumes backbuffer alpha = 255
					// assuming pre-multiplied cursor bitmap
					uint8 a = 255 - c[3];
					d[0] = ((s[0] * a + 255) >> 8) + c[0];
					d[1] = ((s[1] * a + 255) >> 8) + c[1];
					d[2] = ((s[2] * a + 255) >> 8) + c[2];

					s += 4;
					c += 4;
					d += 4;
				}
				crs += crsBPR;
				src += srcBPR;
				dst += width * 4;
			}
		}
		// copy result to front buffer
		_CopyToFront(buffer, width * 4, area.left, area.top, area.right,
			area.bottom);

		delete[] buffer;
	}
}


/*!	- source is assumed to be already at the right offset
	- source is assumed to be in B_RGBA32 format
	- location in front buffer is calculated
	- conversion from B_RGBA32 to format of front buffer is taken care of
*/
void
HWInterface::_CopyToFront(uint8* src, uint32 srcBPR, int32 x, int32 y,
	int32 right, int32 bottom) const
{
	RenderingBuffer* frontBuffer = FrontBuffer();

	uint8* dst = (uint8*)frontBuffer->Bits();
	uint32 dstBPR = frontBuffer->BytesPerRow();

	// transfer, handle colorspace conversion
	switch (frontBuffer->ColorSpace()) {
		case B_RGB32:
		case B_RGBA32:
		{
			int32 bytes = (right - x + 1) * 4;

			if (bytes > 0) {
				// offset to left top pixel in dest buffer
				dst += y * dstBPR + x * 4;
				// copy
				for (; y <= bottom; y++) {
					// bytes is guaranteed to be multiple of 4
					gfxcpy32(dst, src, bytes);
					dst += dstBPR;
					src += srcBPR;
				}
			}
			break;
		}

		case B_RGB24:
		{
			// offset to left top pixel in dest buffer
			dst += y * dstBPR + x * 3;
			int32 left = x;
			// copy
			for (; y <= bottom; y++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (x = left; x <= right; x++) {
					dstHandle[0] = srcHandle[0];
					dstHandle[1] = srcHandle[1];
					dstHandle[2] = srcHandle[2];
					dstHandle += 3;
					srcHandle += 4;
				}
				dst += dstBPR;
				src += srcBPR;
			}
			break;
		}

		case B_RGB16:
		{
			// offset to left top pixel in dest buffer
			dst += y * dstBPR + x * 2;
			int32 left = x;
			// copy
			// TODO: assumes BGR order, does this work on big endian as well?
			for (; y <= bottom; y++) {
				uint8* srcHandle = src;
				uint16* dstHandle = (uint16*)dst;
				for (x = left; x <= right; x++) {
					*dstHandle = (uint16)(((srcHandle[2] & 0xf8) << 8)
						| ((srcHandle[1] & 0xfc) << 3) | (srcHandle[0] >> 3));
					dstHandle ++;
					srcHandle += 4;
				}
				dst += dstBPR;
				src += srcBPR;
			}
			break;
		}

		case B_RGB15:
		case B_RGBA15:
		{
			// offset to left top pixel in dest buffer
			dst += y * dstBPR + x * 2;
			int32 left = x;
			// copy
			// TODO: assumes BGR order, does this work on big endian as well?
			for (; y <= bottom; y++) {
				uint8* srcHandle = src;
				uint16* dstHandle = (uint16*)dst;
				for (x = left; x <= right; x++) {
					*dstHandle = (uint16)(((srcHandle[2] & 0xf8) << 7)
						| ((srcHandle[1] & 0xf8) << 2) | (srcHandle[0] >> 3));
					dstHandle ++;
					srcHandle += 4;
				}
				dst += dstBPR;
				src += srcBPR;
			}
			break;
		}

		case B_CMAP8:
		{
			const color_map *colorMap = SystemColorMap();
			// offset to left top pixel in dest buffer
			dst += y * dstBPR + x;
			int32 left = x;
			uint16 index;
			// copy
			// TODO: assumes BGR order again
			for (; y <= bottom; y++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (x = left; x <= right; x++) {
					index = ((srcHandle[2] & 0xf8) << 7)
						| ((srcHandle[1] & 0xf8) << 2) | (srcHandle[0] >> 3);
					*dstHandle = colorMap->index_map[index];
					dstHandle ++;
					srcHandle += 4;
				}
				dst += dstBPR;
				src += srcBPR;
			}

			break;
		}

		case B_GRAY8:
			if (frontBuffer->Width() > dstBPR) {
				// VGA 16 color grayscale planar mode
				if (fVGADevice >= 0) {
					vga_planar_blit_args args;
					args.source = src;
					args.source_bytes_per_row = srcBPR;
					args.left = x;
					args.top = y;
					args.right = right;
					args.bottom = bottom;
					if (ioctl(fVGADevice, VGA_PLANAR_BLIT, &args, sizeof(args))
							== 0)
						break;
				}

				// Since we cannot set the plane, we do monochrome output
				dst += y * dstBPR + x / 8;
				int32 left = x;

				// TODO: this is awfully slow...
				// TODO: assumes BGR order
				for (; y <= bottom; y++) {
					uint8* srcHandle = src;
					uint8* dstHandle = dst;
					uint8 current8 = dstHandle[0];
						// we store 8 pixels before writing them back

					for (x = left; x <= right; x++) {
						uint8 pixel = (308 * srcHandle[2] + 600 * srcHandle[1]
							+ 116 * srcHandle[0]) / 1024;
						srcHandle += 4;

						if (pixel > 128)
							current8 |= 0x80 >> (x & 7);
						else
							current8 &= ~(0x80 >> (x & 7));

						if ((x & 7) == 7) {
							// last pixel in 8 pixel group
							dstHandle[0] = current8;
							dstHandle++;
							current8 = dstHandle[0];
						}
					}

					if (x & 7) {
						// last pixel has not been written yet
						dstHandle[0] = current8;
					}
					dst += dstBPR;
					src += srcBPR;
				}
			} else {
				// offset to left top pixel in dest buffer
				dst += y * dstBPR + x;
				int32 left = x;
				// copy
				// TODO: assumes BGR order, does this work on big endian as well?
				for (; y <= bottom; y++) {
					uint8* srcHandle = src;
					uint8* dstHandle = dst;
					for (x = left; x <= right; x++) {
						*dstHandle = (308 * srcHandle[2] + 600 * srcHandle[1]
							+ 116 * srcHandle[0]) / 1024;
						dstHandle ++;
						srcHandle += 4;
					}
					dst += dstBPR;
					src += srcBPR;
				}
			}
			break;

		default:
			fprintf(stderr, "HWInterface::CopyBackToFront() - unsupported "
				"front buffer format! (0x%x)\n", frontBuffer->ColorSpace());
			break;
	}
}


/*!	The object must be locked
*/
IntRect
HWInterface::_CursorFrame() const
{
	IntRect frame(0, 0, -1, -1);
	if (fCursorAndDragBitmap && fCursorVisible && !fHardwareCursorEnabled) {
		frame = fCursorAndDragBitmap->Bounds();
		frame.OffsetTo(fCursorLocation - fCursorAndDragBitmap->GetHotSpot());
	}
	return frame;
}


void
HWInterface::_RestoreCursorArea() const
{
	if (fCursorAreaBackup.Get() != NULL && !fCursorAreaBackup->cursor_hidden) {
		_CopyToFront(fCursorAreaBackup->buffer, fCursorAreaBackup->bpr,
			fCursorAreaBackup->left, fCursorAreaBackup->top,
			fCursorAreaBackup->right, fCursorAreaBackup->bottom);

		fCursorAreaBackup->cursor_hidden = true;
	}
}


void
HWInterface::_AdoptDragBitmap()
{
	// TODO: support other colorspaces/convert bitmap
	if (fDragBitmap && !(fDragBitmap->ColorSpace() == B_RGB32
		|| fDragBitmap->ColorSpace() == B_RGBA32)) {
		fprintf(stderr, "HWInterface::_AdoptDragBitmap() - bitmap has yet "
			"unsupported colorspace\n");
		return;
	}

	_RestoreCursorArea();
	BRect oldCursorFrame = _CursorFrame();

	if (fDragBitmap != NULL && fDragBitmap->Bounds().Width() > 0 && fDragBitmap->Bounds().Height() > 0) {
		BRect bitmapFrame = fDragBitmap->Bounds();
		if (fCursor) {
			// put bitmap frame and cursor frame into the same
			// coordinate space (the cursor location is the origin)
			bitmapFrame.OffsetTo(BPoint(-fDragBitmapOffset.x, -fDragBitmapOffset.y));

			BRect cursorFrame(fCursor->Bounds());
			BPoint hotspot(fCursor->GetHotSpot());
				// the hotspot is at the origin
			cursorFrame.OffsetTo(-hotspot.x, -hotspot.y);

			BRect combindedBounds = bitmapFrame | cursorFrame;

			BPoint shift;
			shift.x = -combindedBounds.left;
			shift.y = -combindedBounds.top;

			combindedBounds.OffsetBy(shift);
			cursorFrame.OffsetBy(shift);
			bitmapFrame.OffsetBy(shift);

			fCursorAndDragBitmap.SetTo(new(std::nothrow) ServerCursor(combindedBounds,
				fDragBitmap->ColorSpace(), 0, shift), true);

			uint8* dst = fCursorAndDragBitmap ? (uint8*)fCursorAndDragBitmap->Bits() : NULL;
			if (dst == NULL) {
				// Oops, we could not allocate memory for the drag bitmap.
				// Let's show the cursor only.
				fCursorAndDragBitmap = fCursor;
			} else {
				// clear the combined buffer
				uint32 dstBPR = fCursorAndDragBitmap->BytesPerRow();

				memset(dst, 0, fCursorAndDragBitmap->BitsLength());

				// put drag bitmap into combined buffer
				uint8* src = (uint8*)fDragBitmap->Bits();
				uint32 srcBPR = fDragBitmap->BytesPerRow();

				dst += (int32)bitmapFrame.top * dstBPR
					+ (int32)bitmapFrame.left * 4;

				uint32 width = bitmapFrame.IntegerWidth() + 1;
				uint32 height = bitmapFrame.IntegerHeight() + 1;

				for (uint32 y = 0; y < height; y++) {
					memcpy(dst, src, srcBPR);
					dst += dstBPR;
					src += srcBPR;
				}

				// compose cursor into combined buffer
				dst = (uint8*)fCursorAndDragBitmap->Bits();
				dst += (int32)cursorFrame.top * dstBPR
					+ (int32)cursorFrame.left * 4;

				src = (uint8*)fCursor->Bits();
				srcBPR = fCursor->BytesPerRow();

				width = cursorFrame.IntegerWidth() + 1;
				height = cursorFrame.IntegerHeight() + 1;

				for (uint32 y = 0; y < height; y++) {
					uint8* d = dst;
					uint8* s = src;
					for (uint32 x = 0; x < width; x++) {
						// takes two semi-transparent pixels
						// with unassociated alpha (not pre-multiplied)
						// and stays within non-premultiplied color space
						if (s[3] > 0) {
							if (s[3] == 255) {
								d[0] = s[0];
								d[1] = s[1];
								d[2] = s[2];
								d[3] = 255;
							} else {
								uint8 alphaRest = 255 - s[3];
								uint32 alphaTemp
									= (65025 - alphaRest * (255 - d[3]));
								uint32 alphaDest = d[3] * alphaRest;
								uint32 alphaSrc = 255 * s[3];
								d[0] = (d[0] * alphaDest + s[0] * alphaSrc)
									/ alphaTemp;
								d[1] = (d[1] * alphaDest + s[1] * alphaSrc)
									/ alphaTemp;
								d[2] = (d[2] * alphaDest + s[2] * alphaSrc)
									/ alphaTemp;
								d[3] = alphaTemp / 255;
							}
						}
						// TODO: make sure the alpha is always upside down,
						// then it doesn't need to be done when drawing the cursor
						// (see _DrawCursor())
						//					d[3] = 255 - d[3];
						d += 4;
						s += 4;
					}
					dst += dstBPR;
					src += srcBPR;
				}

				// handle pre-multiplication with alpha
				// for faster compositing during cursor drawing
				width = combindedBounds.IntegerWidth() + 1;
				height = combindedBounds.IntegerHeight() + 1;

				dst = (uint8*)fCursorAndDragBitmap->Bits();

				for (uint32 y = 0; y < height; y++) {
					uint8* d = dst;
					for (uint32 x = 0; x < width; x++) {
						d[0] = (d[0] * d[3]) >> 8;
						d[1] = (d[1] * d[3]) >> 8;
						d[2] = (d[2] * d[3]) >> 8;
						d += 4;
					}
					dst += dstBPR;
				}
			}
		} else {
			fCursorAndDragBitmap.SetTo(new ServerCursor(fDragBitmap->Bits(),
				bitmapFrame.IntegerWidth() + 1, bitmapFrame.IntegerHeight() + 1,
				fDragBitmap->ColorSpace()), true);
			fCursorAndDragBitmap->SetHotSpot(BPoint(-fDragBitmapOffset.x, -fDragBitmapOffset.y));
		}
	} else {
		fCursorAndDragBitmap = fCursor;
	}

	Invalidate(oldCursorFrame);

	fCursorAreaBackup.Unset();

	if (!fCursorAndDragBitmap)
		return;

	if (fCursorAndDragBitmap && !IsDoubleBuffered()) {
		BRect cursorBounds = fCursorAndDragBitmap->Bounds();
		fCursorAreaBackup.SetTo(new buffer_clip(cursorBounds.IntegerWidth() + 1,
			cursorBounds.IntegerHeight() + 1));
		if (fCursorAreaBackup->buffer == NULL)
			fCursorAreaBackup.Unset();
	}
 	_DrawCursor(_CursorFrame());
}


void
HWInterface::_NotifyFrameBufferChanged()
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		HWInterfaceListener* listener
			= (HWInterfaceListener*)listeners.ItemAtFast(i);
		listener->FrameBufferChanged();
	}
}


void
HWInterface::_NotifyScreenChanged()
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		HWInterfaceListener* listener
			= (HWInterfaceListener*)listeners.ItemAtFast(i);
		listener->ScreenChanged(this);
	}
}


/*static*/ bool
HWInterface::_IsValidMode(const display_mode& mode)
{
	// TODO: more of those!
	if (mode.virtual_width < 320
		|| mode.virtual_height < 200)
		return false;

	return true;
}

