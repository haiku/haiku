//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		WinBorder.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@mymail.ro>
//	Description:	Layer subclass which handles window management
//  
//------------------------------------------------------------------------------
#ifndef _WINBORDER_H_
#define _WINBORDER_H_

#include <Rect.h>
#include <String.h>
#include "Layer.h"
#include "FMWList.h"
#include "Decorator.h"

// these are used by window manager to properly place window.
enum {
	B_SYSTEM_LAST = -10L,
	B_FLOATING_APP = 0L,
	B_MODAL_APP = 1L,
	B_NORMAL = 2L,
	B_FLOATING_ALL = 3L,
	B_MODAL_ALL = 4L,
	B_SYSTEM_FIRST = 10L,
};

class ServerWindow;
class Decorator;
class DisplayDriver;
class Desktop;

class PointerEvent
{
	public:
	int32 code;	//B_MOUSE_UP, B_MOUSE_DOWN, B_MOUSE_MOVED
			//B_MOUSE_WHEEL_CHANGED
	bigtime_t when;
	BPoint where;
	float wheel_delta_x;
	float wheel_delta_y;
	int32 modifiers;
	int32 buttons;	//B_PRIMARY_MOUSE_BUTTON, B_SECONDARY_MOUSE_BUTTON
			//B_TERTIARY_MOUSE_BUTTON
	int32 clicks;
};

class WinBorder : public Layer
{
public:
	WinBorder(const BRect &r, const char *name, const int32 look,const int32 feel, 
			const int32 flags, ServerWindow *win, DisplayDriver *driver);
	virtual	~WinBorder(void);
	
	virtual	void Draw(const BRect &r);
	
	virtual	void MoveBy(float x, float y);
	virtual	void ResizeBy(float x, float y);

	virtual	void RebuildFullRegion(void);

	void SetSizeLimits(float minwidth, float maxwidth, float minheight, float maxheight);

	click_type TellWhat(PointerEvent& evt) const;
	void MouseDown(click_type action);
	void MouseMoved(click_type action);
	void MouseUp(click_type action);
	void MouseWheel(PointerEvent& evt, BPoint& ptWhere);
	
	void UpdateColors(void);
	void UpdateDecorator(void);
	void UpdateFont(void);
	void UpdateScreen(void);
	
	virtual bool HasClient(void) { return false; }
	Decorator *GetDecorator(void) const { return fDecorator; }

	void HighlightDecorator(const bool &active);
	
	bool HasPoint(const BPoint &pt) const;
	
	FMWList		fFMWList;

private:
	void SetLevel();

protected:
	friend class Layer;
	friend class ServerWindow;
	friend class RootLayer;

	Decorator *fDecorator;
	Layer *fTopLayer;

	int32 fMouseButtons;
	int32 fKeyModifiers;
	BPoint fLastMousePosition;

	bool fIsClosing;
	bool fIsMinimizing;
	bool fIsZooming;
	
	float fMinWidth, fMaxWidth;
	float fMinHeight, fMaxHeight;
};

#endif
