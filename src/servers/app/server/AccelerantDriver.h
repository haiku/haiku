//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, OpenBeOS
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
//	File Name:		AccelerantDriver.h
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//	Description:		A display driver which works directly with the
//					accelerant for the graphics card.
//  
//------------------------------------------------------------------------------
#ifndef _ACCELERANTDRIVER_H_
#define _ACCELERANTDRIVER_H_

#if 0
#include <Application.h>
#include <WindowScreen.h>
#include <View.h>
#include <GraphicsCard.h>
#include <Message.h>
#include <OS.h>
#include <Locker.h>
#include <Region.h>	// for clipping_rect definition
#include <Bitmap.h>
#endif
#include <Accelerant.h>
#include "DisplayDriver.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#if 0
class VDWindow;
class RGBColor;
class PortLink;
#endif
class ServerBitmap;
class ServerCursor;

class AccelerantDriver : public DisplayDriver
{
public:
	AccelerantDriver(void);
	~AccelerantDriver(void);

	bool Initialize(void);
	void Shutdown(void);
	
	// Settings functions
	virtual void CopyBits(BRect src, BRect dest);
	virtual void DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest, LayerData *d);
	virtual void DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL);

	virtual void FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat);
	virtual void FillBezier(BPoint *pts, LayerData *d, int8 *pat);
	virtual void FillEllipse(BRect r, LayerData *d, int8 *pat);
	virtual void FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat);
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
	virtual void StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d);
	virtual void SetMode(int32 mode);
	virtual bool DumpToFile(const char *path);
	float StringWidth(const char *string, int32 length, LayerData *d);
	float StringHeight(const char *string, int32 length, LayerData *d);

	virtual void GetBoundingBoxes(const char *string, int32 count, 
		font_metric_mode mode, escapement_delta *delta, 
		BRect *rectarray, LayerData *d);
	virtual void GetEscapements(const char *string, int32 charcount, 
		escapement_delta *delta, escapement_delta *escapements, 
		escapement_delta *offsets, LayerData *d);
	virtual void GetEdges(const char *string, int32 charcount, 
		edge_info *edgearray, LayerData *dw);
	virtual void GetHasGlyphs(const char *string, int32 charcount, bool *hasarray);
	virtual void GetTruncatedStrings( const char **instrings, int32 stringcount, uint32 mode, float maxwidth, char **outstrings);
protected:
	void BlitMono2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d);
	void BlitGray2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d);
	void BlitBitmap(ServerBitmap *sourcebmp, BRect sourcerect, BRect destrect, drawing_mode mode=B_OP_COPY);
	void ExtractToBitmap(ServerBitmap *destbmp, BRect destrect, BRect sourcerect);
	void SetPixelPattern(int x, int y, uint8 *pattern, uint8 patternindex);
	void Line(BPoint start, BPoint end, LayerData *d, int8 *pat);
	void HLine(int32 x1, int32 x2, int32 y, RGBColor color);
	rgb_color GetBlitColor(rgb_color src, rgb_color dest, LayerData *d, bool use_high=true);
	void SetPixel(int x, int y, RGBColor col);
	void SetPixel32(int x, int y, rgb_color col);
	void SetPixel16(int x, int y, uint16 col);
	void SetPixel8(int x, int y, uint8 col);
	void SetThickPixel(int x, int y, int thick, RGBColor col);
	void SetThickPixel32(int x, int y, int thick, rgb_color col);
	void SetThickPixel16(int x, int y, int thick, uint16 col);
	void SetThickPixel8(int x, int y, int thick, uint8 col);
	//FrameBuffer *fbuffer;
	ServerCursor *cursor, *under_cursor;
	int32 drawmode;
	BRect cursorframe;
	int card_fd;
	image_id accelerant_image;
	GetAccelerantHook accelerant_hook;
	frame_buffer_config mFrameBufferConfig;
	display_mode *mode_list;
	display_mode mDisplayMode;
};

#endif
