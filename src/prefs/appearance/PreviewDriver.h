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
//	File Name:		PreviewDriver.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class to display decorators from regular BApplications
//  
//------------------------------------------------------------------------------
#ifndef _PREVIEWDRIVER_H_
#define _PREVIEWDRIVER_H_

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <GraphicsCard.h>
#include <Cursor.h>
#include <Message.h>
#include <Rect.h>
#include <Region.h>
#include "DisplayDriver.h"

class BBitmap;
class PortLink;
class ServerCursor;
class ColorSet;

class PVView : public BView
{
public:
	PVView(const BRect &bounds);
	~PVView(void);
	void AttachedToWindow(void);
	void DetachedFromWindow(void);
	void Draw(BRect rect);

	BBitmap *viewbmp;
	BWindow *win;
};

class PreviewDriver : public DisplayDriver
{
public:
	PreviewDriver(void);
	~PreviewDriver(void);

	bool Initialize(void);		// Sets the driver
	void Shutdown(void);		// You never know when you'll need this
	
	// Drawing functions
	void DrawBitmap(ServerBitmap *bmp, const BRect &src, const BRect &dest, const DrawData *d);

	void InvertRect(const BRect &r);

	virtual void StrokeLineArray(const int32 &numlines, const LineArrayData *linedata,const DrawData *d);

	void SetMode(const int32 &mode);
	void SetMode(const display_mode &mode);
	
	BView *View(void) { return (BView*)view; };
	virtual bool Lock(bigtime_t timeout=B_INFINITE_TIMEOUT);
	virtual void Unlock(void);

protected:
	virtual void FillSolidRect(const BRect &rect, const RGBColor &color);
	virtual void FillPatternRect(const BRect &rect, const DrawData *d);
	virtual void StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color);
	virtual void StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d);
	virtual void StrokeSolidRect(const BRect &rect, const RGBColor &color);

	virtual bool AcquireBuffer(FBBitmap *bmp);
	virtual void ReleaseBuffer(void);

	BBitmap *framebuffer;
	BView *drawview;

	// Region used to track of invalid areas for the Begin/EndLineArray functions
	BRegion laregion;
	PVView *view;
};

extern BRect preview_bounds;

#endif
