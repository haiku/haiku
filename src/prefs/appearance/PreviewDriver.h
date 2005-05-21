//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		PreviewDriver.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class to display decorators from regular BApplications
//  
//------------------------------------------------------------------------------
#ifndef _PREVIEWDRIVER_H_
#define _PREVIEWDRIVER_H_

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <GraphicsCard.h>
#include <Cursor.h>
#include <Message.h>
#include <Rect.h>
#include <Region.h>
#include "DisplayDriver.h"

class BBitmap;
class PortLink;
class ServerCursor;
class ColorSet;

class PVView : public BView
{
public:
	PVView(const BRect &bounds);
	~PVView(void);
	void AttachedToWindow(void);
	void DetachedFromWindow(void);
	void Draw(BRect rect);

	BBitmap *viewbmp;
	BWindow *win;
};

class PreviewDriver : public DisplayDriver
{
 public:
								PreviewDriver();
	virtual						~PreviewDriver();

	// when implementing, be sure to call the inherited version
	virtual bool				Initialize();
	virtual void				Shutdown();

	// clipping for all drawing functions
	virtual	void				ConstrainClippingRegion(BRegion* region) {}

	// Graphics calls implemented in DisplayDriver
	virtual	void				CopyRegion(		/*const*/ BRegion* region,
												int32 xOffset,
												int32 yOffset) {}

	virtual	void				CopyRegionList(	BList* list,
												BList* pList,
												int32 rCount,
												BRegion* clipReg) {}

	virtual void				InvertRect(		const BRect &r);

	virtual	void				DrawBitmap(		ServerBitmap *bitmap,
												const BRect &source,
												const BRect &dest,
												const DrawData *d) {}

	virtual	void				FillArc(		const BRect &r,
												const float &angle,
												const float &span,
												const DrawData *d) {}

	virtual	void				FillBezier(		BPoint *pts,
												const DrawData *d) {}

	virtual	void				FillEllipse(	const BRect &r,
												const DrawData *d) {}

	virtual	void				FillPolygon(	BPoint *ptlist,
												int32 numpts,
												const BRect &bounds,
												const DrawData *d) {}

	virtual	void				FillRect(		const BRect &r,
												const RGBColor &color) {}

	virtual	void				FillRect(		const BRect &r,
												const DrawData *d) {}

	virtual	void				FillRegion(		BRegion &r,
												const DrawData *d) {}

	virtual	void				FillRoundRect(	const BRect &r,
												const float &xrad,
												const float &yrad,
												const DrawData *d) {}

	virtual	void				FillShape(		const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawData *d) {}

	virtual	void				FillTriangle(	BPoint *pts,
												const BRect &bounds,
												const DrawData *d) {}

	virtual	void				StrokeArc(		const BRect &r,
												const float &angle,
												const float &span,
												const DrawData *d) {}

	virtual	void				StrokeBezier(	BPoint *pts,
												const DrawData *d) {}

	virtual	void				StrokeEllipse(	const BRect &r,
												const DrawData *d) {}

