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
	virtual void DrawBitmap(ServerBitmap *bmp, const BRect &src, const BRect &dest, DrawData *d);

	virtual void SetMode(const int32 &mode);
	virtual void SetMode(const display_mode &mode);
	virtual void InvertRect(const BRect &rect);
protected:
	virtual bool AcquireBuffer(FBBitmap *bmp);
	virtual void ReleaseBuffer(void);

	virtual void Blit(const BRect &src, const BRect &dest, const DrawData *d);
	virtual void FillSolidRect(const BRect &rect, const RGBColor &color);
	virtual void FillPatternRect(const BRect &rect, const DrawData *d);
	virtual void StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color);
	virtual void StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d);
	virtual void StrokeSolidRect(const BRect &rect, const RGBColor &color);
	virtual void CopyBitmap(ServerBitmap *bitmap, const BRect &source, const BRect &dest, const DrawData *d);
	virtual void CopyToBitmap(ServerBitmap *target, const BRect &source);

	void ExtractToBitmap(ServerBitmap *destbmp, BRect destrect, BRect sourcerect);
	rgb_color GetBlitColor(rgb_color src, rgb_color dest, DrawData *d, bool use_high=true);
	void HLinePatternThick(int32 x1, int32 x2, int32 y);
	void VLinePatternThick(int32 x, int32 y1, int32 y2);
//	void FillSolidRect(int32 left, int32 top, int32 right, int32 bottom);
//	void FillPatternRect(int32 left, int32 top, int32 right, int32 bottom);
	void SetThickPatternPixel(int x, int y);
	ServerBitmap *_target;
};

#endif
