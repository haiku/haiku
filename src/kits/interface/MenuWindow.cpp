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
	BMenuFrame(BMenu *menu) ;
	
	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);

private:
	BMenu *fMenu;
};


BMenuWindow::BMenuWindow(const char *name, BMenu *menu)
	:
	// The window will be resized by BMenu, so just pass a dummy rect
	//BWindow(BRect(0, 0, 0, 0), name, B_BORDERED_WINDOW_LOOK, kMenuWindowFeel,
	//		B_NOT_ZOOMABLE),
	BWindow(BRect(0, 0, 0, 0), name, B_BORDERED_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
			B_NOT_ZOOMABLE|B_AVOID_FRONT|B_AVOID_FOCUS),
	fUpperScroller(NULL),
	fLowerScroller(NULL)
{
	SetMenu(menu);
}


BMenuWindow::~BMenuWindow()
{
}


void
BMenuWindow::SetMenu(BMenu *menu)
{
	if (CountChildren() > 0)
		RemoveChild(ChildAt(0));
	BMenuFrame *menuFrame = new BMenuFrame(menu);
	AddChild(menuFrame);
}


// BMenuFrame
BMenuFrame::BMenuFrame(BMenu *menu)
	:
	BView(BRect(0, 0, 1, 1), "menu frame", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fMenu(menu)
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
	if (fMenu->CountItems() == 0) {
		SetHighColor(ui_color(B_MENU_BACKGROUND_COLOR));
		FillRect(updateRect);
		font_height height;
		fMenu->GetFontHeight(&height);
		fMenu->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DISABLED_LABEL_TINT));
		// TODO: This doesn't get drawn for some reason
		fMenu->DrawString("<empty>", BPoint(2, ceilf(height.ascent + 2)));
	}

	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT));	
	BRect bounds(Bounds());
	StrokeLine(BPoint(bounds.right, bounds.top),
				BPoint(bounds.right, bounds.bottom - 1));
	StrokeLine(BPoint(bounds.left + 1, bounds.bottom),
				BPoint(bounds.right, bounds.bottom));
}
