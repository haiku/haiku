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

#include "DisplayDriverPainter.h"

// constructor
DisplayDriverPainter::DisplayDriverPainter()
	: DisplayDriver()
{
}

// destructor
DisplayDriverPainter::~DisplayDriverPainter()
{
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

		fPainter.InvertRect(r);

		Unlock();
	}
}

// DrawBitmap
void
DisplayDriverPainter::DrawBitmap(BRegion *region, ServerBitmap *bitmap,
								 const BRect &source, const BRect &dest,
								 const DrawData *d)
{
	if (region && Lock()) {

		fPainter.ConstrainClipping(*region);
		fPainter.SetDrawData(d);
		fPainter.DrawBitmap(bitmap, source, dest);

		Unlock();
	}
}

// CopyRegionList
void
DisplayDriverPainter::CopyRegionList(BList* list, BList* pList,
									 int32 rCount, BRegion* clipReg)
{
	fprintf(stdout, "DisplayDriverPainter::CopyRegionList()\b");
}

// FillArc
void
DisplayDriverPainter::FillArc(const BRect &r, const float &angle,
							  const float &span, const DrawData *d)
{
	if (region && Lock()) {

		fPainter.SetDrawData(d);
		fPainter.DrawBitmap(bitmap, source, dest);

		Unlock();
	}
}

// FillBezier
void
DisplayDriverPainter::FillBezier(BPoint *pts, const DrawData *d)
{
}

// FillEllipse
void
DisplayDriverPainter::FillEllipse(const BRect &r, const DrawData *d)
{
}

// FillPolygon
void
DisplayDriverPainter::FillPolygon(BPoint *ptlist, int32 numpts,
								  const BRect &bounds, const DrawData *d)
{
}

// FillRect
void
DisplayDriverPainter::FillRect(const BRect &r, const RGBColor &color)
{
}

// FillRect
void
DisplayDriverPainter::FillRect(const BRect &r, const DrawData *d)
{
}

// FillRegion
void
DisplayDriverPainter::FillRegion(BRegion& r, const DrawData *d)
{
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
}

// StrokeLine
void
DisplayDriverPainter::StrokeLine(const BPoint &start, const BPoint &end, const DrawData *d)
{
}

// used by decorator
//
// StrokePoint
void
DisplayDriverPainter::StrokePoint(const BPoint& pt, const RGBColor &color)
{
}

// StrokePoint
void
DisplayDriverPainter::StrokePoint(const BPoint& pt, const DrawData *d)
{
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
}

// StrokeRect
void
DisplayDriverPainter::StrokeRect(const BRect &r, const DrawData *d)
{
}

// StrokeRegion
void
DisplayDriverPainter::StrokeRegion(BRegion& r, const DrawData *d)
{
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
}

// StringWidth
float
DisplayDriverPainter::StringWidth(const char *string, int32 length,
								  const DrawData *d)
{

}

// StringHeight
float
DisplayDriverPainter::StringHeight(const char *string, int32 length,
								   const DrawData *d)
{
}

// GetBoundingBoxes
void
DisplayDriverPainter::GetBoundingBoxes(const char *string, int32 count, 
									   font_metric_mode mode,
									   escapement_delta *delta,
									   BRect *rectarray, const DrawData *d)
{
}

// GetEscapements
void
DisplayDriverPainter::GetEscapements(const char *string, int32 charcount, 
									 escapement_delta *delta,
									 escapement_delta *escapements,
									 escapement_delta *offsets,
									 const DrawData *d)
{
}

// GetEdges
void
DisplayDriverPainter::GetEdges(const char *string, int32 charcount,
							   edge_info *edgearray, const DrawData *d)
{
}

// GetHasGlyphs
void DisplayDriverPainter::GetHasGlyphs(const char *string, int32 charcount,
										bool *hasarray)
{
}

// GetTruncatedStrings
void
DisplayDriverPainter::GetTruncatedStrings(const char **instrings,
										  const int32 &stringcount, 
										  const uint32 &mode,
										  const float &maxwidth,
										  char **outstrings)
{
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
}

// GetDeviceInfo
status_t
DisplayDriverPainter::GetDeviceInfo(accelerant_device_info *info)
{
	return B_ERROR;
}

// GetModeList
status_t
DisplayDriverPainter::GetModeList(display_mode **mode_list, uint32 *count)
{
	return B_UNSUPPORTED;
}

// GetPixelClockLimits
status_t DisplayDriverPainter::GetPixelClockLimits(display_mode *mode,
												   uint32 *low,
												   uint32 *high)
{
	return B_UNSUPPORTED;
}

// GetTimingConstraints
status_t
DisplayDriverPainter::GetTimingConstraints(display_timing_constraints *dtc)
{
	return B_UNSUPPORTED;
}

// ProposeMode
status_t
DisplayDriverPainter::ProposeMode(display_mode *candidate,
								  const display_mode *low,
								  const display_mode *high)
{
	return B_UNSUPPORTED;
}

// WaitForRetrace
status_t
DisplayDriverPainter::WaitForRetrace(bigtime_t timeout)
{
	return B_UNSUPPORTED;
}

// _GetCursor
ServerCursor*
DisplayDriverPainter::_GetCursor()
{
	Lock();
	ServerCursor *c = fCursorHandler.GetCursor();
	Unlock();
	
	return c;
}

// HLinePatternThick
void
DisplayDriverPainter::HLinePatternThick(int32 x1, int32 x2, int32 y)
{
}

// VLinePatternThick
void
DisplayDriverPainter::VLinePatternThick(int32 x1, int32 x2, int32 y)
{
}

// SetThickPatternPixel
void
DisplayDriverPainter::SetThickPatternPixel(int x, int y)
{
}

// AcquireBuffer
bool
DisplayDriverPainter::AcquireBuffer(FBBitmap *bmp)
{
	return false;
}

// ReleaseBuffer
void
DisplayDriverPainter::ReleaseBuffer()
{
}

// Blit
void
DisplayDriverPainter::Blit(const BRect &src, const BRect &dest,
						   const DrawData *d)
{
}

// FillSolidRect
void
DisplayDriverPainter::FillSolidRect(const BRect &rect, const RGBColor &color)
{
}

// FillPatternRect
void
DisplayDriverPainter::FillPatternRect(const BRect &rect, const DrawData *d)
{
}

// StrokeSolidLine
void
DisplayDriverPainter::StrokeSolidLine(int32 x1, int32 y1,
									  int32 x2, int32 y2, const RGBColor &color)
{
}

// StrokePatternLine
void
DisplayDriverPainter::StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d)
{
}

// StrokeSolidRect
void
DisplayDriverPainter::StrokeSolidRect(const BRect &rect, const RGBColor &color)
{
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
}

// ConstrainClippingRegion
void DisplayDriverPainter::ConstrainClippingRegion(BRegion *reg)
{
}
