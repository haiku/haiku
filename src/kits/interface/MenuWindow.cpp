/*
 * Copyright 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */

//!	BMenuWindow is a custom BWindow for BMenus.

// TODO: Add scrollers

#include <Menu.h>
#include <MenuWindow.h>

#include <WindowPrivate.h>


// This draws the frame around the BMenu
class BMenuFrame : public BView {
	public:
		BMenuFrame(BMenu *menu);

		virtual void AttachedToWindow();
		virtual void Draw(BRect updateRect);

	private:
		BMenu *fMenu;
};


BMenuWindow::BMenuWindow(const char *name, BMenu *menu)
	// The window will be resized by BMenu, so just pass a dummy rect
	: BWindow(BRect(0, 0, 0, 0), name, B_BORDERED_WINDOW_LOOK, kMenuWindowFeel,
			B_NOT_ZOOMABLE | B_AVOID_FOCUS),
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


//	#pragma mark -


BMenuFrame::BMenuFrame(BMenu *menu)
	: BView(BRect(0, 0, 1, 1), "menu frame", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
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
