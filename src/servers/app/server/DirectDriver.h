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
//	File Name:		DirectDriver.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//
//	Description:	BDirectWindow graphics module
//  
//------------------------------------------------------------------------------
#ifndef DIRECTDRIVER_H_
#define DIRECTDRIVER_H_

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Region.h>	// for clipping_rect definition
#include <GraphicsCard.h>
#include <Message.h>
#include "DisplayDriver.h"
#include "PortLink.h"
#include <DirectWindow.h>
#include "PatternHandler.h"

class UtilityBitmap;
class SDWindow;
class LayerData;
class DirectDriver;

// This is very much not the fastest thing in the world - just quickly hacked out to get it going
// and make it relatively easy to optimize later. Ideally, rectangles stored in this should be
// stored in a singly-linked list and allocated items should be saved (and freed upon the object's
// destruction) to minimize calls to the OS' memory manager.
class RectPipe
{
public:
	RectPipe(void);
	~RectPipe(void);

	void PutRect(const BRect &rect);
	void PutRect(const clipping_rect &rect);
	bool GetRect(clipping_rect *rect);
	bool HasRects(void);
protected:
	BList list;
	BLocker lock;
	bool has_rects;
};

class DDView : public BView
{
public:
	DDView(BRect bounds);
	void MouseDown(BPoint pt);
	void MouseMoved(BPoint pt, uint32 transit, const BMessage *msg);
	void MouseUp(BPoint pt);
	void MessageReceived(BMessage *msg);
		
	PortLink serverlink;
};

class DDWindow : public BDirectWindow
{
public:
	DDWindow(uint16 width, uint16 height, color_space space, DirectDriver *owner);
	~DDWindow(void);

	virtual bool QuitRequested(void);
	virtual void DirectConnected(direct_buffer_info *info);
	static int32 DrawingThread(void *data);
	
	uint8 *fBits;
	int32 fRowBytes;
	color_space fFormat;
	clipping_rect fBounds;
	uint32 fNumClipRects;
	clipping_rect *fClipList;
	
	bool fDirty, fConnected, fConnectionDisabled;
	BLocker locker;
	thread_id fDrawThreadID;
	DisplayDriver *fOwner;
	BBitmap *framebuffer;
	clipping_rect screensize;
	RectPipe rectpipe;
};

/*!
	\brief BDirectWindow graphics module

	Last (and best) driver class in the app_server which is designed
	to provide fast graphics access for testing.
	
	The concept is to have a bitmap get written to which is used for double-buffering
	the video display. An updater thread simply locks access to this bitmap, copies data to
	the framebuffer, and releases access. Derived DisplayDriver functions operate on this bitmap.
*/
class DirectDriver : public DisplayDriver
{
public:
	DirectDriver(void);
	~DirectDriver(void);

	virtual	bool Initialize(void);
	virtual	void Shutdown(void);
	
	void DrawBitmap(ServerBitmap *bmp, const BRect &src, const BRect &dest, const DrawData *d);

	virtual	void InvertRect(const BRect &r);

	virtual void StrokeLineArray(BPoint *pts, const int32 &numlines, const DrawData *d, RGBColor *colors);

	virtual	void SetMode(const display_mode &mode);
	
	virtual	bool DumpToFile(const char *path);

	virtual	status_t SetDPMSMode(const uint32 &state);
	virtual	uint32 DPMSMode(void) const;
	virtual	uint32 DPMSCapabilities(void) const;
	virtual	status_t GetDeviceInfo(accelerant_device_info *info);
	virtual	status_t GetModeList(display_mode **mode_list, uint32 *count);
	virtual	status_t GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high);
	virtual	status_t GetTimingConstraints(display_timing_constraints *dtc);
	virtual	status_t ProposeMode(display_mode *candidate, const display_mode *low, const display_mode *high);
	virtual	status_t WaitForRetrace(bigtime_t timeout=B_INFINITE_TIMEOUT);

	BBitmap *framebuffer;
	display_mode fCurrentScreenMode, fSavedScreenMode;

protected:
	virtual	void FillSolidRect(const BRect &rect, const RGBColor &color);
	virtual	void FillPatternRect(const BRect &rect, const DrawData *d);
	virtual	void StrokeSolidRect(const BRect &rect, const RGBColor &color);
	virtual void StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color);
	virtual	void StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d);
	virtual	void CopyBitmap(ServerBitmap *bitmap, const BRect &source,
		const BRect &dest, const DrawData *d);
	virtual	void CopyToBitmap(ServerBitmap *target, const BRect &source);
	
	// Temporary function which will exist while we use BViews in the regular code
	void SetDrawData(const DrawData *d, bool set_font_data=false);
	
	// temporarily virtual - until clipping code is added in DisplayDriver
	virtual	void ConstrainClippingRegion(BRegion *reg);	

	rgb_color GetBlitColor(rgb_color src, rgb_color dest,DrawData *d, bool use_high=true);

	PortLink *serverlink;
	DDWindow *screenwin;
	BView *drawview;
};


#endif
