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
#ifndef _DISPLAY_DRIVER_H_
#define _DISPLAY_DRIVER_H_

#include <Accelerant.h>
#include <Font.h>
#include <Locker.h>

#include "CursorHandler.h"

class BPoint;
class BRect;
class BRegion;

class DrawData;
class RGBColor;
class ServerBitmap;
class ServerCursor;

/*!
	\brief Data structure for passing cursor information to hardware drivers.
*/
/*typedef struct
{
	uchar *xormask, *andmask;
	int32 width, height;
	int32 hotx, hoty;

} cursor_data;*/

typedef struct
{
	BPoint pt1;
	BPoint pt2;
	rgb_color color;

} LineArrayData;

/*
#ifndef HOOK_DEFINE_CURSOR

#define HOOK_DEFINE_CURSOR		0
#define HOOK_MOVE_CURSOR		1
#define HOOK_SHOW_CURSOR		2
#define HOOK_DRAW_LINE_8BIT		3
#define HOOK_DRAW_LINE_16BIT	12
#define HOOK_DRAW_LINE_32BIT	4
#define HOOK_DRAW_RECT_8BIT		5
#define HOOK_DRAW_RECT_16BIT	13
#define HOOK_DRAW_RECT_32BIT	6
#define HOOK_BLIT				7
#define HOOK_DRAW_ARRAY_8BIT	8
#define HOOK_DRAW_ARRAY_16BIT	14	// Not implemented in current R5 drivers
#define HOOK_DRAW_ARRAY_32BIT	9
#define HOOK_SYNC				10
#define HOOK_INVERT_RECT		11

#endif

class DisplayDriver;

typedef void (DisplayDriver::* SetPixelFuncType)(int x, int y);
typedef void (DisplayDriver::* SetHorizontalLineFuncType)(int xstart, int xend, int y);
typedef void (DisplayDriver::* SetVerticalLineFuncType)(int x, int ystart, int yend);
typedef void (DisplayDriver::* SetRectangleFuncType)(int left, int top, int right, int bottom);
*/

/*!
	\class DisplayDriver DisplayDriver.h
	\brief Mostly abstract class which handles all graphics output for the server.

	The DisplayDriver is called in order to handle all messiness associated with
	a particular rendering context, such as the screen, and the methods to
	handle organizing information related to it along with writing to the context
	itself.
	
	While all virtual functions are technically optional, the default versions
	do very little, so implementing them all more or less required.
*/

class DisplayDriver {
 public:
								DisplayDriver();
	virtual						~DisplayDriver();

	// when implementing, be sure to call the inherited version
	virtual bool				Initialize();
	virtual void				Shutdown();

	// Graphics calls implemented in DisplayDriver
	virtual	void				CopyBits(		const BRect &src,
												const BRect &dest,
												const DrawData *d) = 0;

	virtual	void				CopyRegion(		BRegion *src,
												const BPoint &lefttop) = 0;

	virtual	void				CopyRegionList(	BList* list,
												BList* pList,
												int32 rCount,
												BRegion* clipReg) = 0;

	virtual void				InvertRect(		const BRect &r) = 0;

	virtual	void				DrawBitmap(		BRegion *region,
												ServerBitmap *bitmap,
												const BRect &source,
												const BRect &dest,
												const DrawData *d) = 0;

	virtual	void				FillArc(		const BRect &r,
												const float &angle,
												const float &span,
												const DrawData *d) = 0;

	virtual	void				FillBezier(		BPoint *pts,
												const DrawData *d) = 0;

	virtual	void				FillEllipse(	const BRect &r,
												const DrawData *d) = 0;

	virtual	void				FillPolygon(	BPoint *ptlist,
												int32 numpts,
												const BRect &bounds,
												const DrawData *d) = 0;

	virtual	void				FillRect(		const BRect &r,
												const RGBColor &color) = 0;

	virtual	void				FillRect(		const BRect &r,
												const DrawData *d) = 0;

	virtual	void				FillRegion(		BRegion &r,
												const DrawData *d) = 0;

	virtual	void				FillRoundRect(	const BRect &r,
												const float &xrad,
												const float &yrad,
												const DrawData *d) = 0;

	virtual	void				FillShape(		const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawData *d) = 0;

	virtual	void				FillTriangle(	BPoint *pts,
												const BRect &bounds,
												const DrawData *d) = 0;

	virtual	void				StrokeArc(		const BRect &r,
												const float &angle,
												const float &span,
												const DrawData *d) = 0;

	virtual	void				StrokeBezier(	BPoint *pts,
												const DrawData *d) = 0;

	virtual	void				StrokeEllipse(	const BRect &r,
												const DrawData *d) = 0;

