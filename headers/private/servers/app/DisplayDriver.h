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
//	File Name:		DisplayDriver.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//
//	Description:	Mostly abstract class which handles all graphics output
//					for the server
//  
//------------------------------------------------------------------------------
#ifndef _DISPLAY_DRIVER_H_
#define _DISPLAY_DRIVER_H_

#include <Accelerant.h>
#include <OS.h>

#include <View.h>
#include <Font.h>
#include <Rect.h>
#include <Locker.h>
#include <Screen.h>
#include "RGBColor.h"
#include <Region.h>
#include "PatternHandler.h"
#include "LayerData.h"
#include "ServerBitmap.h"
#include <ft2build.h>
#include FT_FREETYPE_H

class ServerCursor;

#ifndef ROUND
	#define ROUND(a)	( (long)(a+.5) )
#endif

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

class DisplayDriver;

typedef void (DisplayDriver::* SetPixelFuncType)(int x, int y);
typedef void (DisplayDriver::* SetHorizontalLineFuncType)(int xstart, int xend, int y);
typedef void (DisplayDriver::* SetVerticalLineFuncType)(int x, int ystart, int yend);
typedef void (DisplayDriver::* SetRectangleFuncType)(int left, int top, int right, int bottom);

class LineCalc
{
public:
	LineCalc();
	LineCalc(const BPoint &pta, const BPoint &ptb);
	void SetPoints(const BPoint &pta, const BPoint &ptb);
	float GetX(float y);
	float GetY(float x);
	float Slope(void) { return slope; }
	float Offset(void) { return offset; }
	float MinX() { return minx; }
	float MinY() { return miny; }
	float MaxX() { return maxx; }
	float MaxY() { return maxy; }
	void Swap(LineCalc &from);
	bool ClipToRect(const BRect &rect);
	BPoint GetStart() { return start; }
	BPoint GetEnd() { return end; }
private:
	float slope;
	float offset;
	BPoint start, end;
	float minx;
	float miny;
	float maxx;
	float maxy;
};

/*!
	\class FBBitmap DisplayDriver.h
	\brief Class used for easily passing around information about the framebuffer
*/
class FBBitmap : public ServerBitmap
{
public:
	FBBitmap(void) : ServerBitmap(BRect(0,0,0,0),B_NO_COLOR_SPACE,0) { }
	~FBBitmap(void) { }
	void SetBytesPerRow(const int32 &bpr) { _bytesperrow=bpr; }
	void SetSpace(const color_space &space) { _space=space; }
	void SetSize(const int32 &w, const int32 &h) { _width=w; _height=h; }
	void SetBuffer(void *ptr) { _buffer=(uint8*)ptr; }
	void SetBitsPerPixel(color_space space,int32 bytesperline) { _HandleSpace(space,bytesperline); }
	void ShallowCopy(const FBBitmap *from)
	{
		ServerBitmap::ShallowCopy((ServerBitmap*)from);
		SetBuffer(from->Bits());
	}
};

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

class DisplayDriver
{
public:
	DisplayDriver(void);
	virtual ~DisplayDriver(void);

	// Graphics calls implemented in DisplayDriver
	void CopyBits(const BRect &src, const BRect &dest);
	void CopyRegion(BRegion *src, const BPoint &lefttop);
	void DrawBitmap(ServerBitmap *bitmap, const BRect &source, const BRect &dest, const DrawData *d);
		// one more:
	void CopyRegionList(BList* list, BList* pList, int32 rCount, BRegion* clipReg);

	void FillArc(const BRect &r, const float &angle, const float &span, RGBColor &color);
	void FillArc(const BRect &r, const float &angle, const float &span, const DrawData *d);
	void FillBezier(BPoint *pts, RGBColor &color);
	void FillBezier(BPoint *pts, const DrawData *d);
	void FillEllipse(const BRect &r, RGBColor &color);
	void FillEllipse(const BRect &r, const DrawData *d);
	void FillPolygon(BPoint *ptlist, int32 numpts, RGBColor &color);
	void FillPolygon(BPoint *ptlist, int32 numpts, const DrawData *d);
	void FillRect(const BRect &r, RGBColor &color);
	void FillRect(const BRect &r, const DrawData *d);
	void FillRegion(BRegion &r, RGBColor &color);
	void FillRegion(BRegion &r, const DrawData *d);
	void FillRoundRect(const BRect &r, const float &xrad, const float &yrad, RGBColor &color);
	void FillRoundRect(const BRect &r, const float &xrad, const float &yrad, const DrawData *d);
//	void FillShape(SShape *sh, const DrawData *d, const Pattern &pat);
	void FillTriangle(BPoint *pts, RGBColor &color);
	void FillTriangle(BPoint *pts, const DrawData *d);

