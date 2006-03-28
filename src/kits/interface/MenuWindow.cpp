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

#include <MenuPrivate.h>
#include <MenuWindow.h>
#include <WindowPrivate.h>


// This draws the frame around the BMenu
class BMenuFrame : public BView {
	public:
		BMenuFrame(BMenu *menu);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void Draw(BRect updateRect);

	private:
		BMenu *fMenu;
};


BMenuWindow::BMenuWindow(const char *name)
	// The window will be resized by BMenu, so just pass a dummy rect
	: BWindow(BRect(0, 0, 0, 0), name, B_BORDERED_WINDOW_LOOK, kMenuWindowFeel,
			B_NOT_ZOOMABLE | B_AVOID_FOCUS),
	fUpperScroller(NULL),
	fLowerScroller(NULL)
{
}


BMenuWindow::~BMenuWindow()
{
}


void
BMenuWindow::AttachMenu(BMenu *menu)
{
	if (CountChildren() > 0)
		debugger("BMenuWindow (%s): a menu is already attached!");
	if (menu != NULL) {
		BMenuFrame *menuFrame = new BMenuFrame(menu);
		AddChild(menuFrame);
	}
}


void
BMenuWindow::DetachMenu()
{
	if (CountChildren() > 0)
		RemoveChild(ChildAt(0));	
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
	
	if (fMenu != NULL)
		AddChild(fMenu);
		
	ResizeTo(Window()->Bounds().Width(), Window()->Bounds().Height());
	if (fMenu != NULL) {
		BFont font;
		fMenu->GetFont(&font);
		SetFont(&font);
	}
}
	

void
BMenuFrame::DetachedFromWindow()
{
	if (fMenu != NULL)
		RemoveChild(fMenu);
}


void
BMenuFrame::Draw(BRect updateRect)
{
	if (fMenu != NULL && fMenu->CountItems() == 0) {
		// TODO: Review this as it's a bit hacky.
		// Menu has a size of 0, 0, since there are no items in it.
		// So the BMenuFrame class has to fake it and draw an empty item.
		// Note that we can't add a real "empty" item because then we couldn't
		// tell if the item was added by us or not.
		// See also BMenu::UpdateWindowViewSize()
		SetHighColor(ui_color(B_MENU_BACKGROUND_COLOR));
		SetLowColor(HighColor());
		FillRect(updateRect);

		font_height height;
		GetFontHeight(&height);
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DISABLED_LABEL_TINT));
		DrawString(kEmptyMenuLabel, BPoint(3, ceilf(height.ascent + 2)));
	}

	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT));	
	BRect bounds(Bounds());

	StrokeLine(BPoint(bounds.right, bounds.top),
		BPoint(bounds.right, bounds.bottom - 1));
	StrokeLine(BPoint(bounds.left + 1, bounds.bottom),
		BPoint(bounds.right, bounds.bottom));
}