	// this version used by Decorator
	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												const RGBColor &color) = 0;

	virtual	void				StrokeLine(		const BPoint &start,
												const BPoint &end,
												DrawData *d) = 0;

	// this version used by Decorator
	virtual	void				StrokePoint(	const BPoint &pt,
												const RGBColor &color) = 0;

	virtual	void				StrokePoint(	const BPoint &pt,
												DrawData *d) = 0;

	virtual	void				StrokePolygon(	BPoint *ptlist,
												int32 numpts,
												const BRect &bounds,
												const DrawData *d,
												bool is_closed=true) = 0;

	// this version used by Decorator
	virtual	void				StrokeRect(		const BRect &r,
												const RGBColor &color) = 0;

	virtual	void				StrokeRect(		const BRect &r,
												const DrawData *d) = 0;

	virtual	void				StrokeRegion(	BRegion &r,
												const DrawData *d) = 0;

	virtual	void				StrokeRoundRect(const BRect &r,
												const float &xrad,
												const float &yrad,
												const DrawData *d) = 0;

	virtual	void				StrokeShape(	const BRect &bounds,
												const int32 &opcount,
												const int32 *oplist, 
												const int32 &ptcount,
												const BPoint *ptlist,
												const DrawData *d) = 0;

	virtual	void				StrokeTriangle(	BPoint *pts,
												const BRect &bounds,
												const DrawData *d) = 0;

	// Font-related calls
	
	// DrawData is NOT const because this call updates the pen position in the passed DrawData
	virtual	void				DrawString(		const char *string,
												const int32 &length,
												const BPoint &pt,
												DrawData *d) = 0;

	virtual	void				DrawString(		const char *string,
												const int32 &length,
												const BPoint &pt,
												const RGBColor &color,
												escapement_delta *delta=NULL) = 0;

	virtual	float				StringWidth(	const char *string,
												int32 length,
												const DrawData *d) = 0;

	virtual	float				StringHeight(	const char *string,
												int32 length,
												const DrawData *d) = 0;

	virtual	void				GetBoundingBoxes(const char *string,
												 int32 count,
												 font_metric_mode mode, 
												 escapement_delta *delta,
												 BRect *rectarray,
												 const DrawData *d) = 0;

	virtual	void				GetEscapements(	const char *string,
												int32 charcount,
												escapement_delta *delta, 
												escapement_delta *escapements,
												escapement_delta *offsets,
												const DrawData *d) = 0;

	virtual	void				GetEdges(		const char *string,
												int32 charcount,
												edge_info *edgearray,
												const DrawData *d) = 0;

	virtual	void				GetHasGlyphs(	const char *string,
												int32 charcount,
												bool *hasarray) = 0;

	virtual	void				GetTruncatedStrings(const char **instrings,
													const int32 &stringcount,
													const uint32 &mode, 
													const float &maxwidth,
													char **outstrings) = 0;
	
	virtual	void				HideCursor() = 0;
	virtual	bool				IsCursorHidden() = 0;
	virtual	void				MoveCursorTo(	const float &x,
												const float &y) = 0;
	virtual	void				ShowCursor() = 0;
	virtual	void				ObscureCursor() = 0;
	virtual	void				SetCursor(ServerCursor *cursor) = 0;
	virtual	BPoint				GetCursorPosition() = 0;
	virtual	bool				IsCursorObscured(bool state) = 0;
	
	
	// Virtual methods which need to be implemented by each subclass

	// These two will rarely be implemented by subclasses,
	// but it still needs to be possible
	virtual bool				Lock(bigtime_t timeout = B_INFINITE_TIMEOUT) = 0;
	virtual void				Unlock() = 0;

	// display mode access
	virtual void				SetMode(const display_mode &mode);
	virtual	void				GetMode(display_mode *mode);
			const display_mode*	DisplayMode()
									{ return &fDisplayMode; }
	
	virtual bool				DumpToFile(const char *path) = 0;
	virtual ServerBitmap*		DumpToBitmap() = 0;

	virtual void				StrokeLineArray(const int32 &numlines,
												const LineArrayData *data,
												const DrawData *d) = 0;

	virtual status_t			SetDPMSMode(const uint32 &state) = 0;
	virtual uint32				DPMSMode() = 0;
	virtual uint32				DPMSCapabilities() = 0;
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


	// needed by CursorHandler
	virtual void				CopyBitmap(ServerBitmap *bitmap,
										   const BRect &source,
										   const BRect &dest,
										   const DrawData *d) = 0;

	virtual void				CopyToBitmap(ServerBitmap *target,
											 const BRect &source) = 0;

	// This is for drivers which are internally double buffered
	// and calling this will cause the real framebuffer to be updated
	// needed by CursorHandler, (Layer ?)
	virtual void				Invalidate(const BRect &r) = 0;

	// temporarily virtual - until clipping code is added in DisplayDriver
	// needed by Layer
	virtual	void				ConstrainClippingRegion(BRegion *reg) = 0;

 protected:
			display_mode		fDisplayMode;
};

#endif
