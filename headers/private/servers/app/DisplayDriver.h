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
//				Gabe Yoder <gyoder@stny.rr.com>
//	Description:	Mostly abstract class which handles all graphics output
//					for the server
//  
//------------------------------------------------------------------------------
#ifndef _DISPLAY_DRIVER_H_
#define _DISPLAY_DRIVER_H_

#include <GraphicsCard.h>
#include <OS.h>

#include <View.h>
#include <Font.h>
#include <Rect.h>
#include <Locker.h>
#include <Screen.h>
#include "RGBColor.h"
#include <Region.h>
#include "PatternHandler.h"

class ServerCursor;
class ServerBitmap;
class LayerData;

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
	bool ClipToRect(const BRect& rect);
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
	virtual bool Initialize(void);
	virtual void Shutdown(void);
	virtual void CopyBits(BRect src, BRect dest);
	virtual void CopyRegion(BRegion *src, const BPoint &lefttop);
	virtual void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest, LayerData *d);
	virtual void DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL);

	virtual void FillArc(const BRect r, float angle, float span, RGBColor &color);
	virtual void FillArc(const BRect r, float angle, float span, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void FillBezier(BPoint *pts, RGBColor &color);
	virtual void FillBezier(BPoint *pts, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void FillEllipse(BRect r, RGBColor &color);
	virtual void FillEllipse(BRect r, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void FillPolygon(BPoint *ptlist, int32 numpts, RGBColor &color);
	virtual void FillPolygon(BPoint *ptlist, int32 numpts, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void FillRect(const BRect r, RGBColor &color);
	virtual void FillRect(const BRect r, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void FillRegion(BRegion& r, RGBColor &color);
	virtual void FillRegion(BRegion& r, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void FillRoundRect(BRect r, float xrad, float yrad, RGBColor &color);
	virtual void FillRoundRect(BRect r, float xrad, float yrad, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
//	virtual void FillShape(SShape *sh, LayerData *d, const Pattern &pat);
	virtual void FillTriangle(BPoint *pts, RGBColor &color);
	virtual void FillTriangle(BPoint *pts, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);

	virtual void HideCursor(void);
	virtual bool IsCursorHidden(void);
	virtual void MoveCursorTo(float x, float y);
	virtual void InvertRect(BRect r);
	virtual void ShowCursor(void);
	virtual void ObscureCursor(void);
	virtual void SetCursor(ServerCursor *cursor);

	virtual void StrokeArc(BRect r, float angle, float span, float pensize, RGBColor &color);
	virtual void StrokeArc(BRect r, float angle, float span, float pensize, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void StrokeBezier(BPoint *pts, float pensize, RGBColor &color);
	virtual void StrokeBezier(BPoint *pts, float pensize, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void StrokeEllipse(BRect r, float pensize, RGBColor &color);
	virtual void StrokeEllipse(BRect r, float pensize, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void StrokeLine(BPoint start, BPoint end, float pensize, RGBColor &color);
	virtual void StrokeLine(BPoint start, BPoint end, float pensize, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void StrokePoint(BPoint& pt, RGBColor &color);
	virtual void StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, RGBColor &color, bool is_closed=true);
	virtual void StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color, bool is_closed=true);
	virtual void StrokeRect(BRect r, float pensize, RGBColor &color);
	virtual void StrokeRect(BRect r, float pensize, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void StrokeRegion(BRegion& r, float pensize, RGBColor &color);
	virtual void StrokeRegion(BRegion& r, float pensize, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
	virtual void StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, RGBColor &color);
	virtual void StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);
//	virtual void StrokeShape(SShape *sh, LayerData *d, const Pattern &pat);
	virtual void StrokeTriangle(BPoint *pts, float pensize, RGBColor &color);
	virtual void StrokeTriangle(BPoint *pts, float pensize, const Pattern& pattern, RGBColor &high_color, RGBColor &low_color);

	virtual void StrokeLineArray(BPoint *pts, int32 numlines, float pensize, RGBColor *colors);

	virtual void SetMode(int32 mode);
	virtual void SetMode(const display_mode &mode);
	virtual void GetMode(display_mode *mode);
	virtual bool DumpToFile(const char *path);
	virtual ServerBitmap *DumpToBitmap(void);

	virtual float StringWidth(const char *string, int32 length, LayerData *d);
	virtual float StringHeight(const char *string, int32 length, LayerData *d);

	virtual void GetBoundingBoxes(const char *string, int32 count, font_metric_mode mode,
			escapement_delta *delta, BRect *rectarray, LayerData *d);
	virtual void GetEscapements(const char *string, int32 charcount, escapement_delta *delta, 
			escapement_delta *escapements, escapement_delta *offsets, LayerData *d);
	virtual void GetEdges(const char *string, int32 charcount, edge_info *edgearray, LayerData *d);
	virtual void GetHasGlyphs(const char *string, int32 charcount, bool *hasarray);
	virtual void GetTruncatedStrings( const char **instrings, int32 stringcount, uint32 mode, 
			float maxwidth, char **outstrings);
	
	uint8 GetDepth(void);
	uint16 GetHeight(void);
	uint16 GetWidth(void);
	uint32 GetBytesPerRow(void);
	int32 GetMode(void);
	bool IsCursorObscured(bool state);
	
	bool Lock(bigtime_t timeout=B_INFINITE_TIMEOUT);
	void Unlock(void);

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
	void _SetDepth(uint8 d);
	void _SetHeight(uint16 h);
	void _SetWidth(uint16 w);
	void _SetMode(int32 m);
	void _SetBytesPerRow(uint32 bpr);
	void _SetDPMSCapabilities(uint32 caps);
	void _SetDPMSState(uint32 state);
	void _SetDeviceInfo(const accelerant_device_info &infO);
	ServerCursor *_GetCursor(void);
	//virtual void HLine(int32 x1, int32 x2, int32 y, PatternHandler *pat);
	//virtual void HLineThick(int32 x1, int32 x2, int32 y, int32 thick, PatternHandler *pat);
	virtual void HLinePatternThick(int32 x1, int32 x2, int32 y);
	virtual void VLinePatternThick(int32 x, int32 y1, int32 y2);
	virtual void FillSolidRect(int32 left, int32 top, int32 right, int32 bottom);
	virtual void FillPatternRect(int32 left, int32 top, int32 right, int32 bottom);
	//virtual void SetPixel(int x, int y, RGBColor col);
	//virtual void SetThickPixel(int x, int y, int thick, PatternHandler *pat);
	virtual void SetThickPatternPixel(int x, int y);

	void FillArc(const BRect r, float angle, float span, DisplayDriver*, SetHorizontalLineFuncType setLine);
	void FillBezier(BPoint *pts, DisplayDriver* driver, SetHorizontalLineFuncType setLine);
	void FillEllipse(BRect r, DisplayDriver* driver, SetHorizontalLineFuncType setLine);
	void FillPolygon(BPoint *ptlist, int32 numpts, DisplayDriver* driver, SetHorizontalLineFuncType setLine);
	void FillRegion(BRegion& r, DisplayDriver* driver, SetRectangleFuncType setRect);
	void FillRoundRect(BRect r, float xrad, float yrad, DisplayDriver* driver, SetRectangleFuncType setRect, SetHorizontalLineFuncType setLine);
	void FillTriangle(BPoint *pts, DisplayDriver* driver, SetHorizontalLineFuncType setLine);
	void StrokeArc(BRect r, float angle, float span, DisplayDriver* driver, SetPixelFuncType setPixel);
	void StrokeBezier(BPoint *pts, DisplayDriver* driver, SetPixelFuncType setPixel);
	void StrokeEllipse(BRect r, DisplayDriver* driver, SetPixelFuncType setPixel);
	void StrokeLine(BPoint start, BPoint end, DisplayDriver* driver, SetPixelFuncType setPixel);
	void StrokePolygon(BPoint *ptlist, int32 numpts, DisplayDriver* driver, SetPixelFuncType setPixel, bool is_closed=true);
	void StrokeRect(BRect r, DisplayDriver* driver, SetHorizontalLineFuncType setHLine, SetVerticalLineFuncType setVLine);
	void StrokeRoundRect(BRect r, float xrad, float yrad, DisplayDriver* driver, SetHorizontalLineFuncType setHLine, SetVerticalLineFuncType setVLine, SetPixelFuncType setPixel);

	PatternHandler fDrawPattern;
	RGBColor fDrawColor;
	int fLineThickness;

private:
	BLocker *_locker;
	uint8 _buffer_depth;
	uint16 _buffer_width;
	uint16 _buffer_height;
	int32 _buffer_mode;
	uint32 _bytes_per_row;
	bool _is_cursor_hidden;
	bool _is_cursor_obscured;
	ServerCursor *_cursor;
	uint32 _dpms_state;
	uint32 _dpms_caps;
	accelerant_device_info _acc_device_info;
};

#endif
