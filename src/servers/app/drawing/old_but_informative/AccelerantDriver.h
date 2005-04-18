//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, Haiku, Inc.
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
//					Michael Lotz <mmlr@mlotz.ch>
//	Description:	A display driver which works directly with the
//					accelerant of the graphics card.
//  
//------------------------------------------------------------------------------
#ifndef _ACCELERANTDRIVER_H_
#define _ACCELERANTDRIVER_H_

#include <Accelerant.h>

#include "PatternHandler.h"
#include "FontServer.h"
#include "LayerData.h"
#include "DisplayDriverImpl.h"

class ServerBitmap;
class ServerCursor;

class AccelerantDriver : public DisplayDriverImpl {
public:
								AccelerantDriver();
								~AccelerantDriver();

virtual	bool					Initialize();
virtual	void					Shutdown();

/*
virtual	bool					Lock(bigtime_t timeout = B_INFINITE_TIMEOUT);
virtual	void					Unlock(void);
*/

virtual	void					SetMode(const int32 &mode);
virtual	void					SetMode(const display_mode &mode);

virtual	bool					DumpToFile(const char *path);
virtual	void					InvertRect(const BRect &r);
virtual	void					StrokeLineArray(const int32 &numlines, const LineArrayData *linedata,const DrawData *d);

/*
virtual	status_t				SetDPMSMode(const uint32 &state);
virtual	uint32					DPMSMode() const;
virtual	uint32					DPMSCapabilities() const;
*/

virtual	status_t				GetDeviceInfo(accelerant_device_info *info);
virtual	status_t				GetModeList(display_mode **mode_list, uint32 *count);
virtual	status_t				GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high);
virtual	status_t				GetTimingConstraints(display_timing_constraints *dtc);
virtual	status_t				ProposeMode(display_mode *candidate, const display_mode *low, const display_mode *high);
virtual	status_t				WaitForRetrace(bigtime_t timeout=B_INFINITE_TIMEOUT);

protected:
virtual bool					AcquireBuffer(FBBitmap *bmp);
virtual void					ReleaseBuffer(void);

		void					BlitBitmap(ServerBitmap *sourcebmp, BRect sourcerect, BRect destrect, drawing_mode mode=B_OP_COPY);
		void					ExtractToBitmap(ServerBitmap *destbmp, BRect destrect, BRect sourcerect);
		rgb_color				GetBlitColor(rgb_color src, rgb_color dest, LayerData *d, bool use_high=true);
		int						OpenGraphicsDevice(int deviceNumber);
		int						GetModeFromResolution(int width, int height, int space);
		int						GetWidthFromMode(int mode);
		int						GetHeightFromMode(int mode);
		int						GetDepthFromMode(int mode);
		int						GetDepthFromColorspace(int space);

	// Support functions for the rest of the driver
virtual	void					Blit(const BRect &src, const BRect &dest, const DrawData *d);
virtual	void					FillSolidRect(const BRect &rect, const RGBColor &color);
virtual	void					FillPatternRect(const BRect &rect, const DrawData *d);
virtual	void					StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color);
virtual	void					StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d);
virtual	void					StrokeSolidRect(const BRect &rect, const RGBColor &color);
virtual	void					CopyBitmap(ServerBitmap *bitmap, const BRect &source, const BRect &dest, const DrawData *d);
virtual	void					CopyToBitmap(ServerBitmap *target, const BRect &source); 

		ServerCursor			*cursor;
		ServerCursor			*under_cursor;
		BRect					cursorframe;

		int						card_fd;
		image_id				accelerant_image;
		GetAccelerantHook		accelerant_hook;
		engine_token			*mEngineToken;
		acquire_engine			AcquireEngine;
		release_engine			ReleaseEngine;
	
		// accelerant hooks
		fill_rectangle			accFillRect;
		invert_rectangle		accInvertRect;
		set_cursor_shape		accSetCursorShape;
		move_cursor				accMoveCursor;
		show_cursor				accShowCursor;
		screen_to_screen_blit	accScreenBlit;

		frame_buffer_config		mFrameBufferConfig;
		int						mode_count;
		display_mode			*mode_list;
		display_mode			R5DisplayMode; // This should go away once we stop running under r5
};

#endif