	// this version used by Decorator
	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												const RGBColor &color) {}

	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												DrawData *d) {}

	virtual void				StrokeLineArray(const int32 &numlines,
												const LineArrayData *data,
												const DrawData *d);

	// this version used by Decorator
	virtual	void				StrokePoint(	const BPoint &pt,
												const RGBColor &color) {}

	virtual	void				StrokePoint(	const BPoint &pt,
												DrawData *d) {}

	virtual	void				StrokePolygon(	BPoint *ptlist,
												int32 numpts,
												const BRect &bounds,
												const DrawData *d,
												bool is_closed=true) {}

	// this version used by Decorator
	virtual	void				StrokeRect(		const BRect &r,
												const RGBColor &color) {}

	virtual	void				StrokeRect(		const BRect &r,
												const DrawData *d) {}

	virtual	void				StrokeRegion(	BRegion &r,
												const DrawData *d) {}

	virtual	void				StrokeRoundRect(const BRect &r,
												const float &xrad,
												const float &yrad,
												const DrawData *d) {}

	virtual	void				StrokeShape(	const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawData *d) {}

	virtual	void				StrokeTriangle(	BPoint *pts,
												const BRect &bounds,
												const DrawData *d) {}

	// Font-related calls
	
	// DrawData is NOT const because this call updates the pen position in the passed DrawData
	virtual	void				DrawString(		const char *string,
												const int32 &length,
												const BPoint &pt,
												DrawData *d) {}

	virtual	void				DrawString(		const char *string,
												const int32 &length,
												const BPoint &pt,
												const RGBColor &color,
												escapement_delta *delta=NULL) {}

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
												 const DrawData *d) {}

	virtual	void				GetEscapements(	const char *string,
												int32 charcount,
												escapement_delta *delta, 
												escapement_delta *escapements,
												escapement_delta *offsets,
												const DrawData *d) {}

	virtual	void				GetEdges(		const char *string,
												int32 charcount,
												edge_info *edgearray,
												const DrawData *d) {}

	virtual	void				GetHasGlyphs(	const char *string,
												int32 charcount,
												bool *hasarray) {}

	virtual	void				GetTruncatedStrings(const char **instrings,
													const int32 &stringcount,
													const uint32 &mode, 
													const float &maxwidth,
													char **outstrings) {}
	
	virtual	void				HideCursor() {}
	virtual	bool				IsCursorHidden() { return false; }
	virtual	void				MoveCursorTo(	const float &x,
												const float &y) {}
	virtual	void				ShowCursor() {}
	virtual	void				ObscureCursor() {}
	virtual	void				SetCursor(ServerCursor *cursor) {}
	virtual	BPoint				GetCursorPosition() { return BPoint(0,0); }
	virtual	bool				IsCursorObscured(bool state) { return false; }
	
	
	// Virtual methods which need to be implemented by each subclass

	// These two will rarely be implemented by subclasses,
	// but it still needs to be possible
	virtual bool				Lock(bigtime_t timeout = B_INFINITE_TIMEOUT);
	virtual void				Unlock();

	// display mode access
	virtual void				SetMode(const display_mode &mode) {}
	virtual	void				GetMode(display_mode *mode) {}
			const display_mode*	DisplayMode()
									{ return &fDisplayMode; }
	
	virtual bool				DumpToFile(const char *path) { return false; }
	virtual ServerBitmap*		DumpToBitmap() { return NULL; }

	virtual status_t			SetDPMSMode(const uint32 &state) { return B_ERROR; }
	virtual uint32				DPMSMode() { return 0; }
	virtual uint32				DPMSCapabilities() { return 0; }
	virtual status_t			GetDeviceInfo(accelerant_device_info *info) { return B_ERROR; }

	virtual status_t			GetModeList(display_mode **mode_list,
											uint32 *count) { return B_ERROR; }

	virtual status_t			GetPixelClockLimits(display_mode *mode,
													uint32 *low,
													uint32 *high) { return B_ERROR; }

	virtual status_t			GetTimingConstraints(display_timing_constraints *dtc) { return B_ERROR; }
	virtual status_t			ProposeMode(display_mode *candidate,
											const display_mode *low,
											const display_mode *high) { return B_ERROR; }

	virtual status_t			WaitForRetrace(bigtime_t timeout = B_INFINITE_TIMEOUT) { return B_ERROR; }
	
			// This is only for the PreviewDriver
			BView *				View(void) { return (BView*)fView; };

 protected:
			void				SetDrawData(const DrawData *d, 
											bool set_font_data);
			
			display_mode		fDisplayMode;
			
			BBitmap 			*fFramebuffer;
			BView 				*fDrawView;
			
			// Region used to track of invalid areas for the 
			// Begin/EndLineArray functions
			BRegion				fArrayRegion;
			PVView 				*fView;
};

extern BRect preview_bounds;

#endif
