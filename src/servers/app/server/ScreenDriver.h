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
//	File Name:		ScreenDriver.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//	Description:	BWindowScreen graphics module
//  
//------------------------------------------------------------------------------
#ifndef _SCREENDRIVER_H_
#define _SCREENDRIVER_H_

#include <Application.h>
#include <WindowScreen.h>
#include <View.h>
#include <GraphicsCard.h>
#include <Accelerant.h>
#include <Message.h>
#include <OS.h>
#include <Locker.h>
#include <Region.h>	// for clipping_rect definition
#include <Bitmap.h>
#include <OS.h>
#include "DisplayDriver.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

class VDWindow;
class ServerCursor;
class ServerBitmap;
class RGBColor;
class PortLink;
class ScreenDriver;
class PatternHandler;

// defines to translate acceleration function indices to something I can remember
#define HWLINE_8BIT 3
#define HWLINE_32BIT 4
#define HWRECT_8BIT 5
#define HWRECT_32BIT 6
#define HWBLIT 7
#define HWLINEARRAY_8BIT 8
#define HWLINEARRAY_32BIT 9
#define HWSYNC 10
#define HWINVERT 11
#define HWLINE_16BIT 12
#define HWRECT_16BIT 13

// function pointer typedefs for accelerated 2d functions - from Be Advanced Topics.
typedef int32 hwline8bit(int32 sx,int32 dx, int32 sy, int32 dy, uint8 color,
				bool cliptorect,int16 clipleft, int16 cliptop, int16 clipright, int16 clipbottom);
typedef int32 hwline32bit(int32 sx,int32 dx, int32 sy, int32 dy, uint32 color,
				bool cliptorect,int16 clipleft, int16 cliptop, int16 clipright, int16 clipbottom);
typedef int32 hwrect8bit(int32 left, int32 top, int32 right, int32 bottom, uint8 color);
typedef int32 hwrect32bit(int32 left, int32 top, int32 right, int32 bottom, uint32 color);
typedef int32 hwblit(int32 sx, int32 sy, int32 dx, int32 dy, int32 width, int32 height);
typedef int32 hwlinearray8bit(indexed_color_line *array, int32 linecount,
				bool cliptorect,int16 clipleft, int16 cliptop, int16 clipright, int16 clipbottom);
typedef int32 hwlinearray32bit(rgb_color_line *array, int32 linecount,
				bool cliptorect,int16 clipleft, int16 cliptop, int16 clipright, int16 clipbottom);
typedef int32 hwsync(void);
typedef int32 hwline16bit(int32 sx,int32 dx, int32 sy, int32 dy, uint16 color,
				bool cliptorect,int16 clipleft, int16 cliptop, int16 clipright, int16 clipbottom);
typedef int32 hwrect16bit(int32 left, int32 top, int32 right, int32 bottom, uint16 color);

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
	graphics_card_info gcinfo;
protected:
	friend ScreenDriver;
	
	bool is_connected;
	PortLink *serverlink;
	BPoint mousepos;
	uint32 buttons;
	thread_id monitor_thread;
	BView *view;

	// HW Acceleration stuff
	display_mode _dm;
	
	frame_buffer_config _fbc;
	
	set_cursor_shape _scs;
	move_cursor _mc;
	show_cursor _sc;
	sync_token _st;
	engine_token *_et;
	acquire_engine _ae;
	release_engine _re;
	fill_rectangle _frect;
	fill_span _fspan;
	
	screen_to_screen_blit _s2sb;
	fill_rectangle _fr;
	invert_rectangle _ir;
	screen_to_screen_transparent_blit _s2stb;
	fill_span _fs;
	
	hwline32bit _hwline32;
	hwrect32bit _hwrect32;
	
	sync_token _stoken;
};

