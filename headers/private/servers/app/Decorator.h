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
//	File Name:		Decorator.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Base class for window decorators
//  
//------------------------------------------------------------------------------
#ifndef _DECORATOR_H_
#define _DECORATOR_H_

#include <SupportDefs.h>
#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <Window.h>
#include "LayerData.h"
#include "ColorSet.h"

class DisplayDriver;
class ServerFont;
class BRegion;

typedef enum { CLICK_NONE=0, CLICK_ZOOM, CLICK_CLOSE, CLICK_MINIMIZE,
	CLICK_TAB, CLICK_DRAG, CLICK_MOVETOBACK, CLICK_MOVETOFRONT, CLICK_SLIDETAB,
	
	CLICK_RESIZE, CLICK_RESIZE_L, CLICK_RESIZE_T, 
	CLICK_RESIZE_R, CLICK_RESIZE_B, CLICK_RESIZE_LT, CLICK_RESIZE_RT, 
	CLICK_RESIZE_LB, CLICK_RESIZE_RB } click_type;

class Decorator
{
public:
	Decorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags);
	virtual ~Decorator(void);

	void SetColors(const ColorSet &cset);
	void SetDriver(DisplayDriver *driver);
	void SetFlags(int32 wflags);
	void SetFeel(int32 wfeel);
	void SetFont(ServerFont *font);
	void SetLook(int32 wlook);
	
	void SetClose(bool is_down);
	void SetMinimize(bool is_down);
	void SetZoom(bool is_down);
	virtual void SetTitle(const char *string);

	int32 GetLook(void);
	int32 GetFeel(void);
	int32 GetFlags(void);
	const char *GetTitle(void);
		// we need to know its border(frame). WinBorder's _frame rect
		// must expand to include Decorator borders. Otherwise we can't
		// draw the border. We also add GetTabRect because I feel we'll need it
	BRect GetBorderRect(void);
	BRect GetTabRect(void);

	bool GetClose(void);
	bool GetMinimize(void);
	bool GetZoom(void);


	void SetFocus(bool is_active);
	bool GetFocus(void) { return _has_focus; };
	ColorSet GetColors(void) { return (_colors)?*_colors:ColorSet(); }
	
	virtual BRect SlideTab(float dx, float dy=0);
	virtual void GetFootprint(BRegion *region);
	virtual click_type Clicked(BPoint pt, int32 buttons, int32 modifiers);

	virtual void MoveBy(float x, float y);
	virtual void MoveBy(BPoint pt);
	virtual void ResizeBy(float x, float y);
	virtual void ResizeBy(BPoint pt);

	virtual void Draw(BRect r);
	virtual void Draw(void);
	virtual void DrawClose(void);
	virtual void DrawFrame(void);
	virtual void DrawMinimize(void);
	virtual void DrawTab(void);
	virtual void DrawTitle(void);
	virtual void DrawZoom(void);
	
protected:
	int32 _ClipTitle(float width);

	/*!
		\brief Returns the number of characters in the title
		\return The title character count
	*/
	int32 _TitleWidth(void) { return (_title_string)?_title_string->CountChars():0; }
	
	virtual void _DrawClose(BRect r);
	virtual void _DrawFrame(BRect r);
	virtual void _DrawMinimize(BRect r);
	virtual void _DrawTab(BRect r);
	virtual void _DrawTitle(BRect r);
	virtual void _DrawZoom(BRect r);
	virtual void _SetFocus(void);
	virtual void _DoLayout(void);
	virtual void _SetColors(void);
	
	ColorSet *_colors;
	DisplayDriver *_driver;
	LayerData _layerdata;
	int32 _look, _feel, _flags;
	BRect _zoomrect,_closerect,_minimizerect,_tabrect,_frame,
		_resizerect,_borderrect;

private:
	bool _close_state, _zoom_state, _minimize_state;
	bool _has_focus;
	BString *_title_string;
};

typedef float get_version(void);
typedef Decorator *create_decorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags);

#endif