	void HideCursor(void);
	bool IsCursorHidden(void);
	void MoveCursorTo(const float &x, const float &y);
	void ShowCursor(void);
	void ObscureCursor(void);
	void SetCursor(ServerCursor *cursor);

	void StrokeArc(const BRect &r, const float &angle, const float &span, RGBColor &color);
	void StrokeArc(const BRect &r, const float &angle, const float &span, const DrawData *d);
	void StrokeBezier(BPoint *pts, RGBColor &color);
	void StrokeBezier(BPoint *pts, const DrawData *d);
	void StrokeEllipse(const BRect &r, RGBColor &color);
	void StrokeEllipse(const BRect &r, const DrawData *d);
	void StrokeLine(const BPoint &start, const BPoint &end, RGBColor &color);
	void StrokeLine(const BPoint &start, const BPoint &end, const DrawData *d);
	void StrokePoint(BPoint &pt, RGBColor &color);
	void StrokePolygon(BPoint *ptlist, int32 numpts, RGBColor &color, bool is_closed=true);
	void StrokePolygon(BPoint *ptlist, int32 numpts, const DrawData *d, bool is_closed=true);
	void StrokeRect(const BRect &r, RGBColor &color);
	void StrokeRect(const BRect &r, const DrawData *d);
	void StrokeRegion(BRegion &r, RGBColor &color);
	void StrokeRegion(BRegion &r, const DrawData *d);
	void StrokeRoundRect(const BRect &r, const float &xrad, const float &yrad, RGBColor &color);
	void StrokeRoundRect(const BRect &r, const float &xrad, const float &yrad, const DrawData *d);
//	void StrokeShape(SShape *sh, const DrawData *d, const Pattern &pat);
	void StrokeTriangle(BPoint *pts, RGBColor &color);
	void StrokeTriangle(BPoint *pts, const DrawData *d);

	void GetMode(display_mode *mode);
	
	// Font-related calls
	void DrawString(const char *string, const int32 &length, const BPoint &pt, const DrawData *d);
	void DrawString(const char *string, const int32 &length, const BPoint &pt, RGBColor &color, escapement_delta *delta=NULL);

	float StringWidth(const char *string, int32 length, const DrawData *d);
	float StringHeight(const char *string, int32 length, const DrawData *d);

	void GetBoundingBoxes(const char *string, int32 count, font_metric_mode mode, 
			escapement_delta *delta, BRect *rectarray, const DrawData *d);
	void GetEscapements(const char *string, int32 charcount, escapement_delta *delta, 
			escapement_delta *escapements, escapement_delta *offsets, const DrawData *d);
	void GetEdges(const char *string, int32 charcount, edge_info *edgearray, const DrawData *d);
	void GetHasGlyphs(const char *string, int32 charcount, bool *hasarray);
	void GetTruncatedStrings(const char **instrings, const int32 &stringcount, const uint32 &mode, 
			const float &maxwidth, char **outstrings);
	
	bool IsCursorObscured(bool state);
	
	
	// Virtual methods which need to be implemented by each subclass
	virtual bool Initialize(void);
	virtual void Shutdown(void);

	// These two will rarely be implemented by subclasses, but it still needs to be possible
	virtual bool Lock(bigtime_t timeout=B_INFINITE_TIMEOUT);
	virtual void Unlock(void);

	virtual void SetMode(const display_mode &mode);
	
