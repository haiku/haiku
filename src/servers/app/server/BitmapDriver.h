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
//	File Name:		BitmapDriver.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//
//	Description:	Driver to draw on ServerBitmaps
//  
//------------------------------------------------------------------------------
#ifndef _BITMAPDRIVER_H_
#define _BITMAPNDRIVER_H_

#include <Application.h>
#include <View.h>
#include <GraphicsCard.h>
#include <Message.h>
#include <OS.h>
#include <Locker.h>
#include <Region.h>	// for clipping_rect definition
#include <Bitmap.h>
#include <OS.h>
#include "DisplayDriver.h"
//#include <ft2build.h>
//#include FT_FREETYPE_H
//#include FT_GLYPH_H
#include "FontServer.h"

class ServerCursor;
class ServerBitmap;
class RGBColor;
class PatternHandler;

/*!
	\class BitmapDriver BitmapDriver.h
	\brief Driver to draw on ServerBitmaps
	
	This driver is not technically a regular DisplayDriver subclass. BitmapDriver 
	objects are intended for use when a BBitmap is created with the ability to 
	accept child views. It also adds one significant function over regular DisplayDriver 
	child classes - SetTarget. There is also no option for input server emulation, for 
	obvious reasons. Cursor functions are redefined to do absolutely nothing
	
	Usage: Allocate and call SetTarget on the desired ServerBitmap and start calling 
	graphics methods. All ServerBitmap memory belongs to the BitmapManager.
*/
class BitmapDriver : public DisplayDriver
{
public:
	BitmapDriver(void);
	~BitmapDriver(void);

	bool Initialize(void);
	void Shutdown(void);

	void SetTarget(ServerBitmap *target);
	ServerBitmap *GetTarget(void) const { return _target; }
	
	// Settings functions
	virtual void CopyBits(BRect src, BRect dest);
	virtual void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest, LayerData *d);
//	virtual void DrawPicture(SPicture *pic, BPoint pt);
	virtual void DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL);

	virtual void FillRect(BRect r, LayerData *d, const Pattern &pat);
//	virtual void FillShape(SShape *sh, LayerData *d, const Pattern &pat);

	virtual void HideCursor(void);
	virtual void MoveCursorTo(float x, float y);
	virtual void InvertRect(BRect r);
	virtual void ShowCursor(void);
	virtual void ObscureCursor(void);
	virtual void SetCursor(ServerCursor *cursor);

//	virtual void StrokeShape(SShape *sh, LayerData *d, const Pattern &pat);
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
	void Line(BPoint start, BPoint end, LayerData *d, const Pattern &pat);
	void HLine(int32 x1, int32 x2, int32 y, PatternHandler *pat);
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
	ServerBitmap *_target;
};

#endif
