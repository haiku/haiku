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
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	uint32 flags = BControlLook::B_PARTIALLY_ACTIVATED;

	if (Value() == B_CONTROL_ON)
		SetHighColor(ui_color(B_CONTROL_MARK_COLOR));
	else
		SetHighColor(base);

	BRect rect(Bounds());

	// draw the selected border (2px)
	StrokeRect(rect);
	rect.InsetBy(1, 1);
	StrokeRect(rect);
	rect.InsetBy(1, 1);

	// draw a 1px gap
	SetHighColor(base);
	StrokeRect(rect);
	rect.InsetBy(1, 1);

	// draw a 1px border around the entire scroll bar
	be_control_look->DrawScrollBarBorder(this, rect, updateRect, base, flags,
		B_HORIZONTAL);

	// inset past border
	rect.InsetBy(1, 1);

	// draw arrow buttons
	if (fDrawArrows) {
		BRect buttonFrame(rect.left, rect.top, rect.left + rect.Height(),
			rect.bottom);
		be_control_look->DrawScrollBarButton(this, buttonFrame, updateRect,
			base, flags, BControlLook::B_LEFT_ARROW, B_HORIZONTAL);
		if (fDoubleArrows) {
			buttonFrame.OffsetBy(rect.Height() + 1, 0.0f);
			be_control_look->DrawScrollBarButton(this, buttonFrame,
				updateRect, base, flags, BControlLook::B_RIGHT_ARROW,
				B_HORIZONTAL);
			buttonFrame.OffsetTo(rect.right - ((rect.Height() * 2) + 1),
				rect.top);
			be_control_look->DrawScrollBarButton(this, buttonFrame,
				updateRect, base, flags, BControlLook::B_LEFT_ARROW,
				B_HORIZONTAL);
		}
		buttonFrame.OffsetTo(rect.right - rect.Height(), rect.top);
		be_control_look->DrawScrollBarButton(this, buttonFrame, updateRect,
			base, flags, BControlLook::B_RIGHT_ARROW, B_HORIZONTAL);
	}

	// inset rect to make room for arrows
	if (fDrawArrows) {
		if (fDoubleArrows)
			rect.InsetBy((rect.Height() + 1) * 2, 0.0f);
		else
			rect.InsetBy(rect.Height() + 1, 0.0f);
	}

	// draw background and thumb
	float less = floorf(rect.Width() / 3);
	BRect thumbRect(rect.left + less, rect.top, rect.right - less,
		rect.bottom);
	BRect leftOfThumb(rect.left, thumbRect.top, thumbRect.left - 1,
		thumbRect.bottom);
	BRect rightOfThumb(thumbRect.right + 1, thumbRect.top, rect.right,
		thumbRect.bottom);

	be_control_look->DrawScrollBarBackground(this, leftOfThumb,
		rightOfThumb, updateRect, base, flags, B_HORIZONTAL);
	be_control_look->DrawScrollBarThumb(this, thumbRect, updateRect,
		ui_color(B_SCROLL_BAR_THUMB_COLOR), flags, B_HORIZONTAL, fKnobStyle);
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
