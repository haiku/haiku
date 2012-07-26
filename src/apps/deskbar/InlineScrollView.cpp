/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		John Scipione (jscipione@gmail.com)
 */


#include "InlineScrollView.h"

#include <ControlLook.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <Point.h>
#include <Screen.h>
#include <Window.h>


const int kDefaultScrollStep = 19;
const int kScrollerDimension = 12;


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


class LeftScrollArrow : public ScrollArrow {
public:
							LeftScrollArrow(BRect frame);
	virtual					~LeftScrollArrow();

	virtual	void			Draw(BRect updateRect);
	virtual	void			MouseDown(BPoint where);
};


class RightScrollArrow : public ScrollArrow {
public:
							RightScrollArrow(BRect frame);
	virtual					~RightScrollArrow();

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

	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else {
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
	}

	FillRect(Bounds(), B_SOLID_LOW);

	float middle = Bounds().right / 2;
	FillTriangle(BPoint(middle, (kScrollerDimension / 2) - 3),
		BPoint(middle + 5, (kScrollerDimension / 2) + 2),
		BPoint(middle - 5, (kScrollerDimension / 2) + 2));
}


void
UpScrollArrow::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	TInlineScrollView* parent = dynamic_cast<TInlineScrollView*>(Parent());

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

	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else {
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
	}

	BRect frame = Bounds();
	FillRect(frame, B_SOLID_LOW);

	float middle = Bounds().right / 2;
	FillTriangle(BPoint(middle, frame.bottom - (kScrollerDimension / 2) + 3),
		BPoint(middle + 5, frame.bottom - (kScrollerDimension / 2) - 2),
		BPoint(middle - 5, frame.bottom - (kScrollerDimension / 2) - 2));
}


void
DownScrollArrow::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	TInlineScrollView* grandparent
		= dynamic_cast<TInlineScrollView*>(Parent()->Parent());

	if (grandparent != NULL) {
		grandparent->ScrollBy(kDefaultScrollStep);
		snooze(5000);
	}
}


//	#pragma mark -


LeftScrollArrow::LeftScrollArrow(BRect frame)
	:
	ScrollArrow(frame)
{
}


LeftScrollArrow::~LeftScrollArrow()
{
}


void
LeftScrollArrow::Draw(BRect updateRect)
{
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));

	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else {
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
	}

	FillRect(Bounds(), B_SOLID_LOW);

	float middle = Bounds().bottom / 2;
	FillTriangle(BPoint((kScrollerDimension / 2) - 3, middle),
		BPoint((kScrollerDimension / 2) + 2, middle + 5),
		BPoint((kScrollerDimension / 2) + 2, middle - 5));
}


void
LeftScrollArrow::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	TInlineScrollView* parent = dynamic_cast<TInlineScrollView*>(Parent());

	if (parent != NULL) {
		parent->ScrollBy(-kDefaultScrollStep);
		snooze(5000);
	}
}


//	#pragma mark -


RightScrollArrow::RightScrollArrow(BRect frame)
	:
	ScrollArrow(frame)
{
}


RightScrollArrow::~RightScrollArrow()
{
}


void
RightScrollArrow::Draw(BRect updateRect)
{
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));

	if (IsEnabled())
		SetHighColor(0, 0, 0);
	else {
		SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_2_TINT));
	}

	BRect frame = Bounds();
	FillRect(frame, B_SOLID_LOW);

	float middle = Bounds().bottom / 2;
	FillTriangle(BPoint(frame.bottom - (kScrollerDimension / 2) + 3, middle),
		BPoint(frame.bottom - (kScrollerDimension / 2) - 2, middle + 5),
		BPoint(frame.bottom - (kScrollerDimension / 2) - 2, middle - 5));
}


void
RightScrollArrow::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	TInlineScrollView* grandparent
		= dynamic_cast<TInlineScrollView*>(Parent()->Parent());

	if (grandparent != NULL) {
		grandparent->ScrollBy(kDefaultScrollStep);
		snooze(5000);
	}
}


//	#pragma mark -


