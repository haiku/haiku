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
//	File Name:		ViewDriver.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	BView/BWindow combination graphics module
//  
//------------------------------------------------------------------------------
#ifndef _VIEWDRIVER_H_
#define _VIEWDRIVER_H_

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <GraphicsCard.h>
#include <GraphicsDefs.h>	// for pattern struct
#include <Cursor.h>
#include <Message.h>
#include <Region.h>
#include <Font.h>
#include "DisplayDriver.h"
#include "FontServer.h"

class BBitmap;
class PortLink;
class VDWindow;
class LayerData;

class VDView : public BView
{
public:
	VDView(BRect bounds);
	~VDView(void);
	void AttachedToWindow(void);
	void Draw(BRect rect);
	void MouseDown(BPoint pt);
	void MouseMoved(BPoint pt, uint32 transit, const BMessage *msg);
	void MouseUp(BPoint pt);
	void MessageReceived(BMessage *msg);
		
	BBitmap *viewbmp;
	PortLink *serverlink;
	
	int hide_cursor;
	BBitmap *cursor;
	
	BRect cursorframe, oldcursorframe;
	bool obscure_cursor;
};

class VDWindow : public BWindow
{
public:
	VDWindow(void);
	~VDWindow(void);
	void MessageReceived(BMessage *msg);
	bool QuitRequested(void);
	void WindowActivated(bool active);
	
	VDView *view;
};

/*!
	\brief BView/BWindow combination graphics module

	First, slowest, and easiest driver class in the app_server which is designed
	to utilize the BeOS graphics functions to cut out a lot of junk in getting the
	drawing infrastructure in this server.
	
	The concept is to have VDView::Draw() draw a bitmap, which is a "frame buffer" 
	of sorts, utilize a second view to write to it. This cuts out
	the most problems with having a crapload of code to get just right without
	having to write a bunch of unit tests
	
	Components:		3 classes, VDView, VDWindow, and ViewDriver
	
	ViewDriver - a wrapper class which mostly posts messages to the VDWindow
	VDWindow - does most of the work.
	VDView - doesn't do all that much except display the rendered bitmap
*/
class ViewDriver : public DisplayDriver
{
public:
	ViewDriver(void);
	~ViewDriver(void);

	bool Initialize(void);		// Sets the driver
	void Shutdown(void);		// You never know when you'll need this
	
	// Drawing functions
	void CopyBits(BRect src, BRect dest);
	void CopyRegion(BRegion *src, const BPoint &lefttop);
	void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest);
	void DrawChar(char c, BPoint pt, LayerData *d);
//	virtual void DrawPicture(SPicture *pic, BPoint pt);
	void DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL);

	void HideCursor(void);
	void InvertRect(BRect r);
	bool IsCursorHidden(void);
	void MoveCursorTo(float x, float y);
//	void MovePenTo(BPoint pt);
	void ObscureCursor(void);
//	BPoint PenPosition(void);
//	float PenSize(void);
	void SetCursor(ServerCursor *cursor);
//	drawing_mode GetDrawingMode(void);
//	void SetDrawingMode(drawing_mode mode);
	void ShowCursor(void);

	virtual void FillArc(const BRect r, float angle, float span, RGBColor& color);
	virtual void FillArc(const BRect r, float angle, float span, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
	virtual void FillBezier(BPoint *pts, RGBColor& color);
	virtual void FillBezier(BPoint *pts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
	virtual void FillEllipse(BRect r, RGBColor& color);
	virtual void FillEllipse(BRect r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
	virtual void FillPolygon(BPoint *ptlist, int32 numpts, RGBColor& color);
	virtual void FillPolygon(BPoint *ptlist, int32 numpts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
	virtual void FillRect(const BRect r, RGBColor& color);
	virtual void FillRect(const BRect r, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
	virtual void FillRoundRect(BRect r, float xrad, float yrad, RGBColor& color);
	virtual void FillRoundRect(BRect r, float xrad, float yrad, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
//	virtual void FillShape(SShape *sh, LayerData *d, const Pattern &pat);
	virtual void FillTriangle(BPoint *pts, RGBColor& color);
	virtual void FillTriangle(BPoint *pts, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
	
	virtual void StrokeArc(BRect r, float angle, float span, float pensize, RGBColor& color);
	virtual void StrokeArc(BRect r, float angle, float span, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
	virtual void StrokeBezier(BPoint *pts, float pensize, RGBColor& color);
	virtual void StrokeBezier(BPoint *pts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
	virtual void StrokeEllipse(BRect r, float pensize, RGBColor& color);
	virtual void StrokeEllipse(BRect r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
	virtual void StrokeLine(BPoint start, BPoint end, float pensize, RGBColor& color);
	virtual void StrokeLine(BPoint start, BPoint end, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
	virtual void StrokePoint(BPoint& pt, RGBColor& color);
	virtual void StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, RGBColor& color, bool is_closed=true);
	virtual void StrokePolygon(BPoint *ptlist, int32 numpts, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color, bool is_closed=true);
	virtual void StrokeRect(BRect r, float pensize, RGBColor& color);
	virtual void StrokeRect(BRect r, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
	virtual void StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, RGBColor& color);
	virtual void StrokeRoundRect(BRect r, float xrad, float yrad, float pensize, const Pattern& pattern, RGBColor& high_color, RGBColor& low_color);
//	virtual void StrokeShape(SShape *sh, LayerData *d, const Pattern &pat);

	virtual void StrokeLineArray(BPoint *pts, int32 numlines, float pensize, RGBColor *colors);

	void SetMode(int32 mode);
	void SetMode(const display_mode &mode);
	
	float StringWidth(const char *string, int32 length, LayerData *d);
	float StringHeight(const char *string, int32 length, LayerData *d);
	bool DumpToFile(const char *path);
	VDWindow *screenwin;

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
	void SetLayerData(LayerData *d, bool set_font_data=false);
	void BlitMono2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d);
	void BlitGray2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d);
	rgb_color GetBlitColor(rgb_color src, rgb_color dest, LayerData *d, bool use_high=true);
	int hide_cursor;
	bool obscure_cursor;
	BBitmap *framebuffer;
	BView *drawview;
	BRegion laregion;

	PortLink *serverlink;
//	drawing_mode drawmode;
	
	rgb_color highcolor,lowcolor;
	bool is_initialized;
};

#endif
