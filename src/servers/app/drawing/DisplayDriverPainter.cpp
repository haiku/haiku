//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		DisplayDriverPainter.cpp
//	Author:			Stephan Aßmus <superstippi@gmx.de>
//	Description:	Implementation of DisplayDriver based on top of Painter
//  
//------------------------------------------------------------------------------
#include <stdio.h>

#include "Painter.h"
#include "RenderingBuffer.h"
#include "ViewHWInterface.h"

#include "DisplayDriverPainter.h"

// constructor
DisplayDriverPainter::DisplayDriverPainter()
	: DisplayDriver(),
	  fPainter(new Painter()),
	  fGraphicsCard(new ViewHWInterface())
{
}

// destructor
DisplayDriverPainter::~DisplayDriverPainter()
{
	delete fGraphicsCard;
}

// Initialize
bool
DisplayDriverPainter::Initialize()
{
	return DisplayDriver::Initialize();
}

// Shutdown
void
DisplayDriverPainter::Shutdown()
{
	DisplayDriver::Shutdown();
}

// CopyBits
void
DisplayDriverPainter::CopyBits(const BRect &src, const BRect &dest,
							   const DrawData *d)
{
/*	if(!d)
		return;
	
	Lock();
	
	if(fCursorHandler.IntersectsCursor(dest))
		fCursorHandler.DriverHide();
	Blit(src,dest,d);
	fCursorHandler.DriverShow();
	Unlock();*/
}

// CopyRegion
void
DisplayDriverPainter::CopyRegion(BRegion *src, const BPoint &lefttop)
{
}