TInlineScrollView::TInlineScrollView(BRect frame, BView* target,
	enum orientation orientation)
	:
	BView(frame, "inline scroll view", B_FOLLOW_NONE, 0),
	fTarget(target),
	fUpperScrollArrow(NULL),
	fLowerScrollArrow(NULL),
	fScrollStep(kDefaultScrollStep),
	fValue(0),
	fLimit(0)
{
}


TInlineScrollView::~TInlineScrollView()
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
TInlineScrollView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (fTarget == NULL)
		return;

	AddChild(fTarget);
	fTarget->MoveTo(0, 0);
}


void
TInlineScrollView::DetachedFromWindow()
{
	BView::DetachedFromWindow();

	if (fTarget != NULL)
		fTarget->RemoveSelf();

	if (fUpperScrollArrow != NULL)
		fUpperScrollArrow->RemoveSelf();

	if (fLowerScrollArrow != NULL)
		fLowerScrollArrow->RemoveSelf();
}


//	#pragma mark -


void
TInlineScrollView::AttachScrollers()
{
	if (fTarget == NULL)
		return;

	BRect frame = Bounds();
	BRect screenFrame = (BScreen(Window())).Frame();

	if (HasScrollers()) {
		fLimit = fTarget->Frame().bottom + 2 * kScrollerDimension
			- screenFrame.bottom;
		return;
	}

	fTarget->MakeFocus(true);

	if (fUpperScrollArrow == NULL) {
		fUpperScrollArrow = new UpScrollArrow(
			BRect(0, 0, frame.right, kScrollerDimension - 1));
		AddChild(fUpperScrollArrow);
	}

	if (fLowerScrollArrow == NULL) {
		fLowerScrollArrow = new DownScrollArrow(
			BRect(0, frame.bottom - 2 * kScrollerDimension + 1, frame.right,
				frame.bottom - kScrollerDimension));
		fTarget->AddChild(fLowerScrollArrow);
	}

	fTarget->MoveBy(0, kScrollerDimension);

	fUpperScrollArrow->SetEnabled(false);
	fLowerScrollArrow->SetEnabled(true);

	fLimit = fTarget->Frame().bottom + 2 * kScrollerDimension
		- screenFrame.bottom;
	fValue = 0;
}


void
TInlineScrollView::DetachScrollers()
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

	if (fTarget) {
		// We don't remember the position where the last scrolling
		// ended, so scroll back to the beginning.
		fTarget->MoveBy(0, -kScrollerDimension);
		fTarget->ScrollTo(0, 0);
		fValue = 0;
	}
}


bool
TInlineScrollView::HasScrollers() const
{
	return fTarget != NULL && fUpperScrollArrow != NULL && fLowerScrollArrow != NULL;
}


void
TInlineScrollView::SetSmallStep(float step)
{
	fScrollStep = step;
}


void
TInlineScrollView::GetSteps(float* _smallStep, float* _largeStep) const
{
	if (_smallStep != NULL)
		*_smallStep = fScrollStep;
	if (_largeStep != NULL) {
		if (fTarget != NULL)
			*_largeStep = fTarget->Frame().Height() - fScrollStep;
		else
			*_largeStep = fScrollStep * 2;
	}
}


void
TInlineScrollView::ScrollBy(const float& step)
{
	if (!HasScrollers())
		return;

	if (step > 0) {
		if (fValue == 0)
			fUpperScrollArrow->SetEnabled(true);

		if (fValue + step >= fLimit) {
			// If we reached the limit, only scroll to the end
			fTarget->ScrollBy(0, fLimit - fValue);
			fLowerScrollArrow->MoveBy(0, fLimit - fValue);
			fLowerScrollArrow->SetEnabled(false);
			fValue = fLimit;
		} else {
			fTarget->ScrollBy(0, step);
			fLowerScrollArrow->MoveBy(0, step);
			fValue += step;
		}
	} else if (step < 0) {
		if (fValue == fLimit)
			fLowerScrollArrow->SetEnabled(true);

		if (fValue + step <= 0) {
			fTarget->ScrollBy(0, -fValue);
			fLowerScrollArrow->MoveBy(0, -fValue);
			fUpperScrollArrow->SetEnabled(false);
			fValue = 0;
		} else {
			fTarget->ScrollBy(0, step);
			fLowerScrollArrow->MoveBy(0, step);
			fValue += step;
		}
	}
}
