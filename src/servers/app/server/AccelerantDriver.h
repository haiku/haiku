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

#include <Accelerant.h>
#include "DisplayDriver.h"
#include "PatternHandler.h"
#include "FontServer.h"
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
	virtual void DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *edelta=NULL);

	virtual void FillRect(BRect r, LayerData *d, const Pattern &pat);
//	virtual void FillShape(SShape *sh, LayerData *d, const Pattern &pat);

	virtual void HideCursor(void);
	virtual void MoveCursorTo(float x, float y);
	virtual void InvertRect(BRect r);
	virtual void ShowCursor(void);
	virtual void ObscureCursor(void);
	virtual void SetCursor(ServerCursor *csr);

//	virtual void StrokeShape(SShape *sh, LayerData *d, const Pattern &pat);
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
	void BlitBitmap(ServerBitmap *sourcebmp, BRect sourcerect, BRect destrect, drawing_mode mode=B_OP_COPY);
	void ExtractToBitmap(ServerBitmap *destbmp, BRect destrect, BRect sourcerect);
	void SetPixelPattern(int x, int y, uint8 *pattern, uint8 patternindex);
	void HLine(int32 x1, int32 x2, int32 y, PatternHandler *pat);
	void HLineThick(int32 x1, int32 x2, int32 y, int32 thick, PatternHandler *pat);
	rgb_color GetBlitColor(rgb_color src, rgb_color dest, LayerData *d, bool use_high=true);
	void SetPixel(int x, int y, RGBColor col);
	void SetThickPixel(int x, int y, int thick, PatternHandler *pat);
	int OpenGraphicsDevice(int deviceNumber);
	int GetModeFromResolution(int width, int height, int space);
	int GetWidthFromMode(int mode);
	int GetHeightFromMode(int mode);
	int GetDepthFromMode(int mode);
	int GetDepthFromColorspace(int space);
	ServerCursor *cursor, *under_cursor;

	BRect cursorframe;
	int card_fd;
	image_id accelerant_image;
	GetAccelerantHook accelerant_hook;
        engine_token *mEngineToken;
        acquire_engine AcquireEngine;
        release_engine ReleaseEngine;
	fill_rectangle accFillRect;
	invert_rectangle accInvertRect;
	set_cursor_shape accSetCursorShape;
	move_cursor accMoveCursor;
	show_cursor accShowCursor;
	frame_buffer_config mFrameBufferConfig;
	int mode_count;
	display_mode *mode_list;
	display_mode mDisplayMode;
	display_mode R5DisplayMode; // This should go away once we stop running under r5
};

#endif
