/*
 * Copyright 2005-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include <stdio.h>
#include <string.h>

#include "drawing_support.h"

#include "RenderingBuffer.h"
#include "ServerCursor.h"
#include "SystemPalette.h"
#include "UpdateQueue.h"

#include "HWInterface.h"

// constructor
HWInterface::HWInterface(bool doubleBuffered)
	: MultiLocker("hw interface lock"),
	  fCursorAreaBackup(NULL),
	  fCursor(NULL),
	  fDragBitmap(NULL),
	  fDragBitmapOffset(0, 0),
	  fCursorAndDragBitmap(NULL),
	  fCursorVisible(false),
	  fCursorObscured(false),
	  fCursorLocation(0, 0),
	  fDoubleBuffered(doubleBuffered),
//	  fUpdateExecutor(new UpdateQueue(this))
	  fUpdateExecutor(NULL)
{
}

// destructor
HWInterface::~HWInterface()
{
	delete fCursorAreaBackup;

	// The standard cursor doesn't belong us - the drag bitmap might
	if (fCursor != fCursorAndDragBitmap)
		delete fCursorAndDragBitmap;

	delete fUpdateExecutor;
}

// Initialize
status_t
HWInterface::Initialize()
{
	return MultiLocker::InitCheck();
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


// SetCursor
void
HWInterface::SetCursor(ServerCursor* cursor)
{
	if (WriteLock()) {
		// TODO: if a bitmap is being dragged, it could
		// be considered iritating to the user to change
		// cursor shapes while something is dragged.
		// The disabled code below would do this (except
		// for the minor annoyance that the cursor is not
		// updated when the drag is over)
//		if (fDragBitmap) {
//			// TODO: like a "+" or "-" sign when dragging some files to indicate
//			//		the current drag mode?
//			WriteUnlock();
//			return;
//		}
		if (fCursor != cursor) {
			BRect oldFrame = _CursorFrame();

			if (fCursorAndDragBitmap == fCursor) {
				// make sure _AdoptDragBitmap doesn't delete a real cursor
				fCursorAndDragBitmap = NULL;
			}

			if (fCursor)
				fCursor->Release();

			fCursor = cursor;

			if (fCursor)
				fCursor->Acquire();

			Invalidate(oldFrame);

			_AdoptDragBitmap(fDragBitmap, fDragBitmapOffset);
			Invalidate(_CursorFrame());
		}
		WriteUnlock();
	}
}

// SetCursorVisible
void
HWInterface::SetCursorVisible(bool visible)
{
	if (WriteLock()) {
		if (fCursorVisible != visible) {
			// NOTE: _CursorFrame() will
			// return an invalid rect if
			// fCursorVisible == false!
			if (visible) {
				fCursorVisible = visible;
				fCursorObscured = false;
				BRect r = _CursorFrame();

				_DrawCursor(r);
				Invalidate(r);
			} else {
				BRect r = _CursorFrame();
				fCursorVisible = visible;

				_RestoreCursorArea();
				Invalidate(r);
			}
		}
		WriteUnlock();
	}
}

// IsCursorVisible
bool
HWInterface::IsCursorVisible()
{
	bool visible = true;
	if (ReadLock()) {
		visible = fCursorVisible;
		ReadUnlock();
	}
	return visible;
}

// ObscureCursor
void
HWInterface::ObscureCursor()
{
	if (WriteLock()) {
		if (!fCursorObscured) {
			SetCursorVisible(false);
			fCursorObscured = true;
		}
		WriteUnlock();
	}
}

// MoveCursorTo
void
HWInterface::MoveCursorTo(const float& x, const float& y)
{
	if (WriteLock()) {
		BPoint p(x, y);
		if (p != fCursorLocation) {
			// unhide cursor if it is obscured only
			if (fCursorObscured) {
				// TODO: causes nested lock, which
				// the MultiLocker doesn't actually support?
				SetCursorVisible(true);
			}
			BRect oldFrame = _CursorFrame();
			fCursorLocation = p;
			if (fCursorVisible) {
				// Invalidate and _DrawCursor would not draw
				// anything if the cursor is hidden
				// (invalid cursor frame), but explicitly
				// testing for it here saves us some cycles
				if (fCursorAreaBackup) {
					// means we have a software cursor which we need to draw
					_RestoreCursorArea();
					_DrawCursor(_CursorFrame());
				}
				Invalidate(oldFrame);
				Invalidate(_CursorFrame());
			}
		}
		WriteUnlock();
	}
}

// GetCursorPosition
BPoint
HWInterface::GetCursorPosition()
{
	BPoint location;
	if (ReadLock()) {
		location = fCursorLocation;
		ReadUnlock();
	}
	return location;
}

// SetDragBitmap
void
HWInterface::SetDragBitmap(const ServerBitmap* bitmap,
						   const BPoint& offsetFromCursor)
{
	if (WriteLock()) {
		_AdoptDragBitmap(bitmap, offsetFromCursor);
		WriteUnlock();
	}
}

// DrawingBuffer
RenderingBuffer*
HWInterface::DrawingBuffer() const
{
	if (IsDoubleBuffered())
		return BackBuffer();
	return FrontBuffer();
}

// IsDoubleBuffered
bool
HWInterface::IsDoubleBuffered() const
{
	return fDoubleBuffered;
}

// Invalidate
// * the object needs to be already locked!
status_t
HWInterface::Invalidate(const BRect& frame)
{
	if (IsDoubleBuffered()) {
		return CopyBackToFront(frame);

// TODO: the remaining problem is the immediate wake up of the
// thread carrying out the updates, when I enable it, there
// seems to be a deadlock, but I didn't figure it out yet.
// Maybe the same bug is there without the wakeup, only, triggered
// less often.... scarry, huh?
/*		if (frame.IsValid()) {
			fUpdateExecutor->AddRect(frame);
			return B_OK;
		}
		return B_BAD_VALUE;*/
	} else {
//		_DrawCursor(frame);
	}
	return B_OK;
}