	virtual bool DumpToFile(const char *path);
	virtual ServerBitmap *DumpToBitmap(void);
	virtual void InvertRect(const BRect &r);
	virtual void StrokeLineArray(BPoint *pts, const int32 &numlines, const DrawData *d, RGBColor *colors);

	virtual status_t SetDPMSMode(const uint32 &state);
	virtual uint32 DPMSMode(void) const;
	virtual uint32 DPMSCapabilities(void) const;
	virtual status_t GetDeviceInfo(accelerant_device_info *info);
	virtual status_t GetModeList(display_mode **mode_list, uint32 *count);
	virtual status_t GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high);
	virtual status_t GetTimingConstraints(display_timing_constraints *dtc);
	virtual status_t ProposeMode(display_mode *candidate, const display_mode *low, const display_mode *high);
	virtual status_t WaitForRetrace(bigtime_t timeout=B_INFINITE_TIMEOUT);

protected:
friend class Layer;
friend class WinBorder;

	ServerCursor *_GetCursor(void);
	virtual void HLinePatternThick(int32 x1, int32 x2, int32 y);
	virtual void VLinePatternThick(int32 x, int32 y1, int32 y2);
//	virtual void FillSolidRect(int32 left, int32 top, int32 right, int32 bottom);
//	virtual void FillPatternRect(int32 left, int32 top, int32 right, int32 bottom);
	virtual void SetThickPatternPixel(int x, int y);
	
	// Blit functions specific to FreeType2 glyph copying. These probably could be replaced with
	// more generic functions, but these are written and can be replaced later.
	void BlitMono2RGB32(FT_Bitmap *src, const BPoint &pt, const DrawData *d);
	void BlitGray2RGB32(FT_Bitmap *src, const BPoint &pt, const DrawData *d);
	
	// Two functions for gaining direct access to the framebuffer of a child class. This removes the need
	// for a set of glyph-blitting virtual functions for each driver.
	virtual bool AcquireBuffer(FBBitmap *bmp);
	virtual void ReleaseBuffer(void);
	
	// This is for drivers which are internally double buffered and calling this will cause the real
	// framebuffer to be updated
	virtual void Invalidate(const BRect &r);
	
	void FillBezier(BPoint *pts, DisplayDriver* driver, SetHorizontalLineFuncType setLine);
	void FillRegion(BRegion &r, DisplayDriver* driver, SetRectangleFuncType setRect);
	void StrokeArc(const BRect &r, const float &angle, const float &span, DisplayDriver* driver, SetPixelFuncType setPixel);
	void StrokeBezier(BPoint *pts, DisplayDriver* driver, SetPixelFuncType setPixel);
	void StrokeEllipse(const BRect &r, DisplayDriver* driver, SetPixelFuncType setPixel);
	void StrokeLine(const BPoint &start, const BPoint &end, DisplayDriver* driver, SetPixelFuncType setPixel);
	
	// Support functions for the rest of the driver
	virtual void Blit(const BRect &src, const BRect &dest, const DrawData *d);
	virtual void FillSolidRect(const BRect &rect, RGBColor &color);
	virtual void FillPatternRect(const BRect &rect, const DrawData *d);
	virtual void StrokeSolidLine(const BPoint &start, const BPoint &end, RGBColor &color);
	virtual void StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d);
	virtual void StrokeSolidRect(const BRect &rect, RGBColor &color);
	virtual void CopyBitmap(ServerBitmap *bitmap, const BRect &source, const BRect &dest, const DrawData *d);
	virtual void CopyToBitmap(ServerBitmap *target, const BRect &source);
		// temporarily virtual - until clipping code is added in DisplayDriver
	virtual	void ConstrainClippingRegion(BRegion *reg);

	PatternHandler fDrawPattern;
	RGBColor fDrawColor;
	int fLineThickness;
	
	BLocker *_locker;
	bool _is_cursor_hidden;
	bool _is_cursor_obscured;

	ServerCursor *_cursor;
	UtilityBitmap *_cursorsave;

	uint32 _dpms_state;
	uint32 _dpms_caps;
	accelerant_device_info _acc_device_info;
	display_mode _displaymode;
	
	BRect oldcursorframe, cursorframe, saveframe;
	DrawData _drawdata;
};

#endif