// InvertRect
void
DisplayDriverPainter::InvertRect(const BRect &r)
{
	if (Lock()) {

		fPainter->InvertRect(r);
		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// DrawBitmap
void
DisplayDriverPainter::DrawBitmap(BRegion *region, ServerBitmap *bitmap,
								 const BRect &source, const BRect &dest,
								 const DrawData *d)
{
	// catch PicturePlayer giving us a NULL region
	if (!region)
		return;

	if (Lock()) {

		fPainter->ConstrainClipping(*region);
		fPainter->SetDrawData(d);
		fPainter->DrawBitmap(bitmap, source, dest);

		fGraphicsCard->Invalidate(dest);

		Unlock();
	}
}

// used to move windows arround on screen
//
// CopyRegionList
void
DisplayDriverPainter::CopyRegionList(BList* list, BList* pList,
									 int32 rCount, BRegion* clipReg)
{
	if (!clipReg || !Lock())
		return;

// This is the same implementation as in DisplayDriverImpl for now

// since we're not using painter.... this won't do much good, will it?
//	fPainter->ConstrainClipping(*clipReg);

	RenderingBuffer* bmp = fGraphicsCard->BackBuffer();
	
	uint32		bytesPerPixel	= bmp->BytesPerRow() / bmp->Width();
	BList		rectList;
	int32		i, k;
	uint8		*bitmapBits = (uint8*)bmp->Bits();
	int32		Bwidth		= bmp->Width();
	int32		Bheight		= bmp->Height();

	BRect updateRect;

	for (k = 0; k < rCount; k++) {
		BRegion* reg = (BRegion*)list->ItemAt(k);

		int32 rectCount = reg->CountRects();
		for(i=0; i < rectCount; i++) {
			BRect		r		= reg->RectAt(i);
			uint8		*rectCopy;
			uint8		*srcAddress;
			uint8		*destAddress;
			int32		firstRow, lastRow;
			int32		firstCol, lastCol;
			int32		copyLength;
			int32		copyRows;
	
			firstRow	= (int32)(r.top < 0? 0: r.top);
			lastRow		= (int32)(r.bottom > (Bheight-1)? (Bheight-1): r.bottom);
			firstCol	= (int32)(r.left < 0? 0: r.left);
			lastCol		= (int32)(r.right > (Bwidth-1)? (Bwidth-1): r.right);
			copyLength	= (lastCol - firstCol + 1) < 0? 0: (lastCol - firstCol + 1);
			copyRows	= (lastRow - firstRow + 1) < 0? 0: (lastRow - firstRow + 1);
	
			rectCopy	= (uint8*)malloc(copyLength * copyRows * bytesPerPixel);
	
			srcAddress	= bitmapBits + (((firstRow) * Bwidth + firstCol) * bytesPerPixel);
			destAddress	= rectCopy;
	
			for (int32 j = 0; j < copyRows; j++) {
				uint8		*destRowAddress	= destAddress + (j * copyLength * bytesPerPixel);
				uint8		*srcRowAddress	= srcAddress + (j * Bwidth * bytesPerPixel);
				memcpy(destRowAddress, srcRowAddress, copyLength * bytesPerPixel );
			}
		
			rectList.AddItem(rectCopy);
		}
	}

	int32 item = 0;
	for(k = 0; k < rCount; k++) {
		BRegion* reg = (BRegion*)list->ItemAt(k);
	
		int32 rectCount = reg->CountRects();
		for(i=0; i < rectCount; i++)
		{
			BRect		r		= reg->RectAt(i);
			uint8		*rectCopy;
			uint8		*srcAddress;
			uint8		*destAddress;
			int32		firstRow, lastRow;
			int32		firstCol, lastCol;
			int32		copyLength, copyLength2;
			int32		copyRows, copyRows2;
	
			firstRow	= (int32)(r.top < 0? 0: r.top);
			lastRow		= (int32)(r.bottom > (Bheight-1)? (Bheight-1): r.bottom);
			firstCol	= (int32)(r.left < 0? 0: r.left);
			lastCol		= (int32)(r.right > (Bwidth-1)? (Bwidth-1): r.right);
			copyLength	= (lastCol - firstCol + 1) < 0? 0: (lastCol - firstCol + 1);
			copyRows	= (lastRow - firstRow + 1) < 0? 0: (lastRow - firstRow + 1);
	
			rectCopy	= (uint8*)rectList.ItemAt(item++);
	
			srcAddress	= rectCopy;
	
			r.Set(firstCol, firstRow, lastCol, lastRow);
			r.OffsetBy( *((BPoint*)pList->ItemAt(k%rCount)) );

			// keep track of invalid area
			updateRect = updateRect.IsValid() ? updateRect | r : r;

			firstRow	= (int32)(r.top < 0? 0: r.top);
			lastRow		= (int32)(r.bottom > (Bheight-1)? (Bheight-1): r.bottom);
			firstCol	= (int32)(r.left < 0? 0: r.left);
			lastCol		= (int32)(r.right > (Bwidth-1)? (Bwidth-1): r.right);
			copyLength2	= (lastCol - firstCol + 1) < 0? 0: (lastCol - firstCol + 1);
			copyRows2	= (lastRow - firstRow + 1) < 0? 0: (lastRow - firstRow + 1);
	
			destAddress	= bitmapBits + (((firstRow) * Bwidth + firstCol) * bytesPerPixel);
	
			int32		minLength	= copyLength < copyLength2? copyLength: copyLength2;
			int32		minRows		= copyRows < copyRows2? copyRows: copyRows2;
	
			for (int32 j = 0; j < minRows; j++)
			{
				uint8		*destRowAddress	= destAddress + (j * Bwidth * bytesPerPixel);
				uint8		*srcRowAddress	= srcAddress + (j * copyLength * bytesPerPixel);
				memcpy(destRowAddress, srcRowAddress, minLength * bytesPerPixel );
			}
		}
	}

	int32 count = rectList.CountItems();
	for (i = 0; i < count; i++) {
		if (void* rectCopy = rectList.ItemAt(i))
			free(rectCopy);
	}

	fGraphicsCard->Invalidate(updateRect);

	Unlock();
}

// FillArc
void
DisplayDriverPainter::FillArc(const BRect &r, const float &angle,
							  const float &span, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		float xRadius = r.Width() / 2.0;
		float yRadius = r.Width() / 2.0;
		BPoint center(r.left + xRadius,
					  r.top + yRadius);

		fPainter->FillArc(center, xRadius, yRadius, angle, span);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// FillBezier
void
DisplayDriverPainter::FillBezier(BPoint *pts, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		fPainter->FillBezier(pts);

		// TODO: Invalidate

		Unlock();
	}
}

// FillEllipse
void
DisplayDriverPainter::FillEllipse(const BRect &r, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		float xRadius = r.Width() / 2.0;
		float yRadius = r.Width() / 2.0;
		BPoint center(r.left + xRadius,
					  r.top + yRadius);

		fPainter->FillEllipse(center, xRadius, yRadius);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// FillPolygon
void
DisplayDriverPainter::FillPolygon(BPoint *ptlist, int32 numpts,
								  const BRect &bounds, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		fPainter->FillPolygon(ptlist, numpts);

		fGraphicsCard->Invalidate(bounds);

		Unlock();
	}
}

// FillRect
void
DisplayDriverPainter::FillRect(const BRect &r, const RGBColor &color)
{
	if (Lock()) {

		fPainter->SetHighColor(color);
		fPainter->FillRect(r);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// FillRect
void
DisplayDriverPainter::FillRect(const BRect &r, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		fPainter->FillRect(r);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// FillRegion
void
DisplayDriverPainter::FillRegion(BRegion& r, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		BRect invalid = r.RectAt(0);
		fPainter->FillRect(invalid);

		int32 count = r.CountRects();
		for (int32 i = 1; i < count; i++) {
			fPainter->FillRect(r.RectAt(i));
			invalid = invalid | r.RectAt(i);
		}

		fGraphicsCard->Invalidate(invalid);

		Unlock();
	}
}

// FillRoundRect
void
DisplayDriverPainter::FillRoundRect(const BRect &r, const float &xrad, const float &yrad, const DrawData *d)
{
}

// FillShape
void
DisplayDriverPainter::FillShape(const BRect &bounds,
								const int32 &opcount, const int32 *oplist, 
								const int32 &ptcount, const BPoint *ptlist,
								const DrawData *d)
{
}

// FillTriangle
void
DisplayDriverPainter::FillTriangle(BPoint *pts, const BRect &bounds,
								   const DrawData *d)
{
}

// StrokeArc
void
DisplayDriverPainter::StrokeArc(const BRect &r, const float &angle,
								const float &span, const DrawData *d)
{
}

// StrokeBezier
void
DisplayDriverPainter::StrokeBezier(BPoint *pts, const DrawData *d)
{
}

// StrokeEllipse
void
DisplayDriverPainter::StrokeEllipse(const BRect &r, const DrawData *d)
{
}

// StrokeLine
void
DisplayDriverPainter::StrokeLine(const BPoint &start, const BPoint &end, const RGBColor &color)
{
	if (Lock()) {

		fPainter->SetHighColor(color);
		fPainter->StrokeLine(start, end);

		BRect r(start, end);
		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// StrokeLine
void
DisplayDriverPainter::StrokeLine(const BPoint &start, const BPoint &end, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		fPainter->StrokeLine(start, end);

		BRect r(start, end);
		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// used by decorator
//
// StrokePoint
void
DisplayDriverPainter::StrokePoint(const BPoint& pt, const RGBColor &color)
{
	StrokeLine(pt, pt, color);
}

// StrokePoint
void
DisplayDriverPainter::StrokePoint(const BPoint& pt, const DrawData *d)
{
	StrokeLine(pt, pt, d);
}

// StrokePolygon
void
DisplayDriverPainter::StrokePolygon(BPoint *ptlist, int32 numpts,
									const BRect &bounds, const DrawData *d,
									bool is_closed)
{
}

// StrokeRect
void
DisplayDriverPainter::StrokeRect(const BRect &r, const RGBColor &color)
{
	if (Lock()) {

		fPainter->SetHighColor(color);
		fPainter->StrokeRect(r);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// StrokeRect
void
DisplayDriverPainter::StrokeRect(const BRect &r, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);
		fPainter->StrokeRect(r);

		fGraphicsCard->Invalidate(r);

		Unlock();
	}
}

// StrokeRegion
void
DisplayDriverPainter::StrokeRegion(BRegion& r, const DrawData *d)
{
	if (Lock()) {

		fPainter->SetDrawData(d);

		BRect invalid = r.RectAt(0);
		fPainter->StrokeRect(invalid);

		int32 count = r.CountRects();
		for (int32 i = 1; i < count; i++) {
			fPainter->StrokeRect(r.RectAt(i));
			invalid = invalid | r.RectAt(i);
		}

		fGraphicsCard->Invalidate(invalid);

		Unlock();
	}
}

// StrokeRoundRect
void
DisplayDriverPainter::StrokeRoundRect(const BRect &r, const float &xrad,
									  const float &yrad, const DrawData *d)
{
}

// StrokeShape
void
DisplayDriverPainter::StrokeShape(const BRect &bounds, const int32 &opcount,
								  const int32 *oplist, const int32 &ptcount,
								  const BPoint *ptlist, const DrawData *d)
{
}

// StrokeTriangle
void
DisplayDriverPainter::StrokeTriangle(BPoint *pts, const BRect &bounds,
									 const DrawData *d)
{
}

// DrawString
void
DisplayDriverPainter::DrawString(const char *string, const int32 &length,
								 const BPoint &pt, const RGBColor &color,
								 escapement_delta *delta)
{
	// what - without any clipping?!?
	DrawData d;
	d.highcolor=color;
	
	if(delta)
		d.edelta=*delta;
	DrawString(string,length,pt,&d);
}

// DrawString
void
DisplayDriverPainter::DrawString(const char *string, const int32 &length,
								 const BPoint &pt, DrawData *d)
{
	if (Lock()) {
		fPainter->SetDrawData(d);

		BRect boundingBox = fPainter->BoundingBox(string, length, pt);

		fPainter->DrawString(string, length, pt);

		fGraphicsCard->Invalidate(boundingBox);

		Unlock();
	}
}

// StringWidth
float
DisplayDriverPainter::StringWidth(const char *string, int32 length,
								  const DrawData *d)
{
	float width = 0.0;
	if (Lock()) {
		fPainter->SetDrawData(d);
		BPoint dummy(0.0, 0.0);
		width = fPainter->BoundingBox(string, length, dummy).Width();
		Unlock();
	}
	return width;
}

// StringHeight
float
DisplayDriverPainter::StringHeight(const char *string, int32 length,
								   const DrawData *d)
{
	float height = 0.0;
	if (Lock()) {
		fPainter->SetDrawData(d);
		BPoint dummy(0.0, 0.0);
		height = fPainter->BoundingBox(string, length, dummy).Height();
		Unlock();
	}
	return height;
}

// GetBoundingBoxes
void
DisplayDriverPainter::GetBoundingBoxes(const char *string, int32 count, 
									   font_metric_mode mode,
									   escapement_delta *delta,
									   BRect *rectarray, const DrawData *d)
{
	// ?!? each glyph or what?
	printf("DisplayDriverPainter::GetBoundingBoxes()\n");
}

// GetEscapements
void
DisplayDriverPainter::GetEscapements(const char *string, int32 charcount, 
									 escapement_delta *delta,
									 escapement_delta *escapements,
									 escapement_delta *offsets,
									 const DrawData *d)
{
	printf("DisplayDriverPainter::GetEscapements()\n");
}

// GetEdges
void
DisplayDriverPainter::GetEdges(const char *string, int32 charcount,
							   edge_info *edgearray, const DrawData *d)
{
	printf("DisplayDriverPainter::GetEdges()\n");
}

// GetHasGlyphs
void DisplayDriverPainter::GetHasGlyphs(const char *string, int32 charcount,
										bool *hasarray)
{
	printf("DisplayDriverPainter::GetHasGlyphs()\n");
}

// GetTruncatedStrings
void
DisplayDriverPainter::GetTruncatedStrings(const char **instrings,
										  const int32 &stringcount, 
										  const uint32 &mode,
										  const float &maxwidth,
										  char **outstrings)
{
	printf("DisplayDriverPainter::GetTruncatedStrings()\n");
}

// SetMode
void
DisplayDriverPainter::SetMode(const display_mode &mode)
{
	if (Lock() && fGraphicsCard->SetMode(mode) >= B_OK) {
		fPainter->AttachToBuffer(fGraphicsCard->BackBuffer());
		DisplayDriver::SetMode(mode);
		Unlock();
	}
}

// DumpToFile
bool
DisplayDriverPainter::DumpToFile(const char *path)
{
	return false;
}

// DumpToBitmap
ServerBitmap*
DisplayDriverPainter::DumpToBitmap()
{
	return NULL;
}

// StrokeLineArray
void
DisplayDriverPainter::StrokeLineArray(const int32 &numlines,
									  const LineArrayData *linedata,
									  const DrawData *d)
{
	// I'm suspecting this is not used
	printf("DisplayDriverPainter::StrokeLineArray()\n");
}

// SetDPMSMode
status_t
DisplayDriverPainter::SetDPMSMode(const uint32 &state)
{
	return B_ERROR;
}

// DPMSMode
uint32
DisplayDriverPainter::DPMSMode() const
{
	return 0;
}

// DPMSCapabilities
uint32
DisplayDriverPainter::DPMSCapabilities() const
{
	return 0;
}

// GetDeviceInfo
status_t
DisplayDriverPainter::GetDeviceInfo(accelerant_device_info *info)
{
	return fGraphicsCard->GetDeviceInfo(info);
}

// GetModeList
status_t
DisplayDriverPainter::GetModeList(display_mode **mode_list, uint32 *count)
{
	return fGraphicsCard->GetModeList(mode_list, count);
}

// GetPixelClockLimits
status_t DisplayDriverPainter::GetPixelClockLimits(display_mode *mode,
												   uint32 *low,
												   uint32 *high)
{
	return fGraphicsCard->GetPixelClockLimits(mode, low, high);
}

// GetTimingConstraints
status_t
DisplayDriverPainter::GetTimingConstraints(display_timing_constraints *dtc)
{
	return fGraphicsCard->GetTimingConstraints(dtc);
}

// ProposeMode
status_t
DisplayDriverPainter::ProposeMode(display_mode *candidate,
								  const display_mode *low,
								  const display_mode *high)
{
	return fGraphicsCard->ProposeMode(candidate, low, high);
}

// WaitForRetrace
status_t
DisplayDriverPainter::WaitForRetrace(bigtime_t timeout)
{
	return fGraphicsCard->WaitForRetrace(timeout);
}

// CopyBitmap
void
DisplayDriverPainter::CopyBitmap(ServerBitmap *bitmap,
								 const BRect &source, const BRect &dest,
								 const DrawData *d)
{
}

// CopyToBitmap
void DisplayDriverPainter::CopyToBitmap(ServerBitmap *target,
										const BRect &source)
{
}

// Invalidate
void DisplayDriverPainter::Invalidate(const BRect &r)
{
	if (Lock()) {
// ?!? Does this implementation need this?
		fGraphicsCard->Invalidate(r);
		Unlock();
	}
}

// ConstrainClippingRegion
void DisplayDriverPainter::ConstrainClippingRegion(BRegion *region)
{
	if (region && Lock()) {
		fPainter->ConstrainClipping(*region);
		Unlock();
	}
}