// CopyBackToFront
// * the object must already be locked!
status_t
HWInterface::CopyBackToFront(const BRect& frame)
{
	RenderingBuffer* frontBuffer = FrontBuffer();
	RenderingBuffer* backBuffer = BackBuffer();

	if (!backBuffer || !frontBuffer)
		return B_NO_INIT;

	// we need to mess with the area, but it is const
	BRect area(frame);
	BRect bufferClip(backBuffer->Bounds());

	if (area.IsValid() && area.Intersects(bufferClip)) {

		// make sure we don't copy out of bounds
		area = bufferClip & area;

		uint32 srcBPR = backBuffer->BytesPerRow();
		uint8* src = (uint8*)backBuffer->Bits();

		// convert to integer coordinates
		int32 left = (int32)floorf(area.left);
		int32 top = (int32)floorf(area.top);
		int32 right = (int32)ceilf(area.right);
		int32 bottom = (int32)ceilf(area.bottom);

		// offset to left top pixel in source buffer (always B_RGBA32)
		src += top * srcBPR + left * 4;

		_CopyToFront(src, srcBPR, left, top, right, bottom);
		_DrawCursor(area);

		return B_OK;
	}
	return B_BAD_VALUE;
}


overlay_token
HWInterface::AcquireOverlayChannel()
{
	return NULL;
}


void
HWInterface::ReleaseOverlayChannel(overlay_token token)
{
}


bool
HWInterface::CheckOverlayRestrictions(int32 width, int32 height, color_space colorSpace)
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


// HideSoftwareCursor
bool
HWInterface::HideSoftwareCursor(const BRect& area)
{
	if (fCursorAreaBackup && !fCursorAreaBackup->cursor_hidden) {
		BRect backupArea(fCursorAreaBackup->left,
						 fCursorAreaBackup->top,
						 fCursorAreaBackup->right,
						 fCursorAreaBackup->bottom);
		if (area.Intersects(backupArea)) {
			_RestoreCursorArea();
			return true;
		}
	}
	return false;
}

