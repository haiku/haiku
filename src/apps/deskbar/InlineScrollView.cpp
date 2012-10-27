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
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_DARKEN_1_TINT));

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
	if (parent == NULL)
		return;

	float smallStep;
	float largeStep;
	parent->GetSteps(&smallStep, &largeStep);

	BMessage* message = Window()->CurrentMessage();
	int32 modifiers = 0;
	message->FindInt32("modifiers", &modifiers);
	// pressing the option/command/control key scrolls faster
	if ((modifiers & (B_OPTION_KEY | B_COMMAND_KEY | B_CONTROL_KEY)) != 0)
		parent->ScrollBy(-largeStep);
	else
		parent->ScrollBy(-smallStep);

	snooze(5000);
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
	SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
		B_DARKEN_1_TINT));

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
	if (grandparent == NULL)
		return;

	float smallStep;
	float largeStep;
	grandparent->GetSteps(&smallStep, &largeStep);

	BMessage* message = Window()->CurrentMessage();
	int32 modifiers = 0;
	message->FindInt32("modifiers", &modifiers);
	// pressing the option/command/control key scrolls faster
	if ((modifiers & (B_OPTION_KEY | B_COMMAND_KEY | B_CONTROL_KEY)) != 0)
		grandparent->ScrollBy(largeStep);
	else
		grandparent->ScrollBy(smallStep);

	snooze(5000);
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
	if (parent == NULL)
		return;

	float smallStep;
	float largeStep;
	parent->GetSteps(&smallStep, &largeStep);

	BMessage* message = Window()->CurrentMessage();
	int32 modifiers = 0;
	message->FindInt32("modifiers", &modifiers);
	// pressing the option/command/control key scrolls faster
	if ((modifiers & (B_OPTION_KEY | B_COMMAND_KEY | B_CONTROL_KEY)) != 0)
		parent->ScrollBy(-largeStep);
	else
		parent->ScrollBy(-smallStep);

	snooze(5000);
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
	FillTriangle(BPoint(kScrollerDimension / 2 + 3, middle),
		BPoint(kScrollerDimension / 2 - 2, middle + 5),
		BPoint(kScrollerDimension / 2 - 2, middle - 5));
}


void
RightScrollArrow::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	TInlineScrollView* grandparent
		= dynamic_cast<TInlineScrollView*>(Parent()->Parent());
	if (grandparent == NULL)
		return;

	float smallStep;
	float largeStep;
	grandparent->GetSteps(&smallStep, &largeStep);

	BMessage* message = Window()->CurrentMessage();
	int32 modifiers = 0;
	message->FindInt32("modifiers", &modifiers);
	// pressing the option/command/control key scrolls faster
	if ((modifiers & (B_OPTION_KEY | B_COMMAND_KEY | B_CONTROL_KEY)) != 0)
		grandparent->ScrollBy(largeStep);
	else
		grandparent->ScrollBy(smallStep);

	snooze(5000);
}


//	#pragma mark -


TInlineScrollView::TInlineScrollView(BRect frame, BView* target,
	enum orientation orientation)
	:
	BView(frame, "inline scroll view", B_FOLLOW_NONE, B_WILL_DRAW),
	fTarget(target),
	fBeginScrollArrow(NULL),
	fEndScrollArrow(NULL),
	fScrollStep(kDefaultScrollStep),
	fScrollValue(0),
	fScrollLimit(0),
	fOrientation(orientation)
{
}


