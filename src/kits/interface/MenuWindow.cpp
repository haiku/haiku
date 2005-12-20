//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
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
//	File Name:		MenuWindow.cpp
//	Authors:		Marc Flerackers (mflerackers@androme.be)
//					Stefano Ceccherini (burton666@libero.it)
//	Description:	BMenuWindow is a custom BWindow for BMenus.
//------------------------------------------------------------------------------
// TODO: Add scrollers

#include <Menu.h>
#include <MenuWindow.h>

// TODO: taken from Deskbar's WindowMenu.cpp.
// this should go to some private header.
const window_feel kMenuWindowFeel = (window_feel)1025;

// This draws the frame around the BMenu
class BMenuFrame : public BView {
public:
	BMenuFrame() ;
	
	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
};


BMenuWindow::BMenuWindow(const char *name)
	:
	// The window will be resized by BMenu, so just pass a dummy rect
	//BWindow(BRect(0, 0, 0, 0), name, B_BORDERED_WINDOW_LOOK, kMenuWindowFeel,
	//		B_NOT_ZOOMABLE),
	BWindow(BRect(0, 0, 0, 0), name, B_BORDERED_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
			B_NOT_ZOOMABLE|B_AVOID_FRONT|B_AVOID_FOCUS),
	fUpperScroller(NULL),
	fLowerScroller(NULL)
{
	BMenuFrame *menuFrame = new BMenuFrame();
	AddChild(menuFrame);
}


BMenuWindow::~BMenuWindow()
{
}


// BMenuFrame
BMenuFrame::BMenuFrame()
	:
	BView(BRect(0, 0, 1, 1), "menu frame", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}
	

void
BMenuFrame::AttachedToWindow()
{
	BView::AttachedToWindow();
	ResizeTo(Window()->Bounds().Width(), Window()->Bounds().Height());
}
	

void
BMenuFrame::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	
	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT));
	StrokeLine(BPoint(bounds.right, bounds.top),
				BPoint(bounds.right, bounds.bottom - 1));
	StrokeLine(BPoint(bounds.left + 1, bounds.bottom),
				BPoint(bounds.right, bounds.bottom));
}
