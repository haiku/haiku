/*
 *  Copyright 2010-2012 Haiku, Inc. All rights reserved.
 *  Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		John Scipione <jscipione@gmail.com>
 */


#include "FakeScrollBar.h"

#include <Box.h>
#include <ControlLook.h>
#include <Message.h>
#include <ScrollBar.h>
#include <Shape.h>
#include <Size.h>
#include <Window.h>


typedef enum {
	ARROW_LEFT = 0,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	ARROW_NONE
} arrow_direction;


FakeScrollBar::FakeScrollBar(bool drawArrows, bool doubleArrows,
	BMessage* message)
	:
	BControl("FakeScrollBar", NULL, message, B_WILL_DRAW | B_NAVIGABLE),
	fDrawArrows(drawArrows),
	fDoubleArrows(doubleArrows)
{
	SetExplicitMinSize(BSize(160, 20));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 20));
}


FakeScrollBar::~FakeScrollBar(void)
{
}


void
FakeScrollBar::Draw(BRect updateRect)
{
	BRect bounds = Bounds();

	rgb_color normal = ui_color(B_PANEL_BACKGROUND_COLOR);

	if (IsFocus()) {
		// draw the focus indicator
		SetHighColor(ui_color(B_NAVIGATION_BASE_COLOR));
		StrokeRect(bounds);
		bounds.InsetBy(1.0, 1.0);

		// Draw the selected border (1px)
		if (Value() == B_CONTROL_ON)
			SetHighColor(ui_color(B_CONTROL_MARK_COLOR));
		else
			SetHighColor(normal);

		StrokeRect(bounds);
		bounds.InsetBy(1.0, 1.0);
	} else {
		// Draw the selected border (2px)
		if (Value() == B_CONTROL_ON)
			SetHighColor(ui_color(B_CONTROL_MARK_COLOR));
		else
			SetHighColor(normal);

		StrokeRect(bounds);
		bounds.InsetBy(1.0, 1.0);
		StrokeRect(bounds);
		bounds.InsetBy(1.0, 1.0);
	}

	// draw a gap (1px)
	SetHighColor(normal);
	StrokeRect(bounds);
	bounds.InsetBy(1.0, 1.0);

	// draw a border around control (1px)
	SetHighColor(tint_color(normal, B_DARKEN_1_TINT));
	StrokeRect(bounds);
	bounds.InsetBy(1.0, 1.0);

	BRect thumbBG = bounds;
	BRect bgRect = bounds;

	if (fDrawArrows) {
		// draw arrows
		SetDrawingMode(B_OP_OVER);

		BRect buttonFrame(bounds.left, bounds.top,
			bounds.left + bounds.Height(), bounds.bottom);

		_DrawArrowButton(ARROW_LEFT, fDoubleArrows, buttonFrame, updateRect);

		if (fDoubleArrows) {
			buttonFrame.OffsetBy(bounds.Height() + 1, 0.0);
			_DrawArrowButton(ARROW_RIGHT, fDoubleArrows, buttonFrame,
				updateRect);

			buttonFrame.OffsetTo(bounds.right - ((bounds.Height() * 2) + 1),
				bounds.top);
			_DrawArrowButton(ARROW_LEFT, fDoubleArrows, buttonFrame,
				updateRect);

			thumbBG.left += bounds.Height() * 2 + 2;
			thumbBG.right -= bounds.Height() * 2 + 2;
		} else {
			thumbBG.left += bounds.Height() + 1;
			thumbBG.right -= bounds.Height() + 1;
		}

		buttonFrame.OffsetTo(bounds.right - bounds.Height(), bounds.top);
		_DrawArrowButton(ARROW_RIGHT, fDoubleArrows, buttonFrame, updateRect);

		SetDrawingMode(B_OP_COPY);

		bgRect = bounds.InsetByCopy(48, 0);
	} else
		bgRect = bounds.InsetByCopy(16, 0);

	// fill background besides the thumb
	BRect leftOfThumb(thumbBG.left, thumbBG.top, bgRect.left - 1,
		thumbBG.bottom);
	BRect rightOfThumb(bgRect.right + 1, thumbBG.top, thumbBG.right,
		thumbBG.bottom);

	be_control_look->DrawScrollBarBackground(this, leftOfThumb,
		rightOfThumb, updateRect, normal, 0, B_HORIZONTAL);

	// Draw scroll thumb

	// fill the clickable surface of the thumb
	be_control_look->DrawButtonBackground(this, bgRect, updateRect,
		normal, 0, BControlLook::B_ALL_BORDERS, B_HORIZONTAL);
}


void
FakeScrollBar::MouseDown(BPoint point)
{
	BControl::MouseDown(point);
}


void
FakeScrollBar::MouseMoved(BPoint point, uint32 transit,
	const BMessage* message)
{
	BControl::MouseMoved(point, transit, message);
}


