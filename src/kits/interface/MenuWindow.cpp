/*
 * Copyright 2001-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

//!	BMenuWindow is a custom BWindow for BMenus.

#include <MenuWindow.h>

#include <ControlLook.h>
#include <Debug.h>
#include <Menu.h>
#include <MenuItem.h>

#include <MenuPrivate.h>
#include <WindowPrivate.h>


namespace BPrivate {

class BMenuScroller : public BView {
public:
							BMenuScroller(BRect frame);

			bool			IsEnabled() const;
			void			SetEnabled(bool enabled);

private:
			bool			fEnabled;
};


class BMenuFrame : public BView {
public:
							BMenuFrame(BMenu* menu);

	virtual	void			AttachedToWindow();
	virtual	void			DetachedFromWindow();
	virtual	void			Draw(BRect updateRect);

private:
	friend class BMenuWindow;

			BMenu*			fMenu;
};


class UpperScroller : public BMenuScroller {
public:
							UpperScroller(BRect frame);

	virtual	void			Draw(BRect updateRect);
};


class LowerScroller : public BMenuScroller {
public:
							LowerScroller(BRect frame);

	virtual	void			Draw(BRect updateRect);
};


}	// namespace BPrivate


using namespace BPrivate;


const int kScrollerHeight = 12;


BMenuScroller::BMenuScroller(BRect frame)
	:
	BView(frame, "menu scroller", 0, B_WILL_DRAW | B_FRAME_EVENTS
		| B_FULL_UPDATE_ON_RESIZE),
	fEnabled(false)
{
	SetViewUIColor(B_MENU_BACKGROUND_COLOR);
}


bool
BMenuScroller::IsEnabled() const
{
	return fEnabled;
}


void
BMenuScroller::SetEnabled(bool enabled)
{
	fEnabled = enabled;
}


//	#pragma mark -


UpperScroller::UpperScroller(BRect frame)
	:
	BMenuScroller(frame)
{
}


void
UpperScroller::Draw(BRect updateRect)
{
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));
	float middle = Bounds().right / 2;

	// Draw the upper arrow.
	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else {
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
	}

	FillRect(Bounds(), B_SOLID_LOW);

	FillTriangle(BPoint(middle, (kScrollerHeight / 2) - 3),
		BPoint(middle + 5, (kScrollerHeight / 2) + 2),
		BPoint(middle - 5, (kScrollerHeight / 2) + 2));
}


//	#pragma mark -


LowerScroller::LowerScroller(BRect frame)
	:
	BMenuScroller(frame)
{
}


void
LowerScroller::Draw(BRect updateRect)
{
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));

	BRect frame = Bounds();
	// Draw the lower arrow.
	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else {
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
	}

	FillRect(frame, B_SOLID_LOW);

	float middle = Bounds().right / 2;

	FillTriangle(BPoint(middle, frame.bottom - (kScrollerHeight / 2) + 3),
		BPoint(middle + 5, frame.bottom - (kScrollerHeight / 2) - 2),
		BPoint(middle - 5, frame.bottom - (kScrollerHeight / 2) - 2));
}


//	#pragma mark -


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
		BRect rect(Bounds());
		be_control_look->DrawMenuBackground(this, rect, updateRect,
			ui_color(B_MENU_BACKGROUND_COLOR));
		SetDrawingMode(B_OP_OVER);

		// TODO: Review this as it's a bit hacky.
		// Since there are no items in this menu, its size is 0x0.
		// To show an empty BMenu, we use BMenuFrame to draw an empty item.
		// It would be nice to simply add a real "empty" item, but in that case
		// we couldn't tell if the item was added by us or not, and applications
		// could break (because CountItems() would return 1 for an empty BMenu).
		// See also BMenu::UpdateWindowViewSize()
		font_height height;
		GetFontHeight(&height);
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DISABLED_LABEL_TINT));
		BPoint where(
			(Bounds().Width() - fMenu->StringWidth(kEmptyMenuLabel)) / 2,
			ceilf(height.ascent + 1));
		DrawString(kEmptyMenuLabel, where);
	}
}



//	#pragma mark -


BMenuWindow::BMenuWindow(const char *name)
	// The window will be resized by BMenu, so just pass a dummy rect
	:
	BWindow(BRect(0, 0, 0, 0), name, B_BORDERED_WINDOW_LOOK, kMenuWindowFeel,
		B_NOT_MOVABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AVOID_FOCUS
			| kAcceptKeyboardFocusFlag),
	fMenu(NULL),
	fMenuFrame(NULL),
	fUpperScroller(NULL),
	fLowerScroller(NULL),
	fScrollStep(19)
{
	SetSizeLimits(2, 10000, 2, 10000);
}


BMenuWindow::~BMenuWindow()
{
	DetachMenu();
}


void
BMenuWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
	BWindow::DispatchMessage(message, handler);
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
		fMenu = menu;
	}
}


void
BMenuWindow::DetachMenu()
{
	DetachScrollers();
	if (fMenuFrame) {
		RemoveChild(fMenuFrame);
		delete fMenuFrame;
		fMenuFrame = NULL;
		fMenu = NULL;
	}
}


void
BMenuWindow::AttachScrollers()
{
	// We want to attach a scroller only if there's a
	// menu frame already existing.
	if (!fMenu || !fMenuFrame)
		return;

	fMenu->MakeFocus(true);

	BRect frame = Bounds();
	float newLimit = fMenu->Bounds().Height()
		- (frame.Height() - 2 * kScrollerHeight);

	if (!HasScrollers())
		fValue = 0;
	else if (fValue > newLimit)
		_ScrollBy(newLimit - fValue);

	fLimit = newLimit;

	if (fUpperScroller == NULL) {
		fUpperScroller = new UpperScroller(
			BRect(0, 0, frame.right, kScrollerHeight - 1));
		AddChild(fUpperScroller);
	}

	if (fLowerScroller == NULL) {
		fLowerScroller = new LowerScroller(
			BRect(0, frame.bottom - kScrollerHeight + 1, frame.right,
				frame.bottom));
		AddChild(fLowerScroller);
	}

	fUpperScroller->ResizeTo(frame.right, kScrollerHeight - 1);
	fLowerScroller->ResizeTo(frame.right, kScrollerHeight - 1);

	fUpperScroller->SetEnabled(fValue > 0);
	fLowerScroller->SetEnabled(fValue < fLimit);

	fMenuFrame->ResizeTo(frame.Width(), frame.Height() - 2 * kScrollerHeight);
	fMenuFrame->MoveTo(0, kScrollerHeight);
}


void
BMenuWindow::DetachScrollers()
{
	// BeOS doesn't remember the position where the last scrolling ended,
	// so we just scroll back to the beginning.
	if (fMenu)
		fMenu->ScrollTo(0, 0);

	if (fLowerScroller) {
		RemoveChild(fLowerScroller);
		delete fLowerScroller;
		fLowerScroller = NULL;
	}

	if (fUpperScroller) {
		RemoveChild(fUpperScroller);
		delete fUpperScroller;
		fUpperScroller = NULL;
	}

	BRect frame = Bounds();

	if (fMenuFrame != NULL) {
		fMenuFrame->ResizeTo(frame.Width(), frame.Height());
		fMenuFrame->MoveTo(0, 0);
	}
}


void
BMenuWindow::SetSmallStep(float step)
{
	fScrollStep = step;
}


void
BMenuWindow::GetSteps(float* _smallStep, float* _largeStep) const
{
	if (_smallStep != NULL)
		*_smallStep = fScrollStep;
	if (_largeStep != NULL) {
		if (fMenuFrame != NULL)
			*_largeStep = fMenuFrame->Bounds().Height() - fScrollStep;
		else
			*_largeStep = fScrollStep * 2;
	}
}


bool
BMenuWindow::HasScrollers() const
{
	return fMenuFrame != NULL && fUpperScroller != NULL
		&& fLowerScroller != NULL;
}


bool
BMenuWindow::CheckForScrolling(const BPoint &cursor)
{
	if (!fMenuFrame || !fUpperScroller || !fLowerScroller)
		return false;

	return _Scroll(cursor);
}


bool
BMenuWindow::TryScrollBy(const float& step)
{
	if (!fMenuFrame || !fUpperScroller || !fLowerScroller)
		return false;

	_ScrollBy(step);
	return true;
}


bool
BMenuWindow::TryScrollTo(const float& where)
{
	if (!fMenuFrame || !fUpperScroller || !fLowerScroller)
		return false;

	_ScrollBy(where - fValue);
	return true;
}


bool
BMenuWindow::_Scroll(const BPoint& where)
{
	ASSERT((fLowerScroller != NULL));
	ASSERT((fUpperScroller != NULL));

	const BPoint cursor = ConvertFromScreen(where);
	const BRect &lowerFrame = fLowerScroller->Frame();
	const BRect &upperFrame = fUpperScroller->Frame();

	int32 delta = 0;
	if (fLowerScroller->IsEnabled() && lowerFrame.Contains(cursor))
		delta = 1;
	else if (fUpperScroller->IsEnabled() && upperFrame.Contains(cursor))
		delta = -1;

	if (delta == 0)
		return false;

	float smallStep;
	GetSteps(&smallStep, NULL);
	_ScrollBy(smallStep * delta);

	snooze(5000);

	return true;
}


void
BMenuWindow::_ScrollBy(const float& step)
{
	if (step > 0) {
		if (fValue == 0) {
			fUpperScroller->SetEnabled(true);
			fUpperScroller->Invalidate();
		}

		if (fValue + step >= fLimit) {
			// If we reached the limit, only scroll to the end
			fMenu->ScrollBy(0, fLimit - fValue);
			fValue = fLimit;
			fLowerScroller->SetEnabled(false);
			fLowerScroller->Invalidate();
		} else {
			fMenu->ScrollBy(0, step);
			fValue += step;
		}
	} else if (step < 0) {
		if (fValue == fLimit) {
			fLowerScroller->SetEnabled(true);
			fLowerScroller->Invalidate();
		}

		if (fValue + step <= 0) {
			fMenu->ScrollBy(0, -fValue);
			fValue = 0;
			fUpperScroller->SetEnabled(false);
			fUpperScroller->Invalidate();
		} else {
			fMenu->ScrollBy(0, step);
			fValue += step;
		}
	}
}

