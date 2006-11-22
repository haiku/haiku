/*
 * Copyright 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */

//!	BMenuWindow is a custom BWindow for BMenus.

#include <Menu.h>

#include <MenuPrivate.h>
#include <MenuWindow.h>
#include <WindowPrivate.h>


namespace BPrivate {

class BMenuScroller : public BView {
	public:
		BMenuScroller(BRect frame, BMenu *menu);
		virtual ~BMenuScroller();

		virtual void Pulse();
		virtual void Draw(BRect updateRect);

	private:
		BMenu *fMenu;
		BRect fUpperButton;
		BRect fLowerButton;

		float fValue;
		float fLimit;

		bool fUpperEnabled;
		bool fLowerEnabled;

		uint32 fButton;
		BPoint fPosition;
};

class BMenuFrame : public BView {
	public:
		BMenuFrame(BMenu *menu);
		virtual ~BMenuFrame();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void Draw(BRect updateRect);

  private:
		friend class BMenuWindow;

		BMenu *fMenu;
};

}	// namespace BPrivate

using namespace BPrivate;


const int kScrollerHeight = 10;
const int kScrollStep = 16;


BMenuScroller::BMenuScroller(BRect frame, BMenu *menu)
	: BView(frame, "menu scroller", 0,
		B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED),
	fMenu(menu),
	fUpperButton(0, 0, frame.right, kScrollerHeight),
	fLowerButton(0, frame.bottom - kScrollerHeight, frame.right, frame.bottom),
	fValue(0),
	fUpperEnabled(false),
	fLowerEnabled(true)
{
	if (!menu)
		debugger("BMenuScroller(): Scroller not attached to a menu!");
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));

	fLimit = menu->Bounds().Height() - (frame.Height() - 2 * kScrollerHeight);
}


BMenuScroller::~BMenuScroller()
{
}


void
BMenuScroller::Pulse()
{
	// To reduce the overhead even a little, fButton and fPosition were
	// made private variables.

	// We track the mouse and move the scrollers depending on its position.
	GetMouse(&fPosition, &fButton);
	if (fLowerButton.Contains(fPosition) && fLowerEnabled) {
		if (fValue == 0) {
			fUpperEnabled = true;
			
			Invalidate(fUpperButton);
		}

		if (fValue + kScrollStep >= fLimit) {
			// If we reached the limit, we don't want to scroll a whole
			// 'step' if not needed.
			fMenu->ScrollBy(0, fLimit - fValue);
			fValue = fLimit;
			fLowerEnabled = false;
			Invalidate(fLowerButton);
		} else {
			fMenu->ScrollBy(0, kScrollStep);
			fValue += kScrollStep;
		}
	} else if (fUpperButton.Contains(fPosition) && fUpperEnabled) {
		if (fValue == fLimit) {
			fLowerEnabled = true;
			Invalidate(fLowerButton);
		}

		if (fValue - kScrollStep <= 0) {
			fMenu->ScrollBy(0, -fValue);
			fValue = 0;
			fUpperEnabled = false;
			Invalidate(fUpperButton);
		} else {
			fMenu->ScrollBy(0, -kScrollStep);
			fValue -= kScrollStep;
		}

		// In this case, we need to redraw the lower button because of a
		// probable bug in ScrollBy handling. The scrolled view is drawing below 
		// its bounds, dirtying our button in this process.
		// Redrawing it everytime makes scrolling a little slower.
		// TODO: Try to find why and fix this.
		Draw(fLowerButton);
	}
}


void
BMenuScroller::Draw(BRect updateRect)
{
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));
	float middle = Bounds().right / 2;

	// Draw the upper arrow.
	if (updateRect.Intersects(fUpperButton)) {
		if (fUpperEnabled)
			SetHighColor(0, 0, 0);
		else
			SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), 
						B_DARKEN_2_TINT));

		FillRect(fUpperButton, B_SOLID_LOW);
	
		FillTriangle(BPoint(middle, (kScrollerHeight / 2) - 3),
				BPoint(middle + 5, (kScrollerHeight / 2) + 2),
				BPoint(middle - 5, (kScrollerHeight / 2) + 2));
	}

	// Draw the lower arrow.
	if (updateRect.Intersects(fLowerButton)) {
		if (fLowerEnabled)
			SetHighColor(0, 0, 0);
		else
			SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), 
						B_DARKEN_2_TINT));

		FillRect(fLowerButton, B_SOLID_LOW);

		FillTriangle(BPoint(middle, fLowerButton.bottom - (kScrollerHeight / 2) + 3),
				BPoint(middle + 5, fLowerButton.bottom - (kScrollerHeight / 2) - 2),
				BPoint(middle - 5, fLowerButton.bottom - (kScrollerHeight / 2) - 2));
	}
}


//	#pragma mark -


BMenuFrame::BMenuFrame(BMenu *menu)
	: BView(BRect(0, 0, 1, 1), "menu frame", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fMenu(menu)
{
}
	

BMenuFrame::~BMenuFrame()
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
		BPoint where((Bounds().Width() - fMenu->StringWidth(kEmptyMenuLabel)) / 2, ceilf(height.ascent + 1));
		DrawString(kEmptyMenuLabel, where);
	}

	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT));	
	BRect bounds(Bounds());

	StrokeLine(BPoint(bounds.right, bounds.top),
		BPoint(bounds.right, bounds.bottom - 1));
	StrokeLine(BPoint(bounds.left + 1, bounds.bottom),
		BPoint(bounds.right, bounds.bottom));
}


//	#pragma mark -


BMenuWindow::BMenuWindow(const char *name)
	// The window will be resized by BMenu, so just pass a dummy rect
	: BWindow(BRect(0, 0, 0, 0), name, B_BORDERED_WINDOW_LOOK, kMenuWindowFeel,
			B_NOT_ZOOMABLE | B_AVOID_FOCUS),
	fScroller(NULL),
	fMenuFrame(NULL)
{
	SetPulseRate(200000);
}


BMenuWindow::~BMenuWindow()
{
}


void
BMenuWindow::AttachMenu(BMenu *menu)
{
	if (fMenuFrame)
		debugger("BMenuWindow: a menu is already attached!");
	if (menu != NULL) {
		fMenuFrame = new BMenuFrame(menu);
		AddChild(fMenuFrame);
		menu->MakeFocus(true);
	}
}


void
BMenuWindow::DetachMenu()
{
	if (fMenuFrame) {
		if (fScroller) {
			DetachScrollers();
		} else {
			RemoveChild(fMenuFrame);
		}
		delete fMenuFrame;
		fMenuFrame = NULL;
	}
}


void
BMenuWindow::AttachScrollers()
{
	// We want to attach a scroller only if there's a menu frame already
	// existing.
	if (fScroller || !fMenuFrame)
		return;

	RemoveChild(fMenuFrame);
	fScroller = new BMenuScroller(Bounds(), fMenuFrame->fMenu);
	fScroller->AddChild(fMenuFrame);
	AddChild(fScroller);

	fMenuFrame->fMenu->MakeFocus(true);

	fMenuFrame->ResizeBy(0, -2 * kScrollerHeight);
	fMenuFrame->MoveBy(0, kScrollerHeight);
}


void
BMenuWindow::DetachScrollers()
{
	if(!fScroller || !fMenuFrame)
		return;

	// BeOS doesn't remember the position where the last scrolling ended,
	// so we just scroll back to the beginning.
	fMenuFrame->fMenu->ScrollTo(0, 0);

	fScroller->RemoveChild(fMenuFrame);
	RemoveChild(fScroller);

	delete fScroller;
	fScroller = NULL;
}
