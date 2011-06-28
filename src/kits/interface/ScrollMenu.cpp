/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		John Scipione (jscipione@gmail.com)
 */


#include <ScrollMenu.h>

#include <ControlLook.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <Layout.h>
#include <GroupLayout.h>
#include <Menu.h>
#include <Screen.h>

#include <MenuPrivate.h>


const char* kEmptyMenuLabel = "<empty>";
const int kDefaultScrollStep = 19;
const int kScrollerHeight = 12;


class BMenuScroller : public BView {
public:
							BMenuScroller(BRect frame);
	virtual					~BMenuScroller();

			bool			IsEnabled() const { return fEnabled; };
			void			SetEnabled(bool enabled);

private:
			bool			fEnabled;
};


class BMenuUpScroller : public BMenuScroller {
public:
							BMenuUpScroller(BRect frame);
	virtual					~BMenuUpScroller();

	virtual	void			Draw(BRect updateRect);
};


class BMenuDownScroller : public BMenuScroller {
public:
							BMenuDownScroller(BRect frame);
	virtual					~BMenuDownScroller();

	virtual	void			Draw(BRect updateRect);
};


//	#pragma mark -


BMenuScroller::BMenuScroller(BRect frame)
	:
	BView(frame, "menu scroll arrow", 0, B_WILL_DRAW | B_FRAME_EVENTS),
	fEnabled(false)
{
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
}


BMenuScroller::~BMenuScroller()
{
}


void
BMenuScroller::SetEnabled(bool enabled)
{
	fEnabled = enabled;
	Invalidate();
}


//	#pragma mark -


BMenuUpScroller::BMenuUpScroller(BRect frame)
	:
	BMenuScroller(frame)
{
}


BMenuUpScroller::~BMenuUpScroller()
{
}


void
BMenuUpScroller::Draw(BRect updateRect)
{
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));

	// Draw the upper arrow.
	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else {
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
	}

	FillRect(Bounds(), B_SOLID_LOW);

	float middle = Bounds().right / 2;
	FillTriangle(BPoint(middle, (kScrollerHeight / 2) - 3),
		BPoint(middle + 5, (kScrollerHeight / 2) + 2),
		BPoint(middle - 5, (kScrollerHeight / 2) + 2));
}


//	#pragma mark -


BMenuDownScroller::BMenuDownScroller(BRect frame)
	:
	BMenuScroller(frame)
{
}


BMenuDownScroller::~BMenuDownScroller()
{
}


void
BMenuDownScroller::Draw(BRect updateRect)
{
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));

	// Draw the lower arrow.
	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else {
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
	}

	BRect frame = Bounds();
	FillRect(frame, B_SOLID_LOW);

	float middle = Bounds().right / 2;
	FillTriangle(BPoint(middle, frame.bottom - (kScrollerHeight / 2) + 3),
		BPoint(middle + 5, frame.bottom - (kScrollerHeight / 2) - 2),
		BPoint(middle - 5, frame.bottom - (kScrollerHeight / 2) - 2));
}


//	#pragma mark -


BScrollMenu::BScrollMenu(BMenu *menu)
	:
	BView("menu scroll view", B_WILL_DRAW | B_FRAME_EVENTS),
	fMenu(menu),
	fUpperScroller(NULL),
	fLowerScroller(NULL),
	fScrollStep(kDefaultScrollStep)
{
}


BScrollMenu::~BScrollMenu()
{
	if (fMenu != NULL) {
		fMenu->RemoveSelf();
		delete fMenu;
		fMenu = NULL;
	}

	if (fUpperScroller != NULL) {
		fUpperScroller->RemoveSelf();
		delete fUpperScroller;
		fUpperScroller = NULL;
	}

	if (fLowerScroller != NULL) {
		fLowerScroller->RemoveSelf();
		delete fLowerScroller;
		fLowerScroller = NULL;
	}
}


void
BScrollMenu::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (fMenu == NULL)
		return;

	AddChild(fMenu);

	// Move the scroll menu into the right position
	MoveTo(fMenu->Frame().LeftTop());

	BFont font;
	fMenu->GetFont(&font);
	SetFont(&font);
}


void
BScrollMenu::DetachedFromWindow()
{
	BView::DetachedFromWindow();

	if (fMenu != NULL)
		fMenu->RemoveSelf();

	if (fUpperScroller != NULL)
		fUpperScroller->RemoveSelf();

	if (fLowerScroller != NULL)
		fLowerScroller->RemoveSelf();
}


