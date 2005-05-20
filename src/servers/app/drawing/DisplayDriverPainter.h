//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		DisplayDriver.h
//	Authors:		DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//
//	Description:	Abstract class which handles all graphics output
//					for the server
//  
//------------------------------------------------------------------------------
#ifndef _DISPLAY_DRIVER_PAINTER_H_
#define _DISPLAY_DRIVER_PAINTER_H_

#include "DisplayDriver.h"

class HWInterface;
class Painter;

class DisplayDriverPainter : public DisplayDriver {
 public:
								DisplayDriverPainter();
	virtual						~DisplayDriverPainter();

	// when implementing, be sure to call the inherited version
	virtual bool				Initialize();
	virtual void				Shutdown();

	// clipping for all drawing functions, passing a NULL region
	// will remove any clipping (drawing allowed everywhere)
	virtual	void				ConstrainClippingRegion(BRegion* region);

	// drawing functions
	virtual	void				CopyRegion(		/*const*/ BRegion* region,
												int32 xOffset,
												int32 yOffset);

	virtual	void				CopyRegionList(	BList* list,
												BList* pList,
												int32 rCount,
												BRegion* clipReg);

	virtual void				InvertRect(		const BRect &r);

	virtual	void				DrawBitmap(		ServerBitmap *bitmap,
												const BRect &source,
												const BRect &dest,
												const DrawData *d);

	virtual	void				FillArc(		const BRect &r,
												const float &angle,
												const float &span,
												const DrawData *d);

	virtual	void				FillBezier(		BPoint *pts,
												const DrawData *d);

	virtual	void				FillEllipse(	const BRect &r,
												const DrawData *d);

	virtual	void				FillPolygon(	BPoint *ptlist,
												int32 numpts,
												const BRect &bounds,
												const DrawData *d);

	virtual	void				FillRect(		const BRect &r,
												const RGBColor &color);

	virtual	void				FillRect(		const BRect &r,
												const DrawData *d);

	virtual	void				FillRegion(		BRegion &r,
												const DrawData *d);

	virtual	void				FillRoundRect(	const BRect &r,
												const float &xrad,
												const float &yrad,
												const DrawData *d);

	virtual	void				FillShape(		const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawData *d);

	virtual	void				FillTriangle(	BPoint *pts,
												const BRect &bounds,
												const DrawData *d);

	virtual	void				StrokeArc(		const BRect &r,
												const float &angle,
												const float &span,
												const DrawData *d);

	virtual	void				StrokeBezier(	BPoint *pts,
												const DrawData *d);

	virtual	void				StrokeEllipse(	const BRect &r,
												const DrawData *d);

	// this version used by Decorator
	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												const RGBColor &color);

	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												DrawData *d);

	virtual void				StrokeLineArray(const int32 &numlines,
												const LineArrayData *data,
												const DrawData *d);

	// this version used by Decorator
	virtual	void				StrokePoint(	const BPoint &pt,
												const RGBColor &color);

	virtual	void				StrokePoint(	const BPoint &pt,
												DrawData *d);

	virtual	void				StrokePolygon(	BPoint *ptlist,
												int32 numpts,
												const BRect &bounds,
												const DrawData *d,
												bool is_closed=true);

	// this version used by Decorator
	virtual	void				StrokeRect(		const BRect &r,
												const RGBColor &color);

	virtual	void				StrokeRect(		const BRect &r,
												const DrawData *d);

	virtual	void				StrokeRegion(	BRegion &r,
												const DrawData *d);

	virtual	void				StrokeRoundRect(const BRect &r,
												const float &xrad,
												const float &yrad,
												const DrawData *d);

	virtual	void				StrokeShape(	const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawData *d);

	virtual	void				StrokeTriangle(	BPoint *pts,
												const BRect &bounds,
												const DrawData *d);

	// Font-related calls
	
	// DrawData is NOT const because this call updates the pen position in the passed DrawData
	virtual	void				DrawString(		const char *string,
												const int32 &length,
												const BPoint &pt,
												DrawData *d);

	virtual	void				DrawString(		const char *string,
												const int32 &length,
												const BPoint &pt,
												const RGBColor &color,
												escapement_delta *delta=NULL);

	virtual	float				StringWidth(	const char *string,
												int32 length,
												const DrawData *d);

	virtual	float				StringWidth(	const char *string,
												int32 length,
												const ServerFont &font);

	virtual	float				StringHeight(	const char *string,
												int32 length,
												const DrawData *d);

	virtual	void				GetBoundingBoxes(const char *string,
												 int32 count,
												 font_metric_mode mode, 
												 escapement_delta *delta,
												 BRect *rectarray,
												 const DrawData *d);

	virtual	void				GetEscapements(	const char *string,
												int32 charcount,
												escapement_delta *delta, 
												escapement_delta *escapements,
												escapement_delta *offsets,
												const DrawData *d);

	virtual	void				GetEdges(		const char *string,
												int32 charcount,
												edge_info *edgearray,
												const DrawData *d);

	virtual	void				GetHasGlyphs(	const char *string,
												int32 charcount,
												bool *hasarray);

	virtual	void				GetTruncatedStrings(const char **instrings,
													const int32 &stringcount,
													const uint32 &mode, 
													const float &maxwidth,
													char **outstrings);
	
	// cursor handling		
	virtual	void				HideCursor();
	virtual	bool				IsCursorHidden();
	virtual	void				MoveCursorTo(	const float &x,
												const float &y);
	virtual	void				ShowCursor();
	virtual	void				ObscureCursor();
	virtual	void				SetCursor(ServerCursor *cursor);
			BPoint				GetCursorPosition();
	virtual	bool				IsCursorObscured(bool state);
	

	virtual bool				Lock(bigtime_t timeout = B_INFINITE_TIMEOUT);
	virtual void				Unlock();

	// display mode access
	virtual void				SetMode(const display_mode &mode);

	virtual bool				DumpToFile(const char *path);
	virtual ServerBitmap*		DumpToBitmap();

	virtual status_t			SetDPMSMode(const uint32 &state);
	virtual uint32				DPMSMode();
	virtual uint32				DPMSCapabilities();
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

	virtual status_t			WaitForRetrace(bigtime_t timeout = B_INFINITE_TIMEOUT);

 private:
			BRect				_CopyRect(BRect r, int32 xOffset, int32 yOffset) const;

			void				_CopyRect(uint8* bits,
										  uint32 width, uint32 height, uint32 bpr,
										  int32 xOffset, int32 yOffset) const;

			Painter*			fPainter;
			HWInterface*		fGraphicsCard;
			uint32				fAvailableHWAccleration;
};

#endif // _DISPLAY_DRIVER_PAINTER_H_