void
FakeScrollBar::MouseUp(BPoint point)
{
	SetValue(B_CONTROL_ON);
	Invoke();

	Invalidate();

	BControl::MouseUp(point);
}


void
FakeScrollBar::SetValue(int32 value)
{
	if (value != Value()) {
		BControl::SetValueNoUpdate(value);
		Invalidate();
	}

	if (!value)
		return;

	BView* parent = Parent();
	BView* child = NULL;

	if (parent != NULL) {
		// If the parent is a BBox, the group parent is the parent of the BBox
		BBox* box = dynamic_cast<BBox*>(parent);

		if (box && box->LabelView() == this)
			parent = box->Parent();

		if (parent != NULL) {
			BBox* box = dynamic_cast<BBox*>(parent);

			// If the parent is a BBox, skip the label if there is one
			if (box && box->LabelView())
				child = parent->ChildAt(1);
			else
				child = parent->ChildAt(0);
		} else
			child = Window()->ChildAt(0);
	} else if (Window())
		child = Window()->ChildAt(0);

	while (child) {
		FakeScrollBar* scrollbar = dynamic_cast<FakeScrollBar*>(child);

		if (scrollbar != NULL && (scrollbar != this))
			scrollbar->SetValue(B_CONTROL_OFF);
		else {
			// If the child is a BBox, check if the label is a scrollbarbutton
			BBox* box = dynamic_cast<BBox*>(child);

			if (box && box->LabelView()) {
				scrollbar = dynamic_cast<FakeScrollBar*>(box->LabelView());

				if (scrollbar != NULL && (scrollbar != this))
					scrollbar->SetValue(B_CONTROL_OFF);
			}
		}

		child = child->NextSibling();
	}

	//ASSERT(Value() == B_CONTROL_ON);
}


//	#pragma mark -


void
FakeScrollBar::SetDoubleArrows(bool doubleArrows)
{
	fDoubleArrows = doubleArrows;
	Invalidate();
}


void
FakeScrollBar::SetKnobStyle(uint32 knobStyle)
{
	fKnobStyle = knobStyle;
	Invalidate();
}


void
FakeScrollBar::SetFromScrollBarInfo(const scroll_bar_info &info)
{
	fDoubleArrows = info.double_arrows;
	fKnobStyle = info.knob;
	Invalidate();
}


//	#pragma mark -


void
FakeScrollBar::_DrawArrowButton(int32 direction, bool doubleArrows, BRect r,
	const BRect& updateRect)
{
	if (!updateRect.Intersects(r))
		return;

	rgb_color c = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color light = tint_color(c, B_LIGHTEN_MAX_TINT);
	rgb_color dark = tint_color(c, B_DARKEN_1_TINT);
	rgb_color darker = tint_color(c, B_DARKEN_2_TINT);
	rgb_color normal = c;
	rgb_color arrow = tint_color(c,
		(B_DARKEN_MAX_TINT + B_DARKEN_4_TINT) / 2.0);

	BPoint tri1, tri2, tri3;
	float hInset = r.Width() / 3;
	float vInset = r.Height() / 3;
	r.InsetBy(hInset, vInset);

	switch (direction) {
		case ARROW_LEFT:
			tri1.Set(r.right, r.top);
			tri2.Set(r.right - r.Width() / 1.33, (r.top + r.bottom + 1) / 2);
			tri3.Set(r.right, r.bottom + 1);
			break;

		case ARROW_RIGHT:
			tri1.Set(r.left, r.bottom + 1);
			tri2.Set(r.left + r.Width() / 1.33, (r.top + r.bottom + 1) / 2);
			tri3.Set(r.left, r.top);
			break;

		case ARROW_UP:
			tri1.Set(r.left, r.bottom);
			tri2.Set((r.left + r.right + 1) / 2, r.bottom - r.Height() / 1.33);
			tri3.Set(r.right + 1, r.bottom);
			break;

		default:
			tri1.Set(r.left, r.top);
			tri2.Set((r.left + r.right + 1) / 2, r.top + r.Height() / 1.33);
			tri3.Set(r.right + 1, r.top);
			break;
	}

	r.InsetBy(-(hInset - 1), -(vInset - 1));
	BRect temp(r.InsetByCopy(-1, -1));
	be_control_look->DrawButtonBackground(this, temp, updateRect,
		normal, 0, BControlLook::B_ALL_BORDERS, B_HORIZONTAL);

	BShape arrowShape;
	arrowShape.MoveTo(tri1);
	arrowShape.LineTo(tri2);
	arrowShape.LineTo(tri3);

	SetHighColor(arrow);
	SetPenSize(ceilf(hInset / 2.0));
	StrokeShape(&arrowShape);
	SetPenSize(1.0);
}
