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
#include "LayerData.h"

class ServerBitmap;
class ServerCursor;

class AccelerantDriver : public DisplayDriver
{
public:
	AccelerantDriver(void);
	~AccelerantDriver(void);

	bool Initialize(void);
	void Shutdown(void);
	
	virtual void InvertRect(const BRect &r);
	virtual void SetMode(const int32 &mode);
	virtual void SetMode(const display_mode &mode);
	virtual bool DumpToFile(const char *path);
	virtual void StrokeLineArray(BPoint *pts, const int32 &numlines, const DrawData *d, RGBColor *colors);

/*
	virtual status_t SetDPMSMode(const uint32 &state);
	virtual uint32 DPMSMode(void) const;
	virtual uint32 DPMSCapabilities(void) const;
	virtual status_t GetDeviceInfo(accelerant_device_info *info);
	virtual status_t GetModeList(display_mode **mode_list, uint32 *count);
	virtual status_t GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high);
	virtual status_t GetTimingConstraints(display_timing_constraints *dtc);
	virtual status_t ProposeMode(display_mode *candidate, const display_mode *low, const display_mode *high);
	virtual status_t WaitForRetrace(bigtime_t timeout=B_INFINITE_TIMEOUT);
*/

protected:
	void BlitBitmap(ServerBitmap *sourcebmp, BRect sourcerect, BRect destrect, drawing_mode mode=B_OP_COPY);
	void ExtractToBitmap(ServerBitmap *destbmp, BRect destrect, BRect sourcerect);
	//void SetPixelPattern(int x, int y, uint8 *pattern, uint8 patternindex);
	//void HLine(int32 x1, int32 x2, int32 y, PatternHandler *pat);
	//void HLineThick(int32 x1, int32 x2, int32 y, int32 thick, PatternHandler *pat);
	void HLinePatternThick(int32 x1, int32 x2, int32 y);
	void VLinePatternThick(int32 x, int32 y1, int32 y2);
	void FillSolidRect(int32 left, int32 top, int32 right, int32 bottom);
	void FillPatternRect(int32 left, int32 top, int32 right, int32 bottom);
	rgb_color GetBlitColor(rgb_color src, rgb_color dest, LayerData *d, bool use_high=true);
	//void SetPixel(int x, int y, RGBColor col);
	//void SetThickPixel(int x, int y, int thick, PatternHandler *pat);
	void SetThickPatternPixel(int x, int y);
	int OpenGraphicsDevice(int deviceNumber);
	int GetModeFromResolution(int width, int height, int space);
	int GetWidthFromMode(int mode);
	int GetHeightFromMode(int mode);
	int GetDepthFromMode(int mode);
	int GetDepthFromColorspace(int space);

	// Support functions for the rest of the driver
	void Blit(const BRect &src, const BRect &dest, const DrawData *d);
	void FillSolidRect(const BRect &rect, RGBColor &color);
	void FillPatternRect(const BRect &rect, const DrawData *d);
	void StrokeSolidLine(const BPoint &start, const BPoint &end, RGBColor &color);
	void StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d);
	void StrokeSolidRect(const BRect &rect, RGBColor &color);
	void CopyBitmap(ServerBitmap *bitmap, const BRect &source, const BRect &dest, const DrawData *d);
	void CopyToBitmap(ServerBitmap *target, const BRect &source);
	
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
