//------------------------------------------------------------------------------
//	Copyright (c) 2002-2004, OpenBeOS
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
//	File Name:		PrivateScreen.h
//	Author:			Stefano Ceccherini (burton666@libero.it)
//	Description:	BPrivateScreen is the class which does the real work
//					for the proxy class BScreen (it interacts with the app server).
//------------------------------------------------------------------------------

#ifndef __PRIVATESCREEN_H
#define __PRIVATESCREEN_H

#include <Accelerant.h>
#include <GraphicsDefs.h>

struct screen_desc;
class BApplication;
class BWindow;

class BPrivateScreen
{
	// Constructor and destructor are private. Use the static methods
	// CheckOut() and Return() instead.
public:
	static BPrivateScreen* CheckOut(BWindow *win);
	static BPrivateScreen* CheckOut(screen_id id);
	static void Return(BPrivateScreen *screen);
	
	status_t SetToNext();
	
	color_space ColorSpace();
	BRect Frame();
	screen_id ID();
	
	status_t WaitForRetrace(bigtime_t timeout);
	sem_id RetraceSemaphore();

	uint8 IndexForColor(uint8 red, uint8 green, uint8 blue, uint8 alpha);
	rgb_color ColorForIndex(const uint8 index);
	uint8 InvertIndex(uint8 index);
	
	const color_map *ColorMap();

	status_t GetBitmap(BBitmap **bitmap, bool drawCursor, BRect *bound);
	status_t ReadBitmap(BBitmap *bitmap, bool drawCursor, BRect *bound);
		
	rgb_color DesktopColor(uint32 index);
	void SetDesktopColor(rgb_color, uint32, bool);

	status_t ProposeMode(display_mode *target, const display_mode *low, const display_mode *high);
		
	status_t GetModeList(display_mode **mode_list, uint32 *count);
	status_t GetMode(uint32 workspace, display_mode *mode);
	status_t SetMode(uint32 workspace, display_mode *mode, bool makeDefault);
	status_t GetDeviceInfo(accelerant_device_info *info);
	status_t GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high);
	status_t GetTimingConstraints(display_timing_constraints *dtc);
	status_t SetDPMS(uint32 dpmsState);
	uint32 DPMSState();
	uint32 DPMSCapabilites();
	
	void* BaseAddress();
	uint32 BytesPerRow();
	
	status_t get_screen_desc(screen_desc *desc);
	
private:

	friend class BApplication;	
	
	color_map *fColorMap;
	sem_id fRetraceSem;
	bool fOwnsColorMap;

	BPrivateScreen();
	~BPrivateScreen();
};

#endif // __PRIVATESCREEN_H
