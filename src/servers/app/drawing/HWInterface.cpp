// HWInterface.cpp

#include <stdio.h>
#include <string.h>

#ifndef __HAIKU__
  #include <Screen.h>
#endif

#include "RenderingBuffer.h"
#include "ServerCursor.h"
#include "UpdateQueue.h"

#include "HWInterface.h"

// constructor
HWInterface::HWInterface()
	: BLocker("hw interface lock"),
	  fCursor(NULL),
	  fCursorVisible(true),
	  fCursorLocation(0, 0),
//	  fUpdateExecutor(new UpdateQueue(this))
	  fUpdateExecutor(NULL)
{
}

// destructor
HWInterface::~HWInterface()
{
	delete fCursor;
	delete fUpdateExecutor;
}

// SetCursor
void
HWInterface::SetCursor(ServerCursor* cursor)
{
	if (Lock()) {
		if (fCursor != cursor) {
			BRect oldFrame = _CursorFrame();
			delete fCursor;
			fCursor = cursor;
			Invalidate(oldFrame);
			Invalidate(_CursorFrame());
		}
		Unlock();
	}
}

// SetCursorVisible
void
HWInterface::SetCursorVisible(bool visible)
{
	if (Lock()) {
		if (fCursorVisible != visible) {
			fCursorVisible = visible;
			Invalidate(_CursorFrame());
		}
		Unlock();
	}
}

// IsCursorVisible
bool
HWInterface::IsCursorVisible()
{
	bool visible = true;
	if (Lock()) {
		visible = fCursorVisible;
		Unlock();
	}
	return visible;
}

// MoveCursorTo
void
HWInterface::MoveCursorTo(const float& x, const float& y)
{
	if (Lock()) {
		BPoint p(x, y);
		if (p != fCursorLocation) {
			BRect oldFrame = _CursorFrame();
			fCursorLocation = p;
			Invalidate(oldFrame);
			Invalidate(_CursorFrame());
		}
		Unlock();
	}
}

// GetCursorPosition
BPoint
HWInterface::GetCursorPosition()
{
	BPoint location;
	if (Lock()) {
		location = fCursorLocation;
		Unlock();
	}
	return location;
}

// Invalidate
// * the object needs to be already locked!
status_t
HWInterface::Invalidate(const BRect& frame)
{
	return CopyBackToFront(frame);;

// TODO: get this working, the locking in the DisplayDriverPainter needs
// to be based on locking this object, which essentially means the access
// to the back buffer is locked, or more precise the access to the invalid
// region scheduled to be copied to the front buffer
//	fUpdateExecutor->AddRect(frame);
//	return B_OK;
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
	BRect bufferClip(0.0, 0.0, backBuffer->Width() - 1, backBuffer->Height() - 1);

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
					memcpy(dst, src, bytes);
					dst += dstBPR;
					src += srcBPR;
				}
			}
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
// TODO: make this work on Haiku, the problem is only
// the rgb_color->index mapping, there is an implementation
// in Bitmap.cpp, maybe it needs to be moved to a public
// place...
#ifndef __HAIKU__
			// offset to left top pixel in dest buffer
			dst += y * dstBPR + x;
			int32 left = x;
			// copy
			// TODO: using BScreen will not be an option in the
			// final implementation, will it? The BBitmap implementation
			// has a class that handles this, something so useful
			// should be moved to a more public place.
			// TODO: assumes BGR order again
			BScreen screen;
			for (; y <= bottom; y++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (x = left; x <= right; x++) {
					*dstHandle = screen.IndexForColor(srcHandle[2],
													  srcHandle[1],
													  srcHandle[0]);
					dstHandle ++;
					srcHandle += 4;
				}
				dst += dstBPR;
				src += srcBPR;
			}
#endif // __HAIKU__
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
			fprintf(stderr, "ViewHWInterface::CopyBackToFront() - unsupported front buffer format!\n");
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
	if (fCursor && fCursorVisible) {
		frame = fCursor->Bounds();
		frame.OffsetTo(fCursorLocation - fCursor->GetHotSpot());
	}
	return frame;
}