// HideSoftwareCursor
void
HWInterface::HideSoftwareCursor()
{
	_RestoreCursorArea();
}

// ShowSoftwareCursor
void
HWInterface::ShowSoftwareCursor()
{
	if (fCursorAreaBackup && fCursorAreaBackup->cursor_hidden) {
		_DrawCursor(_CursorFrame());
	}
}


// _DrawCursor
// * default implementation, can be used as fallback or for
//   software cursor
// * area is where we potentially draw the cursor, the cursor
//   might be somewhere else, in which case this function does nothing
void
HWInterface::_DrawCursor(BRect area) const
{
	RenderingBuffer* backBuffer = DrawingBuffer();
	if (!backBuffer || !area.IsValid())
		return;

	BRect cf = _CursorFrame();

	// make sure we don't copy out of bounds
	area = backBuffer->Bounds() & area;

	if (cf.IsValid() && area.Intersects(cf)) {
		// clip to common area
		area = area & cf;

		int32 left = (int32)area.left;
		int32 top = (int32)area.top;
		int32 right = (int32)area.right;
		int32 bottom = (int32)area.bottom;
		int32 width = right - left + 1;
		int32 height = bottom - top + 1;

		// make a bitmap from the backbuffer
		// that has the cursor blended on top of it

		// blending buffer
		uint8* buffer = new uint8[width * height * 4];

		// offset into back buffer
		uint8* src = (uint8*)backBuffer->Bits();
		uint32 srcBPR = backBuffer->BytesPerRow();
		src += top * srcBPR + left * 4;

		// offset into cursor bitmap
		uint8* crs = (uint8*)fCursorAndDragBitmap->Bits();
		uint32 crsBPR = fCursorAndDragBitmap->BytesPerRow();
		// since area is clipped to cf,
		// the diff between top and cf.top is always positive,
		// same for diff between left and cf.left
		crs += (top - (int32)floorf(cf.top)) * crsBPR
				+ (left - (int32)floorf(cf.left)) * 4;

		uint8* dst = buffer;

		if (fCursorAreaBackup && fCursorAreaBackup->buffer) {
			fCursorAreaBackup->cursor_hidden = false;
			// remember which area the backup contains
			fCursorAreaBackup->left = left;
			fCursorAreaBackup->top = top;
			fCursorAreaBackup->right = right;
			fCursorAreaBackup->bottom = bottom;
			uint8* bup = fCursorAreaBackup->buffer;
			uint32 bupBPR = fCursorAreaBackup->bpr;

			// blending and backup of drawing buffer
			for (int32 y = top; y <= bottom; y++) {
				uint8* s = src;
				uint8* c = crs;
				uint8* d = dst;
				uint8* b = bup;
				
				for (int32 x = left; x <= right; x++) {
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
		} else {
			// blending
			for (int32 y = top; y <= bottom; y++) {
				uint8* s = src;
				uint8* c = crs;
				uint8* d = dst;
				for (int32 x = left; x <= right; x++) {
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
		_CopyToFront(buffer, width * 4, left, top, right, bottom);

		delete[] buffer;
	}
}

// _CopyToFront
//
// * source is assumed to be already at the right offset
// * source is assumed to be in B_RGBA32 format
// * location in front buffer is calculated
// * conversion from B_RGBA32 to format of front buffer is taken care of
void
HWInterface::_CopyToFront(uint8* src, uint32 srcBPR,
						  int32 x, int32 y,
						  int32 right, int32 bottom) const
{
	RenderingBuffer* frontBuffer = FrontBuffer();

	uint8* dst = (uint8*)frontBuffer->Bits();
	uint32 dstBPR = frontBuffer->BytesPerRow();

	// transfer, handle colorspace conversion
	switch (frontBuffer->ColorSpace()) {
		case B_RGB32:
		case B_RGBA32: {
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
			} else
printf("nothing to copy\n");
			break;
		}
		// NOTE: on R5, B_RGB24 bitmaps are not supported by DrawBitmap()
		case B_RGB24: {
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
		case B_RGB16: {
			// offset to left top pixel in dest buffer
			dst += y * dstBPR + x * 2;
			int32 left = x;
			// copy
			// TODO: assumes BGR order, does this work on big endian as well?
			for (; y <= bottom; y++) {
				uint8* srcHandle = src;
				uint16* dstHandle = (uint16*)dst;
				for (x = left; x <= right; x++) {
					*dstHandle = (uint16)(((srcHandle[2] & 0xf8) << 8) |
										  ((srcHandle[1] & 0xfc) << 3) |
										  (srcHandle[0] >> 3));
					dstHandle ++;
					srcHandle += 4;
				}
				dst += dstBPR;
				src += srcBPR;
			}
			break;
		}
		case B_RGB15: {
			// offset to left top pixel in dest buffer
			dst += y * dstBPR + x * 2;
			int32 left = x;
			// copy
			// TODO: assumes BGR order, does this work on big endian as well?
			for (; y <= bottom; y++) {
				uint8* srcHandle = src;
				uint16* dstHandle = (uint16*)dst;
				for (x = left; x <= right; x++) {
					*dstHandle = (uint16)(((srcHandle[2] & 0xf8) << 7) |
										  ((srcHandle[1] & 0xf8) << 2) |
										  (srcHandle[0] >> 3));
					dstHandle ++;
					srcHandle += 4;
				}
				dst += dstBPR;
				src += srcBPR;
			}
			break;
		}
		case B_CMAP8: {
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
					index = ((srcHandle[2] & 0xf8) << 7) | ((srcHandle[1] & 0xf8) << 2) | (srcHandle[0] >> 3);
					*dstHandle = colorMap->index_map[index];
					dstHandle ++;
					srcHandle += 4;
				}
				dst += dstBPR;
				src += srcBPR;
			}

			break;
		}
		case B_GRAY8: {
			// offset to left top pixel in dest buffer
			dst += y * dstBPR + x;
			int32 left = x;
			// copy
			// TODO: assumes BGR order, does this work on big endian as well?
			for (; y <= bottom; y++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (x = left; x <= right; x++) {
					*dstHandle = (308 * srcHandle[2] + 600 * srcHandle[1] + 116 * srcHandle[0]) / 1024;
					dstHandle ++;
					srcHandle += 4;
				}
				dst += dstBPR;
				src += srcBPR;
			}
			break;
		}
		default:
			fprintf(stderr, "HWInterface::CopyBackToFront() - unsupported front buffer format!\n");
			break;
	}
}


// _CursorFrame
//
// PRE: the object must be locked
BRect
HWInterface::_CursorFrame() const
{
	BRect frame(0.0, 0.0, -1.0, -1.0);
	if (fCursorAndDragBitmap && fCursorVisible) {
		frame = fCursorAndDragBitmap->Bounds();
		frame.OffsetTo(fCursorLocation - fCursorAndDragBitmap->GetHotSpot());
	}
	return frame;
}

// _RestoreCursorArea
void
HWInterface::_RestoreCursorArea() const
{
	if (fCursorAreaBackup && !fCursorAreaBackup->cursor_hidden) {
		_CopyToFront(fCursorAreaBackup->buffer,
					 fCursorAreaBackup->bpr,
					 fCursorAreaBackup->left,
					 fCursorAreaBackup->top,
					 fCursorAreaBackup->right,
					 fCursorAreaBackup->bottom);

		fCursorAreaBackup->cursor_hidden = true;
	}
}

// _AdoptDragBitmap
void
HWInterface::_AdoptDragBitmap(const ServerBitmap* bitmap, const BPoint& offset)
{
	// TODO: support other colorspaces/convert bitmap
	if (bitmap && !(bitmap->ColorSpace() == B_RGB32
		|| bitmap->ColorSpace() == B_RGBA32)) {
		fprintf(stderr, "HWInterface::_AdoptDragBitmap() - bitmap has yet unsupported colorspace\n");
		return;
	}

	_RestoreCursorArea();
	BRect cursorFrame = _CursorFrame();

	if (fCursorAndDragBitmap && fCursorAndDragBitmap != fCursor) {
		delete fCursorAndDragBitmap;
		fCursorAndDragBitmap = NULL;
	}

	if (bitmap) {
		BRect bitmapFrame = bitmap->Bounds();
		if (fCursor) {
			// put bitmap frame and cursor frame into the same
			// coordinate space (the cursor location is the origin)
			bitmapFrame.OffsetTo(BPoint(-offset.x, -offset.y));

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

			fCursorAndDragBitmap = new ServerCursor(combindedBounds,
													bitmap->ColorSpace(), 0,
													hotspot + shift);

			// clear the combined buffer
			uint8* dst = (uint8*)fCursorAndDragBitmap->Bits();
			uint32 dstBPR = fCursorAndDragBitmap->BytesPerRow();

			memset(dst, 0, fCursorAndDragBitmap->BitsLength());

			// put drag bitmap into combined buffer
			uint8* src = (uint8*)bitmap->Bits();
			uint32 srcBPR = bitmap->BytesPerRow();

			dst += (int32)bitmapFrame.top * dstBPR + (int32)bitmapFrame.left * 4;

			uint32 width = bitmapFrame.IntegerWidth() + 1;
			uint32 height = bitmapFrame.IntegerHeight() + 1;

			for (uint32 y = 0; y < height; y++) {
				memcpy(dst, src, srcBPR);
				dst += dstBPR;
				src += srcBPR;
			}

			// compose cursor into combined buffer
			dst = (uint8*)fCursorAndDragBitmap->Bits();
			dst += (int32)cursorFrame.top * dstBPR + (int32)cursorFrame.left * 4;

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
							uint32 alphaTemp = (65025 - alphaRest * (255 - d[3]));
							uint32 alphaDest = d[3] * alphaRest;
							uint32 alphaSrc = 255 * s[3];
							d[0] = (d[0] * alphaDest + s[0] * alphaSrc) / alphaTemp;
							d[1] = (d[1] * alphaDest + s[1] * alphaSrc) / alphaTemp;
							d[2] = (d[2] * alphaDest + s[2] * alphaSrc) / alphaTemp;
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
		} else {
			fCursorAndDragBitmap = new ServerCursor(bitmap->Bits(),
													bitmapFrame.IntegerWidth() + 1,
													bitmapFrame.IntegerHeight() + 1,
													bitmap->ColorSpace());
			fCursorAndDragBitmap->SetHotSpot(BPoint(-offset.x, -offset.y));
		}
	} else {
		fCursorAndDragBitmap = fCursor;
	}

	Invalidate(cursorFrame);

// NOTE: the EventDispatcher does the reference counting stuff for us
// TODO: You can not simply call Release() on a ServerBitmap like you
// can for a ServerCursor... it could be changed, but there are linking
// troubles with the test environment that need to be solved than.
//	if (fDragBitmap)
//		fDragBitmap->Release();
	fDragBitmap = bitmap;
	fDragBitmapOffset = offset;
//	if (fDragBitmap)
//		fDragBitmap->Acquire();

	delete fCursorAreaBackup;
	fCursorAreaBackup = NULL;

	if (!fCursorAndDragBitmap)
		return;

	if (fCursorAndDragBitmap && !IsDoubleBuffered()) {
		BRect cursorBounds = fCursorAndDragBitmap->Bounds();
		fCursorAreaBackup = new buffer_clip(cursorBounds.IntegerWidth() + 1,
											cursorBounds.IntegerHeight() + 1);
	}
 	_DrawCursor(_CursorFrame());
}




