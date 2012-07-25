/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		John Scipione (jscipione@gmail.com)
 */


#include "ScrollArrowView.h"

#include <ControlLook.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <Point.h>
#include <Screen.h>
#include <Window.h>


const int kDefaultScrollStep = 19;
const int kScrollerHeight = 12;


class ScrollArrow : public BView {
public:
							ScrollArrow(BRect frame);
	virtual					~ScrollArrow();

			bool			IsEnabled() const { return fEnabled; };
			void			SetEnabled(bool enabled);

private:
			bool			fEnabled;
};


class UpScrollArrow : public ScrollArrow {
public:
							UpScrollArrow(BRect frame);
	virtual					~UpScrollArrow();

	virtual	void			Draw(BRect updateRect);
	virtual	void			MouseDown(BPoint where);
};


class DownScrollArrow : public ScrollArrow {
public:
							DownScrollArrow(BRect frame);
	virtual					~DownScrollArrow();

	virtual	void			Draw(BRect updateRect);
	virtual	void			MouseDown(BPoint where);
};


//	#pragma mark -


ScrollArrow::ScrollArrow(BRect frame)
	:
	BView(frame, "menu scroll arrow", B_FOLLOW_NONE, B_WILL_DRAW),
	fEnabled(false)
{
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
}


ScrollArrow::~ScrollArrow()
{
}


void
ScrollArrow::SetEnabled(bool enabled)
{
	fEnabled = enabled;
	Invalidate();
}


//	#pragma mark -


UpScrollArrow::UpScrollArrow(BRect frame)
	:
	ScrollArrow(frame)
{
}


UpScrollArrow::~UpScrollArrow()
{
}


void
UpScrollArrow::Draw(BRect updateRect)
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


void
UpScrollArrow::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	TScrollArrowView* parent = dynamic_cast<TScrollArrowView*>(Parent());

	if (parent != NULL) {
		parent->ScrollBy(-kDefaultScrollStep);
		snooze(5000);
	}
}


//	#pragma mark -


DownScrollArrow::DownScrollArrow(BRect frame)
	:
	ScrollArrow(frame)
{
}


DownScrollArrow::~DownScrollArrow()
{
}


void
DownScrollArrow::Draw(BRect updateRect)
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


void
DownScrollArrow::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	TScrollArrowView* grandparent
		= dynamic_cast<TScrollArrowView*>(Parent()->Parent());

	if (grandparent != NULL) {
		grandparent->ScrollBy(kDefaultScrollStep);
		snooze(5000);
	}
}


//	#pragma mark -


TScrollArrowView::TScrollArrowView(BRect frame, BMenu* menu)
	:
	BView(frame, "menu scroll view", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
	fMenu(menu),
	fUpperScrollArrow(NULL),
	fLowerScrollArrow(NULL),
	fScrollStep(kDefaultScrollStep),
	fValue(0),
	fLimit(0)
{
}


TScrollArrowView::~TScrollArrowView()
{
	if (fUpperScrollArrow != NULL) {
		fUpperScrollArrow->RemoveSelf();
		delete fUpperScrollArrow;
		fUpperScrollArrow = NULL;
	}

	if (fLowerScrollArrow != NULL) {
		fLowerScrollArrow->RemoveSelf();
		delete fLowerScrollArrow;
		fLowerScrollArrow = NULL;
	}
}


void
TScrollArrowView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (fMenu == NULL)
		return;

	AddChild(fMenu);
	fMenu->MoveTo(0, 0);
}


void
TScrollArrowView::DetachedFromWindow()
{
	BView::DetachedFromWindow();

	if (fMenu != NULL)
		fMenu->RemoveSelf();

	if (fUpperScrollArrow != NULL)
		fUpperScrollArrow->RemoveSelf();

	if (fLowerScrollArrow != NULL)
		fLowerScrollArrow->RemoveSelf();
}


//	#pragma mark -


void
TScrollArrowView::AttachScrollers()
{
	if (fMenu == NULL)
		return;

	BRect frame = Bounds();
	BRect screenFrame = (BScreen(Window())).Frame();

	if (HasScrollers()) {
		fLimit = Window()->Frame().bottom + 2 * kScrollerHeight
			- screenFrame.bottom;
		return;
	}

	fMenu->MakeFocus(true);

	if (fUpperScrollArrow == NULL) {
		fUpperScrollArrow = new UpScrollArrow(
			BRect(0, 0, frame.right, kScrollerHeight - 1));
		AddChild(fUpperScrollArrow);
	}

	if (fLowerScrollArrow == NULL) {
		fLowerScrollArrow = new DownScrollArrow(
			BRect(0, frame.bottom - 2 * kScrollerHeight + 1, frame.right,
				frame.bottom - kScrollerHeight));
		fMenu->AddChild(fLowerScrollArrow);
	}

	fMenu->MoveBy(0, kScrollerHeight);

	fUpperScrollArrow->SetEnabled(false);
	fLowerScrollArrow->SetEnabled(true);

	fLimit = Window()->Frame().bottom + 2 * kScrollerHeight
		- screenFrame.bottom;
	fValue = 0;
}


void
TScrollArrowView::DetachScrollers()
{
	if (!HasScrollers())
		return;

	if (fLowerScrollArrow) {
		fLowerScrollArrow->RemoveSelf();
		delete fLowerScrollArrow;
		fLowerScrollArrow = NULL;
	}

	if (fUpperScrollArrow) {
		fUpperScrollArrow->RemoveSelf();
		delete fUpperScrollArrow;
		fUpperScrollArrow = NULL;
	}

	if (fMenu) {
		// We don't remember the position where the last scrolling
		// ended, so scroll back to the beginning.
		fMenu->MoveBy(0, -kScrollerHeight);
		fMenu->ScrollTo(0, 0);
		fValue = 0;
	}
}


bool
TScrollArrowView::HasScrollers() const
{
	return fMenu != NULL && fUpperScrollArrow != NULL && fLowerScrollArrow != NULL;
}


void
TScrollArrowView::SetSmallStep(float step)
{
	fScrollStep = step;
}


void
TScrollArrowView::GetSteps(float* _smallStep, float* _largeStep) const
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


void
TScrollArrowView::ScrollBy(const float& step)
{
	if (!HasScrollers())
		return;

	if (step > 0) {
		if (fValue == 0)
			fUpperScrollArrow->SetEnabled(true);

		if (fValue + step >= fLimit) {
			// If we reached the limit, only scroll to the end
			fMenu->ScrollBy(0, fLimit - fValue);
			fLowerScrollArrow->MoveBy(0, fLimit - fValue);
			fLowerScrollArrow->SetEnabled(false);
			fValue = fLimit;
		} else {
			fMenu->ScrollBy(0, step);
			fLowerScrollArrow->MoveBy(0, step);
			fValue += step;
		}
	} else if (step < 0) {
		if (fValue == fLimit)
			fLowerScrollArrow->SetEnabled(true);

		if (fValue + step <= 0) {
			fMenu->ScrollBy(0, -fValue);
			fLowerScrollArrow->MoveBy(0, -fValue);
			fUpperScrollArrow->SetEnabled(false);
			fValue = 0;
		} else {
			fMenu->ScrollBy(0, step);
			fLowerScrollArrow->MoveBy(0, step);
			fValue += step;
		}
	}
}