void
BScrollMenu::Draw(BRect updateRect)
{
	if (be_control_look != NULL)
		return;

	SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_2_TINT));
	BRect bounds(Bounds());

	StrokeLine(BPoint(bounds.right, bounds.top),
		BPoint(bounds.right, bounds.bottom - 1));
	StrokeLine(BPoint(bounds.left + 1, bounds.bottom),
		BPoint(bounds.right, bounds.bottom));
}


void
BScrollMenu::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);

	if (fMenu != NULL) {
		if (HasScrollers())
			fMenu->MoveTo(0, kScrollerHeight);
		else
			fMenu->MoveTo(0, 0);
	}
}


void
BScrollMenu::AttachScrollers()
{
	if (fMenu == NULL)
		return;

	BRect frame = Bounds();
	BRect screenFrame = (BScreen(Window())).Frame();

	if (HasScrollers()) {
		fLimit = Frame().bottom + 2 * kScrollerHeight - screenFrame.bottom;
		return;
	}

	fMenu->MakeFocus(true);

	if (fUpperScroller == NULL) {
		fUpperScroller = new BMenuUpScroller(
			BRect(0, 0, frame.right, kScrollerHeight - 1));
		AddChild(fUpperScroller, fMenu);
	}

	if (fLowerScroller == NULL) {
		fLowerScroller = new BMenuDownScroller(
			BRect(0, frame.bottom - kScrollerHeight + 1, frame.right,
				frame.bottom));
		AddChild(fLowerScroller);
	}

	fUpperScroller->SetEnabled(false);
	fLowerScroller->SetEnabled(true);

	fLimit = Frame().bottom + 2 * kScrollerHeight - screenFrame.bottom;
	fValue = 0;
}


void
BScrollMenu::DetachScrollers()
{
	if (!HasScrollers())
		return;

	if (fLowerScroller) {
		fLowerScroller->RemoveSelf();
		delete fLowerScroller;
		fLowerScroller = NULL;
	}

	if (fUpperScroller) {
		fUpperScroller->RemoveSelf();
		delete fUpperScroller;
		fUpperScroller = NULL;
	}

	if (fMenu) {
		// We don't remember the position where the last scrolling
		// ended, so scroll back to the beginning.
		fMenu->ScrollTo(0, 0);
		// Since the scrollers were removed move back up.
		//fMenu->ResizeBy(0, 2 * kScrollerHeight);
		//fMenu->MoveBy(0, -kScrollerHeight);
		fValue = 0;
	}
}


bool
BScrollMenu::HasScrollers() const
{
	return fMenu != NULL && fUpperScroller != NULL && fLowerScroller != NULL;
}


void
BScrollMenu::SetSmallStep(float step)
{
	fScrollStep = step;
}


void
BScrollMenu::GetSteps(float* _smallStep, float* _largeStep) const
{
	if (_smallStep != NULL)
		*_smallStep = fScrollStep;
	if (_largeStep != NULL) {
		if (fMenu != NULL)
			*_largeStep = fMenu->Frame().Height() - fScrollStep;
		else
			*_largeStep = fScrollStep * 2;
	}
}


bool
BScrollMenu::CheckForScrolling(const BPoint &cursor)
{
	if (!HasScrollers())
		return false;

	return _Scroll(cursor);
}


bool
BScrollMenu::TryScrollBy(const float& step)
{
	if (!HasScrollers())
		return false;

	_ScrollBy(step);
	return true;
}


bool
BScrollMenu::_Scroll(const BPoint& where)
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
BScrollMenu::_ScrollBy(const float& step)
{
	if (step > 0) {
		if (fValue == 0)
			fUpperScroller->SetEnabled(true);

		if (fValue + step >= fLimit) {
			// If we reached the limit, only scroll to the end
			fMenu->ScrollBy(0, fLimit - fValue);
			fValue = fLimit;
			fLowerScroller->SetEnabled(false);
		} else {
			fMenu->ScrollBy(0, step);
			fValue += step;
		}
	} else if (step < 0) {
		if (fValue == fLimit)
			fLowerScroller->SetEnabled(true);

		if (fValue + step <= 0) {
			fMenu->ScrollBy(0, -fValue);
			fValue = 0;
			fUpperScroller->SetEnabled(false);
		} else {
			fMenu->ScrollBy(0, step);
			fValue += step;
		}
	}
}
