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
//	File Name:		ScreenDriver.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//	Description:	BWindowScreen graphics module
//  
//------------------------------------------------------------------------------
#ifndef _SCREENDRIVER_H_
#define _SCREENDRIVER_H_

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
#include <WindowScreen.h>
#include "FontServer.h"
#include "PatternHandler.h"

class BBitmap;
class BPortLink;
class SDWindow;
class LayerData;
class ScreenDriver;

class FrameBuffer : public BWindowScreen
{
public:
	FrameBuffer(const char *title, uint32 space, status_t *st,bool debug);
	~FrameBuffer(void);
	void ScreenConnected(bool connected);
	void MessageReceived(BMessage *msg);
	bool IsConnected(void) const { return is_connected; }
	bool QuitRequested(void);
	static int32 MouseMonitor(void *data);
	static int32 CopyThread(void *data);
	void Invalidate(const BRect &r);
	graphics_card_info gcinfo;
protected:
	friend class ScreenDriver;
	
	bool is_connected;
	BPortLink *serverlink;
	BPoint mousepos;
	uint32 buttons;
	thread_id monitor_thread,copy_thread;
	BView *view;
	BBitmap *viewbmp;
	ServerBitmap *cursor;
	BRegion *invalid;
	int32 invalidflag;
};
/*
class SDView : public BView
{
public:
	SDView(BRect bounds);
	~SDView(void);
	void AttachedToWindow(void);
	void Draw(BRect rect);
	void MouseDown(BPoint pt);
	void MouseMoved(BPoint pt, uint32 transit, const BMessage *msg);
	void MouseUp(BPoint pt);
	void MessageReceived(BMessage *msg);
		
	BBitmap *viewbmp;
	BPortLink *serverlink;
	
	int hide_cursor;
	BBitmap *cursor;
	
	BRect cursorframe, oldcursorframe;
	bool obscure_cursor;
};
*/
class SDWindow : public BWindowScreen
{
public:
	SDWindow(void);
	~SDWindow(void);
	void MessageReceived(BMessage *msg);
	bool QuitRequested(void);
	void WindowActivated(bool active);
	
//	SDView *view;
};

/*!
	\brief BView/BWindow combination graphics module

	First, slowest, and easiest driver class in the app_server which is designed
	to utilize the BeOS graphics functions to cut out a lot of junk in getting the
	drawing infrastructure in this server.
	
	The concept is to have SDView::Draw() draw a bitmap, which is a "frame buffer" 
	of sorts, utilize a second view to write to it. This cuts out
	the most problems with having a crapload of code to get just right without
	having to write a bunch of unit tests
	
	Components:		3 classes, SDView, SDWindow, and ScreenDriver
	
	ScreenDriver - a wrapper class which mostly posts messages to the SDWindow
	SDWindow - does most of the work.
	SDView - doesn't do all that much except display the rendered bitmap
*/
class ScreenDriver : public DisplayDriver
{
public:
	ScreenDriver(void);
	~ScreenDriver(void);

	bool Initialize(void);		// Sets the driver
	void Shutdown(void);		// You never know when you'll need this
	
	// Drawing functions
	void CopyBits(BRect src, BRect dest);
	void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest);
	void DrawChar(char c, BPoint pt, LayerData *d);
//	virtual void DrawPicture(SPicture *pic, BPoint pt);
	void DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL);

	void FillArc(BRect r, float angle, float span, LayerData *d, const Pattern &pat);
	void FillBezier(BPoint *pts, LayerData *d, const Pattern &pat);
	void FillEllipse(BRect r, LayerData *d, const Pattern &pat);
	void FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat);
	void FillRect(BRect r, LayerData *d, const Pattern &pat);
	void FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat);
//	void FillShape(SShape *sh, LayerData *d, const Pattern &pat);
	void FillTriangle(BPoint *pts, BRect r, LayerData *d, const Pattern &pat);

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

	void StrokeArc(BRect r, float angle, float span, LayerData *d, const Pattern &pat);
	void StrokeBezier(BPoint *pts, LayerData *d, const Pattern &pat);
	void StrokeEllipse(BRect r, LayerData *d, const Pattern &pat);
	void StrokeLine(BPoint start, BPoint end, LayerData *d, const Pattern &pat);
	void StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d);
	void StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat, bool is_closed=true);
	void StrokeRect(BRect r, LayerData *d, const Pattern &pat);
	void StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat);
//	void StrokeShape(SShape *sh, LayerData *d, const Pattern &pat);
	void StrokeTriangle(BPoint *pts, BRect r, LayerData *d, const Pattern &pat);
	void SetMode(int32 mode);
	void SetMode(const display_mode &mode);
	float StringWidth(const char *string, int32 length, LayerData *d);
	float StringHeight(const char *string, int32 length, LayerData *d);
	bool DumpToFile(const char *path);
	FrameBuffer *screenwin;
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
	BPortLink *serverlink;
	
	rgb_color highcolor,lowcolor;
	bool is_initialized;
};

#endif