/*
	ScreenDriver.cpp
		Replacement class for the ViewDriver which utilizes a BWindowScreen for ease
		of testing without requiring a second video card for SecondDriver and also
		without the limitations (mostly speed) of ViewDriver.
		
		This module also emulates the input server via the keyboard.
		Cursor keys move the mouse and the left modifier keys Option and Ctrl, which
		operate	the first and second mouse buttons, respectively.
		The right window key on American keymaps doesn't map to anything, so it is
		not used. Consequently, when a right modifier key is pressed, it works normally.
		
		The concept is pretty close to the retooled ViewDriver, where each module
		call locks a couple BLockers and draws to the buffer.
		
		Components:
			ScreenDriver: actual driver module
			FrameBuffer: BWindowScreen derivative which provides the module access
				to the video card
			Internal functions for doing graphics on the buffer
*/
class ScreenDriver : public DisplayDriver
{
public:
	ScreenDriver(void);
	~ScreenDriver(void);

	bool Initialize(void);
	void Shutdown(void);
	
	// Settings functions
	virtual void CopyBits(BRect src, BRect dest);
	virtual void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest, LayerData *d);
//	virtual void DrawPicture(SPicture *pic, BPoint pt);
	virtual void DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL);

	virtual void FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	virtual void FillBezier(BPoint *pts, LayerData *d, int8 *pat);
	virtual void FillEllipse(BRect r, LayerData *d, int8 *pat);
//	virtual void FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat);
	virtual void FillRect(BRect r, LayerData *d, int8 *pat);
	virtual void FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
//	virtual void FillShape(SShape *sh, LayerData *d, int8 *pat);
	virtual void FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);

	virtual void HideCursor(void);
	virtual void MoveCursorTo(float x, float y);
	virtual void InvertRect(BRect r);
	virtual void ShowCursor(void);
	virtual void ObscureCursor(void);
	virtual void SetCursor(ServerCursor *cursor);

	virtual void StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	virtual void StrokeBezier(BPoint *pts, LayerData *d, int8 *pat);
	virtual void StrokeEllipse(BRect r, LayerData *d, int8 *pat);
	virtual void StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat);
	virtual void StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true);
	virtual void StrokeRect(BRect r, LayerData *d, int8 *pat);
	virtual void StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat);
//	virtual void StrokeShape(SShape *sh, LayerData *d, int8 *pat);
	virtual void StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat);
//	virtual void StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d);
	virtual void SetMode(int32 mode);
	float StringWidth(const char *string, int32 length, LayerData *d);
	float StringHeight(const char *string, int32 length, LayerData *d);
//	virtual bool DumpToFile(const char *path);
protected:
	void BlitMono2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d);
	void BlitGray2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d);
	void BlitBitmap(ServerBitmap *sourcebmp, BRect sourcerect, BRect destrect, drawing_mode mode=B_OP_COPY);
	void ExtractToBitmap(ServerBitmap *destbmp, BRect destrect, BRect sourcerect);
	void SetPixelPattern(int x, int y, uint8 *pattern, uint8 patternindex);
	void Line(BPoint start, BPoint end, LayerData *d, int8 *pat);
	void HLine(int32 x1, int32 x2, int32 y, RGBColor color);
	void HLineThick(int32 x1, int32 x2, int32 y, int32 thick, PatternHandler *pat);
	rgb_color GetBlitColor(rgb_color src, rgb_color dest, LayerData *d, bool use_high=true);
	void SetPixel(int x, int y, RGBColor col);
	void SetPixel32(int x, int y, rgb_color col);
	void SetPixel16(int x, int y, uint16 col);
	void SetPixel8(int x, int y, uint8 col);
	void SetThickPixel(int x, int y, int thick, RGBColor col);
	void SetThickPixel32(int x, int y, int thick, rgb_color col);
	void SetThickPixel16(int x, int y, int thick, uint16 col);
	void SetThickPixel8(int x, int y, int thick, uint8 col);
	FrameBuffer *fbuffer;
	ServerCursor *cursor, *under_cursor;
	BRect cursorframe;
};

#endif