TInlineScrollView::~TInlineScrollView()
{
	if (fBeginScrollArrow != NULL) {
		fBeginScrollArrow->RemoveSelf();
		delete fBeginScrollArrow;
		fBeginScrollArrow = NULL;
	}

	if (fEndScrollArrow != NULL) {
		fEndScrollArrow->RemoveSelf();
		delete fEndScrollArrow;
		fEndScrollArrow = NULL;
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

	if (fBeginScrollArrow != NULL)
		fBeginScrollArrow->RemoveSelf();

	if (fEndScrollArrow != NULL)
		fEndScrollArrow->RemoveSelf();
}


void
TInlineScrollView::Draw(BRect updateRect)
{
	BRect frame = Bounds();
	be_control_look->DrawButtonBackground(this, frame, updateRect,
		ui_color(B_MENU_BACKGROUND_COLOR));
}


//	#pragma mark -


void
TInlineScrollView::AttachScrollers()
{
	if (fTarget == NULL)
		return;

	BRect frame = Bounds();

	if (HasScrollers()) {
		if (fOrientation == B_VERTICAL) {
			fScrollLimit = fTarget->Bounds().Height()
				- (frame.Height() - 2 * kScrollerDimension);
		} else {
			fScrollLimit = fTarget->Bounds().Width()
				- (frame.Width() - 2 * kScrollerDimension);
		}
		return;
	}

	fTarget->MakeFocus(true);

	if (fOrientation == B_VERTICAL) {
		if (fBeginScrollArrow == NULL) {
			fBeginScrollArrow = new UpScrollArrow(
				BRect(frame.left, frame.top, frame.right,
					kScrollerDimension - 1));
			AddChild(fBeginScrollArrow);
		}

		if (fEndScrollArrow == NULL) {
			fEndScrollArrow = new DownScrollArrow(
				BRect(0, frame.bottom - 2 * kScrollerDimension + 1, frame.right,
					frame.bottom - kScrollerDimension));
			fTarget->AddChild(fEndScrollArrow);
		}

		fTarget->MoveBy(0, kScrollerDimension);

		fScrollLimit = fTarget->Bounds().Height()
			- (frame.Height() - 2 * kScrollerDimension);
	} else {
		if (fBeginScrollArrow == NULL) {
			fBeginScrollArrow = new LeftScrollArrow(
				BRect(frame.left, frame.top,
					frame.left + kScrollerDimension - 1, frame.bottom));
			AddChild(fBeginScrollArrow);
		}

		if (fEndScrollArrow == NULL) {
			fEndScrollArrow = new RightScrollArrow(
				BRect(frame.right - 2 * kScrollerDimension + 1, frame.top,
					frame.right, frame.bottom));
			fTarget->AddChild(fEndScrollArrow);
		}

		fTarget->MoveBy(kScrollerDimension, 0);

		fScrollLimit = fTarget->Bounds().Width()
			- (frame.Width() - 2 * kScrollerDimension);
	}

	fBeginScrollArrow->SetEnabled(false);
	fEndScrollArrow->SetEnabled(true);

	fScrollValue = 0;
}


void
TInlineScrollView::DetachScrollers()
{
	if (!HasScrollers())
		return;

	if (fEndScrollArrow) {
		fEndScrollArrow->RemoveSelf();
		delete fEndScrollArrow;
		fEndScrollArrow = NULL;
	}

	if (fBeginScrollArrow) {
		fBeginScrollArrow->RemoveSelf();
		delete fBeginScrollArrow;
		fBeginScrollArrow = NULL;
	}

	if (fTarget) {
		// We don't remember the position where the last scrolling
		// ended, so scroll back to the beginning.
		if (fOrientation == B_VERTICAL)
			fTarget->MoveBy(0, -kScrollerDimension);
		else
			fTarget->MoveBy(-kScrollerDimension, 0);

		fTarget->ScrollTo(0, 0);
		fScrollValue = 0;
	}
}


bool
TInlineScrollView::HasScrollers() const
{
	return fTarget != NULL && fBeginScrollArrow != NULL
		&& fEndScrollArrow != NULL;
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
		*_largeStep = fScrollStep * 3;
	}
}


void
TInlineScrollView::ScrollBy(const float& step)
{
	if (!HasScrollers())
		return;

	if (step > 0) {
		if (fScrollValue == 0)
			fBeginScrollArrow->SetEnabled(true);

		if (fScrollValue + step >= fScrollLimit) {
			// If we reached the limit, only scroll to the end
			if (fOrientation == B_VERTICAL) {
				fTarget->ScrollBy(0, fScrollLimit - fScrollValue);
				fEndScrollArrow->MoveBy(0, fScrollLimit - fScrollValue);
			} else {
				fTarget->ScrollBy(fScrollLimit - fScrollValue, 0);
				fEndScrollArrow->MoveBy(fScrollLimit - fScrollValue, 0);
			}
			fEndScrollArrow->SetEnabled(false);
			fScrollValue = fScrollLimit;
		} else {
			if (fOrientation == B_VERTICAL) {
				fTarget->ScrollBy(0, step);
				fEndScrollArrow->MoveBy(0, step);
			} else {
				fTarget->ScrollBy(step, 0);
				fEndScrollArrow->MoveBy(step, 0);
			}
			fScrollValue += step;
		}
	} else if (step < 0) {
		if (fScrollValue == fScrollLimit)
			fEndScrollArrow->SetEnabled(true);

		if (fScrollValue + step <= 0) {
			if (fOrientation == B_VERTICAL) {
				fTarget->ScrollBy(0, -fScrollValue);
				fEndScrollArrow->MoveBy(0, -fScrollValue);
			} else {
				fTarget->ScrollBy(-fScrollValue, 0);
				fEndScrollArrow->MoveBy(-fScrollValue, 0);
			}
			fBeginScrollArrow->SetEnabled(false);
			fScrollValue = 0;
		} else {
			if (fOrientation == B_VERTICAL) {
				fTarget->ScrollBy(0, step);
				fEndScrollArrow->MoveBy(0, step);
			} else {
				fTarget->ScrollBy(step, 0);
				fEndScrollArrow->MoveBy(step, 0);
			}
			fScrollValue += step;
		}
	}

	//fTarget->Invalidate();
}
