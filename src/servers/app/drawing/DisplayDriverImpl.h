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
//	File Name:		DisplayDriver.h
//	Authors:		DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//
//	Description:	Abstract class which handles all graphics output
//					for the server
//  
//------------------------------------------------------------------------------
#ifndef _DISPLAY_DRIVER_IMPL_H_
#define _DISPLAY_DRIVER_IMPL_H_

#include <OS.h>

#include <View.h>
#include <Rect.h>
#include <Screen.h>
#include <Region.h>

#include <ft2build.h>
#include FT_FREETYPE_H

//#include "CursorHandler.h"
#include "DisplaySupport.h"
#include "LayerData.h"
#include "PatternHandler.h"
#include "RGBColor.h"
#include "ServerBitmap.h"

#include "DisplayDriver.h"

#ifndef ROUND
	#define ROUND(a)	( (long)(a+.5) )
#endif


class ServerCursor;

/*!
	\brief Data structure for passing cursor information to hardware drivers.
*/
typedef struct
{
	uchar *xormask, *andmask;
	int32 width, height;
	int32 hotx, hoty;

} cursor_data;

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

class DisplayDriverImpl;

typedef void (DisplayDriverImpl::* SetPixelFuncType)(int x, int y);
typedef void (DisplayDriverImpl::* SetHorizontalLineFuncType)(int xstart, int xend, int y);
typedef void (DisplayDriverImpl::* SetVerticalLineFuncType)(int x, int ystart, int yend);
typedef void (DisplayDriverImpl::* SetRectangleFuncType)(int left, int top, int right, int bottom);

/*!
	\class DisplayDriverImpl DisplayDriverImpl.h
	\brief original DisplayDriver implementation.
	
	While all virtual functions are technically optional, the default versions
	do very little, so implementing them all more or less required.
*/

class DisplayDriverImpl : public DisplayDriver {
 public:
								DisplayDriverImpl();
	virtual						~DisplayDriverImpl();

	// Graphics calls implemented in DisplayDriver
	virtual	void				CopyBits(		const BRect &src,
												const BRect &dest,
												const DrawData *d);

	virtual	void				CopyRegion(		BRegion *src,
												const BPoint &lefttop);

	virtual void				InvertRect(		const BRect &r);

	virtual	void				DrawBitmap(		BRegion *region,
												ServerBitmap *bitmap,
												const BRect &source,
												const BRect &dest,
												const DrawData *d);

	virtual	void				CopyRegionList(	BList* list,
												BList* pList,
												int32 rCount,
												BRegion* clipReg);

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
												const DrawData *d);

	// this version used by Decorator
	virtual	void				StrokePoint(	const BPoint &pt,
												const RGBColor &color);

	virtual	void				StrokePoint(	const BPoint &pt,
												const DrawData *d);

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
	virtual	void				DrawString(const char *string,
										   const int32 &length,
										   const BPoint &pt,
										   DrawData *d);

	virtual	void				DrawString(const char *string,
										   const int32 &length,
										   const BPoint &pt,
										   const RGBColor &color,
										   escapement_delta *delta=NULL);

	virtual	float				StringWidth(const char *string,
											int32 length,
											const DrawData *d);

	virtual	float				StringHeight(const char *string,
											 int32 length,
											 const DrawData *d);

	virtual	void				GetBoundingBoxes(const char *string,
												 int32 count,
												 font_metric_mode mode, 
												 escapement_delta *delta,
												 BRect *rectarray,
												 const DrawData *d);

	virtual	void				GetEscapements(const char *string,
											   int32 charcount,
											   escapement_delta *delta, 
											   escapement_delta *escapements,
											   escapement_delta *offsets,
											   const DrawData *d);

	virtual	void				GetEdges(const char *string,
										 int32 charcount,
										 edge_info *edgearray,
										 const DrawData *d);

	virtual	void				GetHasGlyphs(const char *string,
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
	
	
	// These two will rarely be implemented by subclasses,
	// but it still needs to be possible
	virtual bool				Lock(bigtime_t timeout = B_INFINITE_TIMEOUT);
	virtual void				Unlock();

	virtual bool				DumpToFile(const char *path);
	virtual ServerBitmap*		DumpToBitmap();

	virtual void				StrokeLineArray(const int32 &numlines,
												const LineArrayData *data,
												const DrawData *d);

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


	// needed by CursorHandler
	virtual void				CopyBitmap(ServerBitmap *bitmap,
										   const BRect &source,
										   const BRect &dest,
										   const DrawData *d);

	virtual void				CopyToBitmap(ServerBitmap *target,
											 const BRect &source);

			// This is for drivers which are internally double buffered
			// and calling this will cause the real framebuffer to be updated
	virtual void				Invalidate(const BRect &r);

		// temporarily virtual - until clipping code is added in DisplayDriver
		// needed by Layer
	virtual	void				ConstrainClippingRegion(BRegion *reg);

protected:
//friend class Layer;
friend class WinBorder;
//friend CursorHandler;

			ServerCursor*		_GetCursor();

	virtual void				HLinePatternThick(int32 x1, int32 x2, int32 y);

	virtual void				VLinePatternThick(int32 x, int32 y1, int32 y2);

	virtual void				SetThickPatternPixel(int x, int y);
	
			// Blit functions specific to FreeType2 glyph copying.
			// These probably could be replaced with more generic functions,
			// but these are written and can be replaced later.
			void				BlitMono2RGB32(FT_Bitmap *src,
											   const BPoint &pt,
											   const DrawData *d);

			void				BlitGray2RGB32(FT_Bitmap *src,
											   const BPoint &pt,
											   const DrawData *d);
	
			// Two functions for gaining direct access to the framebuffer
			// of a child class. This removes the need for a set of
			// glyph-blitting virtual functions for each driver.
	virtual bool				AcquireBuffer(FBBitmap *bmp);
	virtual void				ReleaseBuffer();
	
	
/*			void				FillBezier(BPoint *pts,
										   DisplayDriver* driver,
										   SetHorizontalLineFuncType setLine);

			void				FillRegion(BRegion &r,
										   DisplayDriver* driver,
										   SetRectangleFuncType setRect);

			void				StrokeArc(const BRect &r,
										  const float &angle,
										  const float &span,
										  DisplayDriver* driver,
										  SetPixelFuncType setPixel);

			void				StrokeBezier(BPoint *pts,
											 DisplayDriver* driver,
											 SetPixelFuncType setPixel);

			void				StrokeEllipse(const BRect &r,
											  DisplayDriver* driver,
											  SetPixelFuncType setPixel);

			void				StrokeLine(const BPoint &start,
										   const BPoint &end,
										   DisplayDriver* driver,
										   SetPixelFuncType setPixel);*/
	
	// Support functions for the rest of the driver
	virtual void				Blit(const BRect &src,
									 const BRect &dest,
									 const DrawData *d);

	virtual void				FillSolidRect(const BRect &rect,
											  const RGBColor &color);

	virtual void				FillPatternRect(const BRect &rect,
												const DrawData *d);

	virtual void				StrokeSolidLine(int32 x1, int32 y1,
												int32 x2, int32 y2,
												const RGBColor &color);

	virtual void				StrokePatternLine(int32 x1, int32 y1,
												  int32 x2, int32 y2,
												  const DrawData *d);

	virtual void				StrokeSolidRect(const BRect &rect,
												const RGBColor &color);

 protected:
			BLocker				fLocker;
			CursorHandler		fCursorHandler;

			uint32				fDPMSState;
			uint32				fDPMSCaps;
};

#endif
