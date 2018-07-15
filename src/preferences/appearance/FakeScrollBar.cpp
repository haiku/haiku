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
	// add some height to draw the ring around the scroll bar
	float height = B_H_SCROLL_BAR_HEIGHT + 8;
	SetExplicitMinSize(BSize(200, height));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, height));
}


FakeScrollBar::~FakeScrollBar(void)
{
}


void
FakeScrollBar::Draw(BRect updateRect)
{
	BRect rect(Bounds());

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	uint32 flags = BControlLook::B_SCROLLABLE;
	if (IsFocus())
		flags |= BControlLook::B_FOCUSED;

	if (Value() == B_CONTROL_ON)
		SetHighColor(ui_color(B_CONTROL_MARK_COLOR));
	else
		SetHighColor(base);

	// Draw the selected border (2px)
	StrokeRect(rect);
	rect.InsetBy(1, 1);
	StrokeRect(rect);
	rect.InsetBy(1, 1);

	// draw a 1px gap
	SetHighColor(base);
	StrokeRect(rect);
	rect.InsetBy(1, 1);

	// draw a 1px border around control
	SetHighColor(tint_color(base, B_DARKEN_1_TINT));
	StrokeRect(rect);
	rect.InsetBy(1, 1);
	SetHighColor(base);

	be_control_look->DrawScrollBar(this, rect, updateRect, base,
		flags, B_HORIZONTAL, fDoubleArrows);
	float less = floorf(rect.Width() / 3); // thumb takes up 1/3 full width
	BRect thumbRect(rect.left + less, rect.top, rect.right - less, rect.bottom);
	be_control_look->DrawScrollBarThumb(this, rect, thumbRect, updateRect, base,
		flags, B_HORIZONTAL, fKnobStyle);
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
FakeScrollBar::_DrawArrowButton(int32 direction, BRect rect,
	const BRect& updateRect)
{
	if (!updateRect.Intersects(rect))
		return;

	uint32 flags = 0;

	rgb_color baseColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_LIGHTEN_1_TINT);

	be_control_look->DrawButtonBackground(this, rect, updateRect, baseColor,
		flags, BControlLook::B_ALL_BORDERS, B_HORIZONTAL);

	rect.InsetBy(-1, -1);
	be_control_look->DrawArrowShape(this, rect, updateRect,
		baseColor, direction, flags, B_DARKEN_MAX_TINT);
}
