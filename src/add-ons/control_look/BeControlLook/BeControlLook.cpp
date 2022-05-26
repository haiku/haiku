/*
 * Copyright 2003-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Marc Flerackers, mflerackers@androme.be
 *		François Revol, revol@free.fr
 *		John Scipione, jscipione@gmail.com
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


/*! BControlLook resembling BeOS R5 */


#include "BeControlLook.h"

#include <algorithm>

#include <Alignment.h>
#include <Bitmap.h>
#include <Button.h>
#include <Control.h>
#include <LayoutUtils.h>
#include <Region.h>
#include <Shape.h>
#include <String.h>
#include <TabView.h>
#include <View.h>
#include <Window.h>
#include <WindowPrivate.h>

//#define DEBUG_CONTROL_LOOK
#ifdef DEBUG_CONTROL_LOOK
#  define STRACE(x) printf x
#else
#  define STRACE(x) ;
#endif


namespace BPrivate {

static const float kButtonPopUpIndicatorWidth = 11;


BeControlLook::BeControlLook(image_id id)
	:
	fCachedOutline(false)
{
}


BeControlLook::~BeControlLook()
{
}


BAlignment
BeControlLook::DefaultLabelAlignment() const
{
	return BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER);
}


float
BeControlLook::DefaultLabelSpacing() const
{
	return ceilf(be_plain_font->Size() / 2.0);
}


float
BeControlLook::DefaultItemSpacing() const
{
	return ceilf(be_plain_font->Size() * 0.85);
}


uint32
BeControlLook::Flags(BControl* control) const
{
	uint32 flags = B_IS_CONTROL;

	if (!control->IsEnabled())
		flags |= B_DISABLED;

	if (control->IsFocus() && control->Window() != NULL
		&& control->Window()->IsActive()) {
		flags |= B_FOCUSED;
	}

	switch (control->Value()) {
		case B_CONTROL_ON:
			flags |= B_ACTIVATED;
			break;
		case B_CONTROL_PARTIALLY_ON:
			flags |= B_PARTIALLY_ACTIVATED;
			break;
	}

	if (control->Parent() != NULL
		&& (control->Parent()->Flags() & B_DRAW_ON_CHILDREN) != 0) {
		// In this constellation, assume we want to render the control
		// against the already existing view contents of the parent view.
		flags |= B_BLEND_FRAME;
	}

	return flags;
}


void
BeControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, 0, 0, 0, 0, base,
		background, 1.0, 1.0, flags, borders);
}


void
BeControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, 0, 0, 0, 0, base,
		background, 1.0, 1.0, flags, borders);
}


void
BeControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, 0, 0, 0, 0, base,
		background, 1.0, 1.0, flags, borders);
}


void
BeControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, 0, 0, 0, 0,
		base, false, flags, borders, orientation);
}


void
BeControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, 0, 0, 0, 0,
		base, false, flags, borders, orientation);
}


void
BeControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float, const rgb_color& base,
	uint32 flags, uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, 0, 0, 0, 0,
		base, false, flags, borders, orientation);
}


void
BeControlLook::DrawCheckBox(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags)
{
	if (!rect.Intersects(updateRect))
		return;

	bool isEnabled = (flags & B_DISABLED) == 0;
	bool isActivated = (flags & B_ACTIVATED) != 0;
	bool isFocused = (flags & B_FOCUSED) != 0;
	bool isClicked = (flags & B_CLICKED) != 0;

	rgb_color lighten1 = tint_color(base, B_LIGHTEN_1_TINT);
	rgb_color lightenMax = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color darken1 = tint_color(base, B_DARKEN_1_TINT);
	rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT);
	rgb_color darken3 = tint_color(base, B_DARKEN_3_TINT);
	rgb_color darken4 = tint_color(base, B_DARKEN_4_TINT);

	view->SetLowColor(base);

	if (isEnabled) {
		// Filling
		view->SetHighColor(lightenMax);
		view->FillRect(rect);

		// Box
		if (isClicked) {
			view->SetHighColor(darken3);
			view->StrokeRect(rect);

			rect.InsetBy(1, 1);

			view->BeginLineArray(6);

			view->AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), darken2);
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), darken2);
			view->AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.right, rect.bottom), darken4);
			view->AddLine(BPoint(rect.right, rect.bottom),
				BPoint(rect.right, rect.top), darken4);

			view->EndLineArray();
		} else {
			view->BeginLineArray(6);

			view->AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), darken1);
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), darken1);

			rect.InsetBy(1, 1);
			view->AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), darken4);
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), darken4);
			view->AddLine(BPoint(rect.left + 1, rect.bottom),
				BPoint(rect.right, rect.bottom), base);
			view->AddLine(BPoint(rect.right, rect.bottom),
				BPoint(rect.right, rect.top + 1), base);

			view->EndLineArray();
		}

		// Focus
		if (isFocused) {
			view->SetDrawingMode(B_OP_OVER);
			view->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			view->StrokeRect(rect);
			view->SetDrawingMode(B_OP_COPY);
		}
	} else {
		// Filling
		view->SetHighColor(lighten1);
		view->FillRect(rect);

		// Box
		view->BeginLineArray(6);

		view->AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), base);
		view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), base);

		rect.InsetBy(1, 1);
		view->AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), darken2);
		view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), darken2);
		view->AddLine(BPoint(rect.left + 1, rect.bottom),
				BPoint(rect.right, rect.bottom), darken1);
		view->AddLine(BPoint(rect.right, rect.bottom),
				BPoint(rect.right, rect.top + 1), darken1);

		view->EndLineArray();
	}

	// Checkmark
	if (isActivated) {
		rect.InsetBy(4, 4);

		float penSize = std::max(1, 2);
		if (penSize > 1 && fmodf(penSize, 2) == 0) {
			// Tweak ends to "include" the pixel at the index,
			// we need to do this in order to produce results like R5,
			// where coordinates were inclusive
			rect.right++;
			rect.bottom++;
		}

		view->SetPenSize(penSize);
		view->SetDrawingMode(B_OP_OVER);
		if (isEnabled)
			view->SetHighColor(ui_color(B_CONTROL_MARK_COLOR));
		else {
			view->SetHighColor(tint_color(ui_color(B_CONTROL_MARK_COLOR),
				B_DISABLED_MARK_TINT));
		}
		view->StrokeLine(rect.LeftTop(), rect.RightBottom());
		view->StrokeLine(rect.LeftBottom(), rect.RightTop());
	}
}


void
BeControlLook::DrawRadioButton(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	bool isEnabled = (flags & B_DISABLED) == 0;
	bool isActivated = (flags & B_ACTIVATED) != 0;
	bool isFocused = (flags & B_FOCUSED) != 0;

	// colors
	rgb_color bg = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightenmax;
	rgb_color lighten1;
	rgb_color darken1;
	rgb_color darken2;
	rgb_color darken3;

	rgb_color markColor = ui_color(B_CONTROL_MARK_COLOR);
	rgb_color knob;
	rgb_color knobDark;
	rgb_color knobLight;

	if (isEnabled) {
		lightenmax	= tint_color(bg, B_LIGHTEN_MAX_TINT);
		lighten1	= tint_color(bg, B_LIGHTEN_1_TINT);
		darken1		= tint_color(bg, B_DARKEN_1_TINT);
		darken2		= tint_color(bg, B_DARKEN_2_TINT);
		darken3		= tint_color(bg, B_DARKEN_3_TINT);

		knob		= markColor;
		knobDark	= tint_color(markColor, B_DARKEN_3_TINT);
		knobLight	= tint_color(markColor, 0.15);
	} else {
		lightenmax	= tint_color(bg, B_LIGHTEN_2_TINT);
		lighten1	= bg;
		darken1		= bg;
		darken2		= tint_color(bg, B_DARKEN_1_TINT);
		darken3		= tint_color(bg, B_DARKEN_2_TINT);

		knob		= tint_color(markColor, B_LIGHTEN_2_TINT);
		knobDark	= tint_color(markColor, B_LIGHTEN_1_TINT);
		knobLight	= tint_color(markColor,
			(B_LIGHTEN_2_TINT + B_LIGHTEN_MAX_TINT) / 2.0);
	}

	rect.InsetBy(2, 2);

	view->SetLowColor(bg);

	// dot
	if (isActivated) {
		// full
		view->SetHighColor(knobDark);
		view->FillEllipse(rect);

		view->SetHighColor(knob);
		view->FillEllipse(BRect(rect.left + 2, rect.top + 2, rect.right - 3,
			rect.bottom - 3));

		view->SetHighColor(knobLight);
		view->FillEllipse(BRect(rect.left + 3, rect.top + 3, rect.right - 5,
			rect.bottom - 5));
	} else {
		// empty
		view->SetHighColor(lightenmax);
		view->FillEllipse(rect);
	}

	rect.InsetBy(-1, -1);

	// outer circle
	if (isFocused) {
		// indicating "about to change value"
		view->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		view->SetPenSize(2);
		view->SetDrawingMode(B_OP_OVER);
		view->StrokeEllipse(rect.InsetByCopy(1, 1));
		view->SetDrawingMode(B_OP_COPY);
		view->SetPenSize(1);
	} else {
		view->SetHighColor(darken1);
		view->StrokeArc(rect, 45.0, 180.0);
		view->SetHighColor(lightenmax);
		view->StrokeArc(rect, 45.0, -180.0);
	}

	// inner circle
	view->SetHighColor(darken3);
	view->StrokeArc(rect, 45.0, 180.0);
	view->SetHighColor(bg);
	view->StrokeArc(rect, 45.0, -180.0);

	// for faster font rendering, we restore B_OP_COPY
	view->SetDrawingMode(B_OP_COPY);
}


void
BeControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	// set clipping constraints to updateRect
	BRegion clipping(updateRect);
	view->ConstrainClippingRegion(&clipping);

	bool isEnabled = (flags & B_DISABLED) == 0;
	bool isFocused = (flags & B_FOCUSED) != 0;

	if (isEnabled && isFocused)
		view->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	else
		view->SetHighColor(tint_color(base, B_DARKEN_2_TINT));

	view->StrokeRect(rect);

	view->PopState();
}


void
BeControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	int32 direction, orientation orientation, bool down)
{
	view->PushState();

	bool isEnabled = (flags & B_DISABLED) == 0;

	// border = 152, shine = 144/255, shadow = 208/184/192
	rgb_color shine = down ? tint_color(base, 1.333)
		: tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color shadow;
	if (isEnabled && down)
		shadow = tint_color(base, 1.037);
	else if (isEnabled)
		shadow = tint_color(base, B_DARKEN_1_TINT);
	else
		shadow = tint_color(base, 1.111);

	view->BeginLineArray(4);
	view->AddLine(rect.LeftTop(), rect.LeftBottom() - BPoint(0, 1), shine);
	view->AddLine(rect.LeftTop(), rect.RightTop() - BPoint(1, 0), shine);
	view->AddLine(rect.RightTop(), rect.RightBottom() - BPoint(0, 1), shadow);
	view->AddLine(rect.LeftBottom(), rect.RightBottom(), shadow);
	view->EndLineArray();

	rgb_color bg;
	if (isEnabled) {
		// bg = 176/216
		bg = down ? tint_color(base, 1.185) : base;
	} else {
		// bg = 240
		rgb_color lighten2 = tint_color(base, B_LIGHTEN_2_TINT);
		lighten2.red++; lighten2.green++; lighten2.blue++;
			// lighten2 = 239, 240 = 239 + 1
		bg = lighten2;
	}
	view->SetHighColor(bg);
	rect.InsetBy(1, 1);
	view->FillRect(rect);

	// draw button triangle
	// don't use DrawArrowShape because we use that to draw arrows differently
	// in menus and outline list view

	rect.InsetBy(1, 1);
	rect.OffsetBy(-3, -3);

	BPoint tri1, tri2, tri3;
	BPoint off1, off2, off3;
	BRect r(rect.left, rect.top, rect.left + 14, rect.top + 14);
	rgb_color lightenmax = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color light, dark, arrow, arrow2;

	switch(direction) {
		case B_LEFT_ARROW:
			tri1.Set(r.left + 3, floorf((r.top + r.bottom) / 2));
			tri2.Set(r.right - 4, r.top + 4);
			tri3.Set(r.right - 4, r.bottom - 4);
			break;

		default:
		case B_RIGHT_ARROW:
			tri1.Set(r.left + 4, r.bottom - 4);
			tri2.Set(r.left + 4, r.top + 4);
			tri3.Set(r.right - 3, floorf((r.top + r.bottom) / 2));
			break;

		case B_UP_ARROW:
			tri1.Set(r.left + 4, r.bottom - 4);
			tri2.Set(floorf((r.left + r.right) / 2), r.top + 3);
			tri3.Set(r.right - 4, r.bottom - 4);
			break;

		case B_DOWN_ARROW:
			tri1.Set(r.left + 4, r.top + 4);
			tri2.Set(r.right - 4, r.top + 4);
			tri3.Set(floorf((r.left + r.right) / 2), r.bottom - 3);
			break;
	}

	r.InsetBy(1, 1);

	float tint = B_NO_TINT;
	if (!isEnabled)
		tint = (tint + B_NO_TINT + B_NO_TINT) / 3;

	view->SetHighColor(tint_color(base, tint));
	view->MovePenTo(B_ORIGIN);
	view->SetDrawingMode(B_OP_OVER);

	view->SetHighColor(tint_color(base, tint));

	if (isEnabled) {
		arrow2 = light = tint_color(base, B_DARKEN_2_TINT);
		dark = tint_color(base, B_DARKEN_3_TINT);
		arrow = tint_color(base, B_DARKEN_MAX_TINT);
	} else
		arrow = arrow2 = light = dark = tint_color(base, B_DARKEN_1_TINT);

	// white triangle offset by 1px
	off1.Set(tri1.x + 1, tri1.y + 1);
	off2.Set(tri2.x + 1, tri2.y + 1);
	off3.Set(tri3.x + 1, tri3.y + 1);

	// draw white triangle
	view->BeginLineArray(3);
	view->AddLine(off2, off3, lightenmax);
	view->AddLine(off1, off3, lightenmax);
	view->AddLine(off1, off2, lightenmax);
	view->EndLineArray();

	// draw triangle on top
	view->BeginLineArray(3);
	view->AddLine(tri2, tri3, dark);
	view->AddLine(tri1, tri3, dark);
	view->AddLine(tri1, tri2, arrow2);
	view->EndLineArray();

	view->PopState();
}


void
BeControlLook::DrawScrollBarBackground(BView* view, BRect& rect1, BRect& rect2,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	_DrawScrollBarBackgroundFirst(view, rect1, updateRect, base, flags,
		orientation);
	_DrawScrollBarBackgroundSecond(view, rect2, updateRect, base, flags,
		orientation);
}


void
BeControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	_DrawScrollBarBackgroundFirst(view, rect, updateRect, base, flags,
		orientation);
}


void
BeControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	// set clipping constraints to updateRect
	BRegion clipping(updateRect);
	view->ConstrainClippingRegion(&clipping);

	bool isEnabled = (flags & B_DISABLED) == 0;

	BRect orig(rect);

	// shine = 255
	rgb_color shine = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color bg;
	if (isEnabled) {
		// bg = 216
		bg = base;
	} else {
		// bg = 240
		rgb_color lighten2 = tint_color(base, B_LIGHTEN_2_TINT);
		lighten2.red++; lighten2.green++; lighten2.blue++;
			// lighten2 = 239, 240 = 239 + 1
		bg = lighten2;
	}

	// draw thumb over background
	view->SetDrawingMode(B_OP_OVER);

	view->BeginLineArray(2);
	if (orientation == B_VERTICAL) {
		// shine
		view->AddLine(rect.LeftTop(), rect.LeftBottom(), shine);
		rect.left++;
		view->AddLine(rect.LeftTop(), rect.RightTop(), shine);
		rect.top++;
	} else {
		// shine
		view->AddLine(rect.LeftTop(), rect.LeftBottom(), shine);
		rect.left++;
		view->AddLine(rect.LeftTop(), rect.RightTop(), shine);
		rect.top++;
	}
	view->EndLineArray();

	// fill bg
	view->SetHighColor(bg);
	view->FillRect(rect);

	// undraw right top or left bottom point
	view->BeginLineArray(1);
	if (orientation == B_VERTICAL) {
		rect.right--;
		view->AddLine(rect.RightTop(), rect.RightTop(), base);
	} else {
		rect.bottom--;
		view->AddLine(rect.LeftBottom(), rect.LeftBottom(), base);
	}
	view->EndLineArray();

	// restore rect
	rect = orig;

	// knobs

	if (knobStyle != B_KNOB_NONE) {
		float hcenter = rect.left + rect.Width() / 2;
		float vmiddle = rect.top + rect.Height() / 2;
		rgb_color knobDark = tint_color(base, B_DARKEN_1_TINT);
		rgb_color knobLight = tint_color(base, B_LIGHTEN_MAX_TINT);

		if (knobStyle == B_KNOB_DOTS) {
			// center/middle dot
			_DrawScrollBarKnobDot(view, hcenter, vmiddle, knobDark, knobLight,
				orientation);
			if (orientation == B_HORIZONTAL) {
				float spacer = rect.Height();
				// left dot
				if (rect.left + 7 < hcenter - spacer) {
					_DrawScrollBarKnobDot(view, hcenter - 7, vmiddle, knobDark,
						knobLight, orientation);
				}
				// right dot
				if (rect.right - 7 > hcenter + spacer) {
					_DrawScrollBarKnobDot(view, hcenter + 7, vmiddle, knobDark,
						knobLight, orientation);
				}
			} else {
				float spacer = rect.Width();
				// top dot
				if (rect.top + 7 < vmiddle - spacer) {
					_DrawScrollBarKnobDot(view, hcenter, vmiddle - 7, knobDark,
						knobLight, orientation);
				}
				// bottom dot
				if (rect.bottom - 7 > vmiddle + spacer) {
					_DrawScrollBarKnobDot(view, hcenter, vmiddle + 7, knobDark,
						knobLight, orientation);
				}
			}
		} else if (knobStyle == B_KNOB_LINES) {
			// center/middle line
			_DrawScrollBarKnobLine(view, hcenter, vmiddle, knobDark, knobLight,
				orientation);
			if (orientation == B_HORIZONTAL) {
				float spacer = rect.Height();
				// left line
				if (rect.left + 4 < hcenter - spacer) {
					_DrawScrollBarKnobLine(view, hcenter - 4, vmiddle, knobDark,
						knobLight, orientation);
				}
				// right line
				if (rect.right - 4 > hcenter + spacer) {
					_DrawScrollBarKnobLine(view, hcenter + 4, vmiddle, knobDark,
						knobLight, orientation);
				}
			} else {
				float spacer = rect.Width();
				// top line
				if (rect.top + 4 < vmiddle - spacer) {
					_DrawScrollBarKnobLine(view, hcenter, vmiddle - 4, knobDark,
						knobLight, orientation);
				}
				// bottom line
				if (rect.bottom - 5 > vmiddle + spacer) {
					_DrawScrollBarKnobLine(view, hcenter, vmiddle + 4, knobDark,
						knobLight, orientation);
				}
			}
		}
	}

	view->PopState();
}



void
BeControlLook::DrawScrollViewFrame(BView* view, BRect& rect,
	const BRect& updateRect, BRect verticalScrollBarFrame,
	BRect horizontalScrollBarFrame, const rgb_color& base,
	border_style borderStyle, uint32 flags, uint32 _borders)
{
	rgb_color darken1 = tint_color(base, B_DARKEN_1_TINT);
	rgb_color lightenmax = tint_color(base, B_LIGHTEN_MAX_TINT);

	view->BeginLineArray(4);
	view->AddLine(rect.LeftBottom(), rect.LeftTop(), darken1);
	view->AddLine(rect.LeftTop(), rect.RightTop(), darken1);
	view->AddLine(rect.RightTop(), rect.RightBottom(), lightenmax);
	view->AddLine(rect.RightBottom(), rect.LeftBottom(), lightenmax);
	view->EndLineArray();

	rect.InsetBy(1, 1);

	bool isEnabled = (flags & B_DISABLED) == 0;
	bool isFocused = (flags & B_FOCUSED) != 0;

	if (isEnabled && isFocused)
		view->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	else
		view->SetHighColor(tint_color(base, B_DARKEN_2_TINT));

	view->StrokeRect(rect);
}


void
BeControlLook::DrawArrowShape(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 direction, uint32 flags, float tint)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	rgb_color fill = tint_color(base, 1.074); // 200
	rgb_color stroke = tint_color(base, 1.629); // 80

	switch(direction) {
		case B_LEFT_ARROW:
			view->SetHighColor(fill);
			view->FillTriangle(rect.LeftTop() + BPoint(4, 6),
				rect.LeftTop() + BPoint(8, 2),
				rect.LeftTop() + BPoint(8, 10));
			view->SetHighColor(stroke);
			view->StrokeTriangle(rect.LeftTop() + BPoint(4, 6),
				rect.LeftTop() + BPoint(8, 2),
				rect.LeftTop() + BPoint(8, 10));
			break;

		default:
		case B_RIGHT_ARROW:
			view->SetHighColor(fill);
			view->FillTriangle(rect.LeftTop() + BPoint(4, 2),
				rect.LeftTop() + BPoint(4, 10),
				rect.LeftTop() + BPoint(8, 6));
			view->SetHighColor(stroke);
			view->StrokeTriangle(rect.LeftTop() + BPoint(4, 2),
				rect.LeftTop() + BPoint(4, 10),
				rect.LeftTop() + BPoint(8, 6));
			break;

		case B_UP_ARROW:
			view->SetHighColor(fill);
			view->FillTriangle(rect.LeftTop() + BPoint(6, 4),
				rect.LeftTop() + BPoint(2, 8),
				rect.LeftTop() + BPoint(10, 8));
			view->SetHighColor(stroke);
			view->StrokeTriangle(rect.LeftTop() + BPoint(6, 4),
				rect.LeftTop() + BPoint(2, 8),
				rect.LeftTop() + BPoint(10, 8));
			break;

		case B_DOWN_ARROW:
			view->SetHighColor(fill);
			view->FillTriangle(rect.LeftTop() + BPoint(2, 4),
				rect.LeftTop() + BPoint(10, 4),
				rect.LeftTop() + BPoint(6, 8));
			view->SetHighColor(stroke);
			view->StrokeTriangle(rect.LeftTop() + BPoint(2, 4),
				rect.LeftTop() + BPoint(10, 4),
				rect.LeftTop() + BPoint(6, 8));
			break;
	}

	view->PopState();
}


void
BeControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	// restore the background color in case a menu item was selected
	view->SetHighColor(base);
	view->FillRect(rect);

	rgb_color lighten2 = tint_color(base, B_LIGHTEN_2_TINT);
	rgb_color darken1 = tint_color(base, B_DARKEN_1_TINT);

	view->BeginLineArray(3);
	view->AddLine(rect.LeftTop(), rect.RightTop(), lighten2);
	// left bottom pixel is base color
	view->AddLine(rect.LeftTop(), rect.LeftBottom() - BPoint(0, 1),
		lighten2);
	view->AddLine(rect.LeftBottom() + BPoint(1, 0), rect.RightBottom(),
		darken1);
	view->EndLineArray();

	view->PopState();
}


void
BeControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	// BeControlLook does not support rounded corners and it never will
	DrawMenuFieldFrame(view, rect, updateRect, 0, base, background, flags,
		borders);
}


void
BeControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	// BeControlLook does not support rounded corners and it never will
	DrawMenuFieldFrame(view, rect, updateRect, 0, 0, 0, 0, base,
		background, flags, borders);
}


void
BeControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	bool isEnabled = (flags & B_DISABLED) == 0;
	bool isFocused = (flags & B_FOCUSED) != 0;

	// inset the frame by 2 and draw the outer border
	rect.InsetBy(2, 2);

	rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT);

	// draw left and top side and top right corner
	view->BeginLineArray(3);
	view->AddLine(BPoint(rect.left - 1, rect.top - 1),
		BPoint(rect.left - 1, rect.bottom - 1), darken2);
	view->AddLine(BPoint(rect.left - 1, rect.top - 1),
		BPoint(rect.right - 1, rect.top - 1), darken2);
	view->AddLine(BPoint(rect.right, rect.top - 1),
		BPoint(rect.right, rect.top - 1), darken2);
	view->EndLineArray();

	if (isEnabled && isFocused) {
		// draw the focus ring on top of the frame
		// Note that this is an intentional deviation from BeOS R5
		// which draws the frame around the outside of the frame
		// but that doesn't look as good.
		view->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		view->StrokeRect(rect.InsetByCopy(-1, -1));
	}
}


void
BeControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	_DrawMenuFieldBackgroundOutside(view, rect, updateRect,
		0, 0, 0, 0, base, popupIndicator, flags);
}


void
BeControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	_DrawMenuFieldBackgroundInside(view, rect, updateRect,
		0, 0, 0, 0, base, flags, borders);
}


void
BeControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	_DrawMenuFieldBackgroundOutside(view, rect, updateRect, 0, 0,
		0, 0, base, popupIndicator, flags);
}


void
BeControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	_DrawMenuFieldBackgroundOutside(view, rect, updateRect, 0, 0,
		0, 0, base, popupIndicator, flags);
}


void
BeControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	view->SetHighColor(base);
	view->FillRect(rect);

	view->PopState();
}


void
BeControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	view->SetHighColor(base);
	view->FillRect(rect);

	view->PopState();
}


void
BeControlLook::DrawStatusBar(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, const rgb_color& barColor, float progressPosition)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	view->SetHighColor(base);
	view->FillRect(rect);

	view->PopState();
}


rgb_color
BeControlLook::SliderBarColor(const rgb_color& base)
{
	return tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT);
}


void
BeControlLook::DrawSliderBar(BView* view, BRect rect, const BRect& updateRect,
	const rgb_color& base, rgb_color leftFillColor, rgb_color rightFillColor,
	float sliderScale, uint32 flags, orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	// separate the bar in two sides
	float sliderPosition;
	BRect leftBarSide = rect;
	BRect rightBarSide = rect;

	if (orientation == B_HORIZONTAL) {
		sliderPosition = floorf(rect.left + 2 + (rect.Width() - 2)
			* sliderScale);
		leftBarSide.right = sliderPosition - 1;
		rightBarSide.left = sliderPosition;
	} else {
		// NOTE: position is reverse of coords
		sliderPosition = floorf(rect.top + 2 + (rect.Height() - 2)
			* (1.0 - sliderScale));
		leftBarSide.top = sliderPosition;
		rightBarSide.bottom = sliderPosition - 1;
	}

	// fill the background for the corners, exclude the middle bar for now
	BRegion region(rect);
	region.Exclude(rightBarSide);
	view->ConstrainClippingRegion(&region);

	view->PushState();

	DrawSliderBar(view, rect, updateRect, base, leftFillColor, flags,
		orientation);

	view->PopState();

	region.Set(rect);
	region.Exclude(leftBarSide);
	view->ConstrainClippingRegion(&region);

	view->PushState();

	DrawSliderBar(view, rect, updateRect, base, rightFillColor, flags,
		orientation);

	view->PopState();

	view->PopState();
}


void
BeControlLook::DrawSliderBar(BView* view, BRect rect, const BRect& updateRect,
	const rgb_color& base, rgb_color fillColor, uint32 flags,
	orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->SetHighColor(fillColor);
	view->FillRect(rect);

	rgb_color lightenmax = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color darken1 = tint_color(base, B_DARKEN_1_TINT);
	rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT);
	rgb_color darkenmax = tint_color(base, B_DARKEN_MAX_TINT);

	view->BeginLineArray(9);

	view->AddLine(BPoint(rect.left, rect.top),
		BPoint(rect.left + 1, rect.top), darken1);
	view->AddLine(BPoint(rect.left, rect.bottom),
		BPoint(rect.left + 1, rect.bottom), darken1);
	view->AddLine(BPoint(rect.right - 1, rect.top),
		BPoint(rect.right, rect.top), darken1);

	view->AddLine(BPoint(rect.left + 1, rect.top),
		BPoint(rect.right - 1, rect.top), darken2);
	view->AddLine(BPoint(rect.left, rect.bottom - 1),
		BPoint(rect.left, rect.top + 1), darken2);

	view->AddLine(BPoint(rect.left + 1, rect.bottom),
		BPoint(rect.right, rect.bottom), lightenmax);
	view->AddLine(BPoint(rect.right, rect.top + 1),
		BPoint(rect.right, rect.bottom), lightenmax);

	rect.InsetBy(1, 1);

	view->AddLine(BPoint(rect.left, rect.top),
		BPoint(rect.left, rect.bottom), darkenmax);
	view->AddLine(BPoint(rect.left, rect.top),
		BPoint(rect.right, rect.top), darkenmax);

	view->EndLineArray();
}


void
BeControlLook::DrawSliderThumb(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	rgb_color lighten2 = tint_color(base, B_LIGHTEN_2_TINT);
	rgb_color dark = tint_color(base, 1.333); // 144
	rgb_color darker = tint_color(base, 1.444); // 120
	rgb_color darkenmax = tint_color(base, B_DARKEN_MAX_TINT);

	view->BeginLineArray(14);

	// outline
	view->AddLine(BPoint(rect.left, rect.bottom - 2),
		BPoint(rect.left, rect.top + 1), darker);
	view->AddLine(BPoint(rect.left + 1, rect.top),
		BPoint(rect.right - 2, rect.top), darker);
	view->AddLine(BPoint(rect.right, rect.top + 2),
		BPoint(rect.right, rect.bottom - 1), darker);
	view->AddLine(BPoint(rect.left + 2, rect.bottom),
		BPoint(rect.right - 1, rect.bottom), darker);

	// first bevel
	rect.InsetBy(1, 1);

	view->SetHighColor(lighten2);
	view->FillRect(rect);

	view->AddLine(BPoint(rect.left, rect.bottom),
		BPoint(rect.right - 1, rect.bottom), darkenmax);
	view->AddLine(BPoint(rect.right, rect.bottom - 1),
		BPoint(rect.right, rect.top), darkenmax);

	rect.InsetBy(1, 1);

	// second bevel and center dots
	view->SetHighColor(dark);
	view->AddLine(BPoint(rect.left, rect.bottom),
		BPoint(rect.right, rect.bottom), dark);
	view->AddLine(BPoint(rect.right, rect.top),
		BPoint(rect.right, rect.bottom), dark);

	// center dots
	float hCenter = rect.Width() / 2;
	float vMiddle = rect.Height() / 2;
	if (orientation == B_HORIZONTAL) {
		view->AddLine(BPoint(rect.left + hCenter, rect.top + 1),
			BPoint(rect.left + hCenter, rect.top + 1), dark);
		view->AddLine(BPoint(rect.left + hCenter, rect.top + 3),
			BPoint(rect.left + hCenter, rect.top + 3), dark);
		view->AddLine(BPoint(rect.left + hCenter, rect.top + 5),
			BPoint(rect.left + hCenter, rect.top + 5), dark);
	} else {
		view->AddLine(BPoint(rect.left + 1, rect.top + vMiddle),
			BPoint(rect.left + 1, rect.top + vMiddle), dark);
		view->AddLine(BPoint(rect.left + 3, rect.top + vMiddle),
			BPoint(rect.left + 3, rect.top + vMiddle), dark);
		view->AddLine(BPoint(rect.left + 5, rect.top + vMiddle - 1),
			BPoint(rect.left + 5, rect.top + vMiddle), dark);
	}

	view->AddLine(BPoint(rect.right + 1, rect.bottom + 1),
		BPoint(rect.right + 1, rect.bottom + 1), dark);

	rect.InsetBy(1, 1);

	// third bevel
	view->AddLine(BPoint(rect.left, rect.bottom),
		BPoint(rect.right, rect.bottom), base);
	view->AddLine(BPoint(rect.right, rect.top),
		BPoint(rect.right, rect.bottom), base);

	view->EndLineArray();
}


void
BeControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawSliderTriangle(view, rect, updateRect, base, base, flags, orientation);
}


void
BeControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	rgb_color lighten1 = tint_color(base, B_LIGHTEN_1_TINT);
	rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT);
	rgb_color darkenmax = tint_color(base, B_DARKEN_MAX_TINT);

	if (orientation == B_HORIZONTAL) {
		view->SetHighColor(lighten1);
		view->FillTriangle(BPoint(rect.left, rect.bottom - 1),
			BPoint(rect.left + 6, rect.top),
			BPoint(rect.right, rect.bottom - 1));

		view->SetHighColor(darkenmax);
		view->StrokeLine(BPoint(rect.right, rect.bottom + 1),
			BPoint(rect.left, rect.bottom + 1));
		view->StrokeLine(BPoint(rect.right, rect.bottom),
			BPoint(rect.left + 6, rect.top));

		view->SetHighColor(darken2);
		view->StrokeLine(BPoint(rect.right - 1, rect.bottom),
			BPoint(rect.left, rect.bottom));
		view->StrokeLine(BPoint(rect.left, rect.bottom),
			BPoint(rect.left + 5, rect.top + 1));

		view->SetHighColor(base);
		view->StrokeLine(BPoint(rect.right - 2, rect.bottom - 1),
			BPoint(rect.left + 3, rect.bottom - 1));
		view->StrokeLine(BPoint(rect.right - 3, rect.bottom - 2),
			BPoint(rect.left + 6, rect.top + 1));
	} else {
		view->SetHighColor(lighten1);
		view->FillTriangle(BPoint(rect.left + 1, rect.top),
			BPoint(rect.left + 7, rect.top + 6),
			BPoint(rect.left + 1, rect.bottom));

		view->SetHighColor(darkenmax);
		view->StrokeLine(BPoint(rect.left, rect.top + 1),
			BPoint(rect.left, rect.bottom));
		view->StrokeLine(BPoint(rect.left + 1, rect.bottom),
			BPoint(rect.left + 7, rect.top + 6));

		view->SetHighColor(darken2);
		view->StrokeLine(BPoint(rect.left, rect.top),
			BPoint(rect.left, rect.bottom - 1));
		view->StrokeLine(BPoint(rect.left + 1, rect.top),
			BPoint(rect.left + 6, rect.top + 5));

		view->SetHighColor(base);
		view->StrokeLine(BPoint(rect.left + 1, rect.top + 2),
			BPoint(rect.left + 1, rect.bottom - 1));
		view->StrokeLine(BPoint(rect.left + 2, rect.bottom - 2),
			BPoint(rect.left + 6, rect.top + 6));
	}
}


void
BeControlLook::DrawSliderHashMarks(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, int32 count,
	hash_mark_location location, uint32 flags, orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	rgb_color lightColor;
	rgb_color darkColor;

	if ((flags & B_DISABLED) != 0) {
		lightColor = tint_color(base, 0.9);
		darkColor = tint_color(base, 1.07);
	} else {
		lightColor = tint_color(base, 0.8);
		darkColor = tint_color(base, 1.14);
	}

	int32 hashMarkCount = std::max(count, (int32)2);
		// draw at least two hashmarks at min/max if
		// fHashMarks != B_HASH_MARKS_NONE
	float factor;
	float startPos;
	if (orientation == B_HORIZONTAL) {
		factor = (rect.Width() - 2) / (hashMarkCount - 1);
		startPos = rect.left + 1;
	} else {
		factor = (rect.Height() - 2) / (hashMarkCount - 1);
		startPos = rect.top + 1;
	}

	if (location & B_HASH_MARKS_TOP) {
		view->BeginLineArray(hashMarkCount * 2);

		if (orientation == B_HORIZONTAL) {
			float pos = startPos;
			for (int32 i = 0; i < hashMarkCount; i++) {
				view->AddLine(BPoint(pos, rect.top),
							  BPoint(pos, rect.top + 4), darkColor);
				view->AddLine(BPoint(pos + 1, rect.top),
							  BPoint(pos + 1, rect.top + 4), lightColor);

				pos += factor;
			}
		} else {
			float pos = startPos;
			for (int32 i = 0; i < hashMarkCount; i++) {
				view->AddLine(BPoint(rect.left, pos),
							  BPoint(rect.left + 4, pos), darkColor);
				view->AddLine(BPoint(rect.left, pos + 1),
							  BPoint(rect.left + 4, pos + 1), lightColor);

				pos += factor;
			}
		}

		view->EndLineArray();
	}

	if ((location & B_HASH_MARKS_BOTTOM) != 0) {
		view->BeginLineArray(hashMarkCount * 2);

		if (orientation == B_HORIZONTAL) {
			float pos = startPos;
			for (int32 i = 0; i < hashMarkCount; i++) {
				view->AddLine(BPoint(pos, rect.bottom - 4),
							  BPoint(pos, rect.bottom), darkColor);
				view->AddLine(BPoint(pos + 1, rect.bottom - 4),
							  BPoint(pos + 1, rect.bottom), lightColor);

				pos += factor;
			}
		} else {
			float pos = startPos;
			for (int32 i = 0; i < hashMarkCount; i++) {
				view->AddLine(BPoint(rect.right - 4, pos),
							  BPoint(rect.right, pos), darkColor);
				view->AddLine(BPoint(rect.right - 4, pos + 1),
							  BPoint(rect.right, pos + 1), lightColor);

				pos += factor;
			}
		}

		view->EndLineArray();
	}
}


void
BeControlLook::DrawTabFrame(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, uint32 borders,
	border_style borderStyle, uint32 side)
{
	view->SetHighColor(base);
	view->FillRect(rect);

	rgb_color lightenmax = tint_color(base, B_LIGHTEN_MAX_TINT);

	view->BeginLineArray(1);

	switch(side) {
		case BTabView::kLeftSide:
			view->AddLine(BPoint(rect.right, rect.top),
				BPoint(rect.right, rect.bottom), lightenmax);
			break;

		case BTabView::kRightSide:
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.left, rect.bottom), lightenmax);
			break;

		default:
		case BTabView::kTopSide:
			view->AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.right, rect.bottom), lightenmax);
			break;

		case BTabView::kBottomSide:
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), lightenmax);
			break;
	}

	view->EndLineArray();
}


void
BeControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32, int32, int32, int32)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	// set clipping constraints to updateRect plus 2px extra
	BRegion clipping(updateRect.InsetByCopy(-2, -2));
	view->ConstrainClippingRegion(&clipping);

	// set colors and draw

	rgb_color lightenmax = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color darken4 = tint_color(base, B_DARKEN_4_TINT);
	rgb_color darkenmax = tint_color(base, B_DARKEN_MAX_TINT);

	view->SetHighColor(darkenmax);
	view->SetLowColor(base);

	view->BeginLineArray(12);

	switch (side) {
		case BTabView::kLeftSide:
			// before going left
			view->AddLine(BPoint(rect.right - 1, rect.top - 1),
				BPoint(rect.right - 1, rect.top - 1), lightenmax);
			view->AddLine(BPoint(rect.right - 2, rect.top),
				BPoint(rect.right - 3, rect.top), lightenmax);

			// going left
			view->AddLine(BPoint(rect.right - 4, rect.top + 1),
				BPoint(rect.left + 5, rect.top + 1), lightenmax);

			// before going down
			view->AddLine(BPoint(rect.left + 2, rect.top + 2),
				BPoint(rect.left + 4, rect.top + 2), lightenmax);
			view->AddLine(BPoint(rect.left + 1, rect.top + 3),
				BPoint(rect.left + 1, rect.top + 4 ), lightenmax);

			// going down
			view->AddLine(BPoint(rect.left, rect.top + 5),
				BPoint(rect.left, rect.bottom - 5), lightenmax);

			// after going down
			view->AddLine(BPoint(rect.left + 1, rect.bottom - 4),
				BPoint(rect.left + 1, rect.bottom - 3), lightenmax);
			view->AddLine(BPoint(rect.left + 2, rect.bottom - 2),
				BPoint(rect.left + 3, rect.bottom - 2), darken4);

			// going right
			view->AddLine(BPoint(rect.left + 4, rect.bottom - 1),
				BPoint(rect.right - 4, rect.bottom - 1), darken4);

			// after going right
			view->AddLine(BPoint(rect.right - 3, rect.bottom),
				BPoint(rect.right - 2, rect.bottom), darken4);
			view->AddLine(BPoint(rect.right - 1, rect.bottom + 1),
				BPoint(rect.right - 1, rect.bottom + 1), darken4);
			view->AddLine(BPoint(rect.right, rect.bottom + 2),
				BPoint(rect.right, rect.bottom + 2), darken4);
			break;

		case BTabView::kRightSide:
			// before going right
			view->AddLine(BPoint(rect.left - 1, rect.top - 1),
				BPoint(rect.left - 1, rect.top - 1), lightenmax);
			view->AddLine(BPoint(rect.left - 2, rect.top),
				BPoint(rect.left - 3, rect.top), lightenmax);

			// going right
			view->AddLine(BPoint(rect.left - 4, rect.top + 1),
				BPoint(rect.right + 5, rect.top + 1), lightenmax);

			// before going down
			view->AddLine(BPoint(rect.right + 2, rect.top + 2),
				BPoint(rect.right + 4, rect.top + 2), lightenmax);
			view->AddLine(BPoint(rect.right + 1, rect.top + 3),
				BPoint(rect.right + 1, rect.top + 4 ), lightenmax);

			// going down
			view->AddLine(BPoint(rect.right, rect.top + 5),
				BPoint(rect.right, rect.bottom - 5), lightenmax);

			// after going down
			view->AddLine(BPoint(rect.right + 1, rect.bottom - 4),
				BPoint(rect.right + 1, rect.bottom - 3), lightenmax);
			view->AddLine(BPoint(rect.right + 2, rect.bottom - 2),
				BPoint(rect.right + 3, rect.bottom - 2), darken4);

			// going left
			view->AddLine(BPoint(rect.right + 4, rect.bottom - 1),
				BPoint(rect.left - 4, rect.bottom - 1), darken4);

			// after going left
			view->AddLine(BPoint(rect.left - 3, rect.bottom),
				BPoint(rect.left - 2, rect.bottom), darken4);
			view->AddLine(BPoint(rect.left - 1, rect.bottom + 1),
				BPoint(rect.left - 1, rect.bottom + 1), darken4);
			view->AddLine(BPoint(rect.left, rect.bottom + 2),
				BPoint(rect.left, rect.bottom + 2), darken4);
			break;

		default:
		case BTabView::kTopSide:
			// before going up
			view->AddLine(BPoint(rect.left - 1, rect.bottom - 1),
				BPoint(rect.left - 1, rect.bottom - 1), lightenmax);
			view->AddLine(BPoint(rect.left, rect.bottom - 2),
				BPoint(rect.left, rect.bottom - 3), lightenmax);

			// going up
			view->AddLine(BPoint(rect.left + 1, rect.bottom - 4),
				BPoint(rect.left + 1, rect.top + 5), lightenmax);

			// before going right
			view->AddLine(BPoint(rect.left + 2, rect.top + 4),
				BPoint(rect.left + 2, rect.top + 2), lightenmax);
			view->AddLine(BPoint(rect.left + 3, rect.top + 1),
				BPoint(rect.left + 4, rect.top + 1), lightenmax);

			// going right
			view->AddLine(BPoint(rect.left + 5, rect.top),
				BPoint(rect.right - 5, rect.top), lightenmax);

			// after going right
			view->AddLine(BPoint(rect.right - 4, rect.top + 1),
				BPoint(rect.right - 3, rect.top + 1), lightenmax);
			view->AddLine(BPoint(rect.right - 2, rect.top + 2),
				BPoint(rect.right - 2, rect.top + 3), darken4);

			// going down
			view->AddLine(BPoint(rect.right - 1, rect.top + 4),
				BPoint(rect.right - 1, rect.bottom - 4), darken4);

			// after going down
			view->AddLine(BPoint(rect.right, rect.bottom - 3),
				BPoint(rect.right, rect.bottom - 2), darken4);
			view->AddLine(BPoint(rect.right + 1, rect.bottom - 1),
				BPoint(rect.right + 1, rect.bottom - 1), darken4);
			view->AddLine(BPoint(rect.right + 2, rect.bottom),
				BPoint(rect.right + 2, rect.bottom), darken4);
			break;

		case BTabView::kBottomSide:
			// before going down
			view->AddLine(BPoint(rect.left - 1, rect.top - 1),
				BPoint(rect.left - 1, rect.top - 1), lightenmax);
			view->AddLine(BPoint(rect.left, rect.top - 2),
				BPoint(rect.left, rect.top - 3), lightenmax);

			// going down
			view->AddLine(BPoint(rect.left + 1, rect.top - 4),
				BPoint(rect.left + 1, rect.bottom + 5), lightenmax);

			// before going right
			view->AddLine(BPoint(rect.left + 2, rect.bottom + 4),
				BPoint(rect.left + 2, rect.bottom + 2), lightenmax);
			view->AddLine(BPoint(rect.left + 3, rect.bottom + 1),
				BPoint(rect.left + 4, rect.bottom + 1), lightenmax);

			// going right
			view->AddLine(BPoint(rect.left + 5, rect.bottom),
				BPoint(rect.right - 5, rect.bottom), lightenmax);

			// after going right
			view->AddLine(BPoint(rect.right - 4, rect.bottom + 1),
				BPoint(rect.right - 3, rect.bottom + 1), lightenmax);
			view->AddLine(BPoint(rect.right - 2, rect.bottom + 2),
				BPoint(rect.right - 2, rect.bottom + 3), darken4);

			// going up
			view->AddLine(BPoint(rect.right - 1, rect.bottom + 4),
				BPoint(rect.right - 1, rect.top - 4), darken4);

			// after going up
			view->AddLine(BPoint(rect.right, rect.top - 3),
				BPoint(rect.right, rect.top - 2), darken4);
			view->AddLine(BPoint(rect.right + 1, rect.top - 1),
				BPoint(rect.right + 1, rect.top - 1), darken4);
			view->AddLine(BPoint(rect.right + 2, rect.top),
				BPoint(rect.right + 2, rect.top), darken4);
			break;
	}
	view->EndLineArray();

	// undraw white line
	view->BeginLineArray(1);
	switch (side) {
		case BTabView::kLeftSide:
			view->AddLine(BPoint(rect.right, rect.top - 1),
				BPoint(rect.right, rect.bottom + 1), base);
			break;

		case BTabView::kRightSide:
			view->AddLine(BPoint(rect.left, rect.top - 1),
				BPoint(rect.left, rect.bottom + 1), base);
			break;

		default:
		case BTabView::kTopSide:
			view->AddLine(BPoint(rect.left - 1, rect.bottom),
				BPoint(rect.right + 1, rect.bottom), base);
			break;

		case BTabView::kBottomSide:
			view->AddLine(BPoint(rect.left - 1, rect.top),
				BPoint(rect.right + 1, rect.top), base);
			break;
	}
	view->EndLineArray();

	// inset rect for view contents
	rect.InsetBy(2, 2);

	view->PopState();
}


void
BeControlLook::DrawInactiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32 index, int32 selected,
	int32 first, int32 last)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	bool isFirst = index == first;
	bool isFull = index != selected - 1;

	view->PushState();

	// set clipping constraints to updateRect plus 2px extra
	BRegion clipping(updateRect.InsetByCopy(-2, -2));
	view->ConstrainClippingRegion(&clipping);

	// set colors and draw

	rgb_color lightenmax = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color darken4 = tint_color(base, B_DARKEN_4_TINT);
	rgb_color darkenmax = tint_color(base, B_DARKEN_MAX_TINT);

	view->SetHighColor(darkenmax);
	view->SetLowColor(base);

	view->BeginLineArray(12);

	switch (side) {
		case BTabView::kLeftSide:
			// only draw if first tab is unselected
			if (isFirst) {
				// before going left
				view->AddLine(BPoint(rect.right - 1, rect.top - 1),
					BPoint(rect.right - 1, rect.top - 1), lightenmax);
				view->AddLine(BPoint(rect.right - 2, rect.top),
					BPoint(rect.right - 3, rect.top), lightenmax);
			}

			// going left
			view->AddLine(BPoint(rect.right - 4, rect.top + 1),
				BPoint(rect.left + 5, rect.top + 1), lightenmax);

			// before going down
			view->AddLine(BPoint(rect.left + 2, rect.top + 2),
				BPoint(rect.left + 4, rect.top + 2), lightenmax);
			view->AddLine(BPoint(rect.left + 1, rect.top + 3),
				BPoint(rect.left + 1, rect.top + 4 ), lightenmax);

			// going down
			view->AddLine(BPoint(rect.left, rect.top + 5),
				BPoint(rect.left, rect.bottom - 5), lightenmax);

			// after going down
			view->AddLine(BPoint(rect.left + 1, rect.bottom - 4),
				BPoint(rect.left + 1, rect.bottom - 3), lightenmax);
			view->AddLine(BPoint(rect.left + 2, rect.bottom - 2),
				BPoint(rect.left + 3, rect.bottom - 2), darken4);

			// going right
			view->AddLine(BPoint(rect.left + 4, rect.bottom - 1),
				BPoint(rect.right - 4, rect.bottom - 1), darken4);

			// only draw if not before selected tab
			if (isFull) {
				// after going right
				view->AddLine(BPoint(rect.right - 3, rect.bottom),
					BPoint(rect.right - 2, rect.bottom), darken4);
				view->AddLine(BPoint(rect.right - 1, rect.bottom + 1),
					BPoint(rect.right - 1, rect.bottom + 1), darken4);
			}
			break;

		case BTabView::kRightSide:
			// only draw if first tab is unselected
			if (isFirst) {
				// before going right
				view->AddLine(BPoint(rect.left - 1, rect.top - 1),
					BPoint(rect.left - 1, rect.top - 1), lightenmax);
				view->AddLine(BPoint(rect.left - 2, rect.top),
					BPoint(rect.left - 3, rect.top), lightenmax);
			}

			// going right
			view->AddLine(BPoint(rect.left - 4, rect.top + 1),
				BPoint(rect.right + 5, rect.top + 1), lightenmax);

			// before going down
			view->AddLine(BPoint(rect.right + 2, rect.top + 2),
				BPoint(rect.right + 4, rect.top + 2), lightenmax);
			view->AddLine(BPoint(rect.right + 1, rect.top + 3),
				BPoint(rect.right + 1, rect.top + 4 ), lightenmax);

			// going down
			view->AddLine(BPoint(rect.right, rect.top + 5),
				BPoint(rect.right, rect.bottom - 5), lightenmax);

			// after going down
			view->AddLine(BPoint(rect.right + 1, rect.bottom - 4),
				BPoint(rect.right + 1, rect.bottom - 3), lightenmax);
			view->AddLine(BPoint(rect.right + 2, rect.bottom - 2),
				BPoint(rect.right + 3, rect.bottom - 2), darken4);

			// going left
			view->AddLine(BPoint(rect.right + 4, rect.bottom - 1),
				BPoint(rect.left - 4, rect.bottom - 1), darken4);

			// only draw if not before selected tab
			if (isFull) {
				// after going left
				view->AddLine(BPoint(rect.left - 3, rect.bottom),
					BPoint(rect.left - 2, rect.bottom), darken4);
				view->AddLine(BPoint(rect.left - 1, rect.bottom + 1),
					BPoint(rect.left - 1, rect.bottom + 1), darken4);
			}
			break;

		default:
		case BTabView::kTopSide:
			// only draw if first tab is unselected
			if (isFirst) {
				// before going up
				view->AddLine(BPoint(rect.left - 1, rect.bottom - 1),
					BPoint(rect.left - 1, rect.bottom - 1), lightenmax);
				view->AddLine(BPoint(rect.left, rect.bottom - 2),
					BPoint(rect.left, rect.bottom - 3), lightenmax);;
			}

			// going up
			view->AddLine(BPoint(rect.left + 1, rect.bottom - 4),
				BPoint(rect.left + 1, rect.top + 5), lightenmax);

			// before going right
			view->AddLine(BPoint(rect.left + 2, rect.top + 4),
				BPoint(rect.left + 2, rect.top + 2), lightenmax);
			view->AddLine(BPoint(rect.left + 3, rect.top + 1),
				BPoint(rect.left + 4, rect.top + 1), lightenmax);

			// going right
			view->AddLine(BPoint(rect.left + 5, rect.top),
				BPoint(rect.right - 5, rect.top), lightenmax);

			// after going right
			view->AddLine(BPoint(rect.right - 4, rect.top + 1),
				BPoint(rect.right - 3, rect.top + 1), lightenmax);
			view->AddLine(BPoint(rect.right - 2, rect.top + 2),
				BPoint(rect.right - 2, rect.top + 3), darken4);

			// going down
			view->AddLine(BPoint(rect.right - 1, rect.top + 4),
				BPoint(rect.right - 1, rect.bottom - 4), darken4);

			// only draw if not before selected tab
			if (isFull) {
				// after going down
				view->AddLine(BPoint(rect.right, rect.bottom - 3),
					BPoint(rect.right, rect.bottom - 2), darken4);
				view->AddLine(BPoint(rect.right + 1, rect.bottom - 1),
					BPoint(rect.right + 1, rect.bottom - 1), darken4);
			}
			break;

		case BTabView::kBottomSide:
			// only draw if first tab is unselected
			if (isFirst) {
				// before going down
				view->AddLine(BPoint(rect.left - 1, rect.top - 1),
					BPoint(rect.left - 1, rect.top - 1), lightenmax);
				view->AddLine(BPoint(rect.left, rect.top - 2),
					BPoint(rect.left, rect.top - 3), lightenmax);
			}

			// before going down
			view->AddLine(BPoint(rect.left + 1, rect.top - 4),
				BPoint(rect.left + 1, rect.bottom + 5), lightenmax);

			// before going right
			view->AddLine(BPoint(rect.left + 2, rect.bottom + 4),
				BPoint(rect.left + 2, rect.bottom + 2), lightenmax);
			view->AddLine(BPoint(rect.left + 3, rect.bottom + 1),
				BPoint(rect.left + 4, rect.bottom + 1), lightenmax);

			// going right
			view->AddLine(BPoint(rect.left + 5, rect.bottom),
				BPoint(rect.right - 5, rect.bottom), lightenmax);

			// after going right
			view->AddLine(BPoint(rect.right - 4, rect.bottom + 1),
				BPoint(rect.right - 3, rect.bottom + 1), lightenmax);
			view->AddLine(BPoint(rect.right - 2, rect.bottom + 2),
				BPoint(rect.right - 2, rect.bottom + 3), darken4);

			// going up
			view->AddLine(BPoint(rect.right - 1, rect.bottom + 4),
				BPoint(rect.right - 1, rect.top - 4), darken4);

			// only draw if not before selected tab
			if (isFull) {
				// after going up
				view->AddLine(BPoint(rect.right, rect.top - 3),
					BPoint(rect.right, rect.top - 2), darken4);
				view->AddLine(BPoint(rect.right + 1, rect.top - 1),
					BPoint(rect.right + 1, rect.top - 1), darken4);
			}
			break;
	}

	view->EndLineArray();

	// inset rect for view contents
	rect.InsetBy(2, 2);

	view->PopState();
}


void
BeControlLook::DrawSplitter(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, orientation orientation, uint32 flags,
	uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	rgb_color background;
	if ((flags & (B_CLICKED | B_ACTIVATED)) != 0)
		background = tint_color(base, B_DARKEN_1_TINT);
	else
		background = base;

	rgb_color light = tint_color(background, 0.6);
	rgb_color shadow = tint_color(background, 1.21);

	// frame
	if (borders != 0 && rect.Width() > 3 && rect.Height() > 3)
		DrawRaisedBorder(view, rect, updateRect, background, flags, borders);

	// dots and rest of background
	if (orientation == B_HORIZONTAL) {
		if (rect.Width() > 2) {
			// background on left/right
			BRegion region(rect);
			rect.left = floorf((rect.left + rect.right) / 2.0 - 0.5);
			rect.right = rect.left + 1;
			region.Exclude(rect);
			view->SetHighColor(background);
			view->FillRegion(&region);
		}

		BPoint dot = rect.LeftTop();
		BPoint stop = rect.LeftBottom();
		int32 num = 1;
		while (dot.y <= stop.y) {
			rgb_color col1;
			rgb_color col2;
			switch (num) {
				case 1:
					col1 = background;
					col2 = background;
					break;
				case 2:
					col1 = shadow;
					col2 = background;
					break;
				case 3:
				default:
					col1 = background;
					col2 = light;
					num = 0;
					break;
			}
			view->SetHighColor(col1);
			view->StrokeLine(dot, dot, B_SOLID_HIGH);
			view->SetHighColor(col2);
			dot.x++;
			view->StrokeLine(dot, dot, B_SOLID_HIGH);
			dot.x -= 1.0;
			// next pixel
			num++;
			dot.y++;
		}
	} else {
		if (rect.Height() > 2) {
			// background on left/right
			BRegion region(rect);
			rect.top = floorf((rect.top + rect.bottom) / 2.0 - 0.5);
			rect.bottom = rect.top + 1;
			region.Exclude(rect);
			view->SetHighColor(background);
			view->FillRegion(&region);
		}

		BPoint dot = rect.LeftTop();
		BPoint stop = rect.RightTop();
		int32 num = 1;
		while (dot.x <= stop.x) {
			rgb_color col1;
			rgb_color col2;
			switch (num) {
				case 1:
					col1 = background;
					col2 = background;
					break;
				case 2:
					col1 = shadow;
					col2 = background;
					break;
				case 3:
				default:
					col1 = background;
					col2 = light;
					num = 0;
					break;
			}
			view->SetHighColor(col1);
			view->StrokeLine(dot, dot, B_SOLID_HIGH);
			view->SetHighColor(col2);
			dot.y++;
			view->StrokeLine(dot, dot, B_SOLID_HIGH);
			dot.y -= 1.0;
			// next pixel
			num++;
			dot.x++;
		}
	}
}


//	#pragma mark - various borders


void
BeControlLook::DrawBorder(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, border_style borderStyle, uint32 flags,
	uint32 borders)
{
	if (borderStyle == B_NO_BORDER)
		return;

	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	BRegion clipping(updateRect);
	view->ConstrainClippingRegion(&clipping);

	rgb_color lightColor;
	rgb_color shadowColor;
	if (base.Brightness() > 128) {
		lightColor = tint_color(base, B_DARKEN_2_TINT);
		shadowColor = tint_color(base, B_LIGHTEN_2_TINT);
	} else {
		lightColor = tint_color(base, B_LIGHTEN_MAX_TINT);
		shadowColor = tint_color(base, B_DARKEN_3_TINT);
	}

	view->BeginLineArray(8);

	if (borderStyle == B_FANCY_BORDER) {
		if ((borders & B_LEFT_BORDER) != 0) {
			view->AddLine(rect.LeftBottom(), rect.LeftTop(), shadowColor);
			rect.left++;
		}
		if ((borders & B_TOP_BORDER) != 0) {
			view->AddLine(rect.LeftTop(), rect.RightTop(), shadowColor);
			rect.top++;
		}
		if ((borders & B_RIGHT_BORDER) != 0) {
			view->AddLine(rect.RightTop(), rect.RightBottom(), shadowColor);
			rect.right--;
		}
		if ((borders & B_BOTTOM_BORDER) != 0) {
			view->AddLine(rect.RightBottom(), rect.LeftBottom(), shadowColor);
			rect.bottom--;
		}

		if ((borders & B_LEFT_BORDER) != 0) {
			view->AddLine(rect.LeftBottom(), rect.LeftTop(), lightColor);
			rect.left++;
		}
		if ((borders & B_TOP_BORDER) != 0) {
			view->AddLine(rect.LeftTop(), rect.RightTop(), lightColor);
			rect.top++;
		}
		if ((borders & B_RIGHT_BORDER) != 0) {
			view->AddLine(rect.RightTop(), rect.RightBottom(), lightColor);
			rect.right--;
		}
		if ((borders & B_BOTTOM_BORDER) != 0) {
			view->AddLine(rect.RightBottom(), rect.LeftBottom(), lightColor);
			rect.bottom--;
		}
	} else if (borderStyle == B_PLAIN_BORDER) {
		if ((borders & B_LEFT_BORDER) != 0) {
			view->AddLine(rect.LeftBottom(), rect.LeftTop(), shadowColor);
			rect.left++;
		}
		if ((borders & B_TOP_BORDER) != 0) {
			view->AddLine(rect.LeftTop(), rect.RightTop(), shadowColor);
			rect.top++;
		}
		if ((borders & B_RIGHT_BORDER) != 0) {
			view->AddLine(rect.RightTop(), rect.RightBottom(), shadowColor);
			rect.right--;
		}
		if ((borders & B_BOTTOM_BORDER) != 0) {
			view->AddLine(rect.RightBottom(), rect.LeftBottom(), shadowColor);
			rect.bottom--;
		}

		if ((borders & B_LEFT_BORDER) != 0) {
			view->AddLine(rect.LeftBottom(), rect.LeftTop(), lightColor);
			rect.left++;
		}
		if ((borders & B_TOP_BORDER) != 0) {
			view->AddLine(rect.LeftTop(), rect.RightTop(), lightColor);
			rect.top++;
		}
		if ((borders & B_RIGHT_BORDER) != 0) {
			view->AddLine(rect.RightTop(), rect.RightBottom(), lightColor);
			rect.right--;
		}
		if ((borders & B_BOTTOM_BORDER) != 0) {
			view->AddLine(rect.RightBottom(), rect.LeftBottom(), lightColor);
			rect.bottom--;
		}
	}

	view->EndLineArray();

	view->PopState();
}


void
BeControlLook::DrawRaisedBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	BRegion clipping(updateRect);
	view->ConstrainClippingRegion(&clipping);

	rgb_color lightColor;
	rgb_color shadowColor;

	if ((flags & B_DISABLED) != 0) {
		lightColor = base;
		shadowColor = base;
	} else {
		lightColor = tint_color(base, 0.85);
		shadowColor = tint_color(base, 1.07);
	}

	view->BeginLineArray(8);

	if ((borders & B_LEFT_BORDER) != 0) {
		view->AddLine(rect.LeftBottom(), rect.LeftTop(), lightColor);
		rect.left++;
	}
	if ((borders & B_TOP_BORDER) != 0) {
		view->AddLine(rect.LeftTop(), rect.RightTop(), lightColor);
		rect.top++;
	}
	if ((borders & B_RIGHT_BORDER) != 0) {
		view->AddLine(rect.RightTop(), rect.RightBottom(), lightColor);
		rect.right--;
	}
	if ((borders & B_BOTTOM_BORDER) != 0) {
		view->AddLine(rect.RightBottom(), rect.LeftBottom(), lightColor);
		rect.bottom--;
	}

	if ((borders & B_LEFT_BORDER) != 0) {
		view->AddLine(rect.LeftBottom(), rect.LeftTop(), shadowColor);
		rect.left++;
	}
	if ((borders & B_TOP_BORDER) != 0) {
		view->AddLine(rect.LeftTop(), rect.RightTop(), shadowColor);
		rect.top++;
	}
	if ((borders & B_RIGHT_BORDER) != 0) {
		view->AddLine(rect.RightTop(), rect.RightBottom(), shadowColor);
		rect.right--;
	}
	if ((borders & B_BOTTOM_BORDER) != 0) {
		view->AddLine(rect.RightBottom(), rect.LeftBottom(), shadowColor);
		rect.bottom--;
	}

	view->EndLineArray();

	view->PopState();
}


void
BeControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	BRegion clipping(updateRect);
	view->ConstrainClippingRegion(&clipping);

	rgb_color lighten1 = tint_color(base, B_LIGHTEN_1_TINT);
	rgb_color lightenmax = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color darken1 = tint_color(base, B_DARKEN_1_TINT);
	rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT);
	rgb_color darken4 = tint_color(base, B_DARKEN_4_TINT);

	bool isEnabled = (flags & B_DISABLED) == 0;
	bool isFocused = (flags & B_FOCUSED) != 0;

	rgb_color bevelShadow;
	rgb_color bevelLight;

	// first bevel

	bevelShadow = isEnabled ? darken1 : base;
	bevelLight = isEnabled ? lightenmax : lighten1;

	view->BeginLineArray(4);
	if ((borders & B_LEFT_BORDER) != 0) {
		view->AddLine(rect.LeftBottom(), rect.LeftTop(), bevelShadow);
		rect.left++;
	}
	if ((borders & B_TOP_BORDER) != 0) {
		view->AddLine(rect.LeftTop(), rect.RightTop(), bevelShadow);
		rect.top++;
	}
	if ((borders & B_RIGHT_BORDER) != 0) {
		view->AddLine(BPoint(rect.left + 1, rect.bottom), rect.RightBottom(),
			bevelLight);
		rect.right--;
	}
	if ((borders & B_BOTTOM_BORDER) != 0) {
		view->AddLine(rect.RightBottom(), BPoint(rect.right, rect.top + 1),
			bevelLight);
		rect.bottom--;
	}
	view->EndLineArray();

	// second bevel

	if (isEnabled && isFocused) {
		view->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		view->StrokeRect(rect);
		rect.InsetBy(1, 1);
	} else {
		bevelShadow = isEnabled ? darken4 : darken2;
		bevelLight = base;

		view->BeginLineArray(4);
		if ((borders & B_LEFT_BORDER) != 0) {
			view->AddLine(rect.LeftBottom(), rect.LeftTop(), bevelShadow);
			rect.left++;
		}
		if ((borders & B_TOP_BORDER) != 0) {
			view->AddLine(rect.LeftTop(), rect.RightTop(), bevelShadow);
			rect.top++;
		}
		if ((borders & B_RIGHT_BORDER) != 0) {
			view->AddLine(BPoint(rect.left + 1, rect.bottom), rect.RightBottom(),
				bevelLight);
			rect.right--;
		}
		if ((borders & B_BOTTOM_BORDER) != 0) {
			view->AddLine(rect.RightBottom(), BPoint(rect.right, rect.top + 1),
				bevelLight);
			rect.bottom--;
		}
		view->EndLineArray();
	}

	view->PopState();
}


void
BeControlLook::DrawGroupFrame(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 borders)
{
	DrawBorder(view, rect, updateRect, base, B_FANCY_BORDER, 0, borders);
}


//	#pragma mark - Labels


void
BeControlLook::DrawLabel(BView* view, const char* label, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	const rgb_color* textColor)
{
	DrawLabel(view, label, NULL, rect, updateRect, base, flags,
		DefaultLabelAlignment(), textColor);
}


void
BeControlLook::DrawLabel(BView* view, const char* label, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	const BAlignment& alignment, const rgb_color* textColor)
{
	DrawLabel(view, label, NULL, rect, updateRect, base, flags, alignment,
		textColor);
}


void
BeControlLook::DrawLabel(BView* view, const char* label, const rgb_color& base,
	uint32 flags, const BPoint& where, const rgb_color* textColor)
{
	view->PushState();

	bool isButton = (flags & B_FLAT) != 0 || (flags & B_HOVER) != 0
		|| (flags & B_DEFAULT_BUTTON) != 0;
	bool isEnabled = (flags & B_DISABLED) == 0;
	bool isActivated = (flags & B_ACTIVATED) != 0;

	BWindow* window = view->Window();
	bool isDesktop = window != NULL
		&& window->Feel() == kDesktopWindowFeel
		&& window->Look() == kDesktopWindowLook
		&& view->Parent()
		&& view->Parent()->Parent() == NULL
		&& (flags & B_IGNORE_OUTLINE) == 0;

	rgb_color low;
	rgb_color color;
	rgb_color glowColor;

	if (textColor != NULL)
		glowColor = *textColor;
	else if ((flags & B_IS_CONTROL) != 0)
		glowColor = ui_color(B_CONTROL_TEXT_COLOR);
	else
		glowColor = ui_color(B_PANEL_TEXT_COLOR);

	color = glowColor;

	if (isDesktop)
		low = view->Parent()->ViewColor();
	else
		low = base;

	if (!isEnabled) {
		color.red = (uint8)(((int32)low.red + color.red + 1) / 2);
		color.green = (uint8)(((int32)low.green + color.green + 1) / 2);
		color.blue = (uint8)(((int32)low.blue + color.blue + 1) / 2);
	}

	if (isDesktop) {
		// enforce proper use of desktop label colors
		if (low.Brightness() < 100) {
			if (textColor == NULL)
				color = make_color(255, 255, 255);

			glowColor = make_color(0, 0, 0);
		} else {
			if (textColor == NULL)
				color = make_color(0, 0, 0);

			glowColor = make_color(255, 255, 255);
		}

		// drawing occurs on the desktop
		if (fCachedWorkspace != current_workspace()) {
			int8 indice = 0;
			int32 mask;
			bool tmpOutline;
			while (fBackgroundInfo.FindInt32("be:bgndimginfoworkspaces",
					indice, &mask) == B_OK
				&& fBackgroundInfo.FindBool("be:bgndimginfoerasetext",
					indice, &tmpOutline) == B_OK) {

				if (((1 << current_workspace()) & mask) != 0) {
					fCachedOutline = tmpOutline;
					fCachedWorkspace = current_workspace();
					break;
				}
				indice++;
			}
		}

		if (fCachedOutline) {
			BFont font;
			view->GetFont(&font);

			view->SetDrawingMode(B_OP_ALPHA);
			view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
			// Draw glow or outline
			if (glowColor.Brightness() > 128) {
				font.SetFalseBoldWidth(2.0);
				view->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);

				glowColor.alpha = 30;
				view->SetHighColor(glowColor);
				view->DrawString(label, where);

				font.SetFalseBoldWidth(1.0);
				view->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);

				glowColor.alpha = 65;
				view->SetHighColor(glowColor);
				view->DrawString(label, where);

				font.SetFalseBoldWidth(0.0);
				view->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);
			} else {
				font.SetFalseBoldWidth(1.0);
				view->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);

				glowColor.alpha = 30;
				view->SetHighColor(glowColor);
				view->DrawString(label, where);

				font.SetFalseBoldWidth(0.0);
				view->SetFont(&font, B_FONT_FALSE_BOLD_WIDTH);

				glowColor.alpha = 200;
				view->SetHighColor(glowColor);
				view->DrawString(label, BPoint(where.x + 1, where.y + 1));
			}
		}
	}

	rgb_color invertedIfClicked = color;
	if (isButton && isEnabled && isActivated) {
		// only for enabled and activated buttons
		invertedIfClicked.red = 255 - invertedIfClicked.red;
		invertedIfClicked.green = 255 - invertedIfClicked.green;
		invertedIfClicked.blue = 255 - invertedIfClicked.blue;
	}

	view->SetLowColor(invertedIfClicked);
	view->SetHighColor(invertedIfClicked);
	view->SetDrawingMode(B_OP_OVER);
	view->DrawString(label, where);
	view->SetDrawingMode(B_OP_COPY);

	view->PopState();
}


void
BeControlLook::DrawLabel(BView* view, const char* label, const BBitmap* icon,
	BRect rect, const BRect& updateRect, const rgb_color& base, uint32 flags,
	const BAlignment& alignment, const rgb_color* textColor)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	if (label == NULL && icon == NULL)
		return;

	if (label == NULL && icon != NULL) {
		// icon only
		BRect alignedRect = BLayoutUtils::AlignInFrame(
			rect.OffsetByCopy(-2, -2),
			icon->Bounds().Size(), alignment);
		view->SetDrawingMode(B_OP_OVER);
		view->DrawBitmap(icon, alignedRect.LeftTop());
		view->SetDrawingMode(B_OP_COPY);
		return;
	}

	view->PushState();

	// label, possibly with icon
	float availableWidth = rect.Width() + 1;
	float width = 0;
	float textOffset = 0;
	float height = 0;

	if (icon != NULL && label != NULL) {
		// move text over to fit icon
		width = icon->Bounds().Width() + DefaultLabelSpacing() + 1;
		height = icon->Bounds().Height() + 1;
		textOffset = width;
		availableWidth -= textOffset;
	}

	// truncate the label if necessary and get the width and height
	BString truncatedLabel(label);

	BFont font;
	view->GetFont(&font);

	font.TruncateString(&truncatedLabel, B_TRUNCATE_MIDDLE, availableWidth);
	width += ceilf(font.StringWidth(truncatedLabel.String()));

	font_height fontHeight;
	font.GetHeight(&fontHeight);
	float textHeight = ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);
	height = std::max(height, textHeight);

	// handle alignment
	BRect alignedRect(BLayoutUtils::AlignOnRect(rect,
		BSize(width - 1, height - 1), alignment));

	if (icon != NULL) {
		BPoint location(alignedRect.LeftTop());
		if (icon->Bounds().Height() + 1 < height)
			location.y += ceilf((height - icon->Bounds().Height() - 1) / 2);

		view->SetDrawingMode(B_OP_OVER);
		view->DrawBitmap(icon, location);
		view->SetDrawingMode(B_OP_COPY);
	}

	BPoint location(alignedRect.left + textOffset,
		alignedRect.top + ceilf(fontHeight.ascent));
	if (textHeight < height)
		location.y += ceilf((height - textHeight) / 2);

	if ((flags & B_FOCUSED) != 0) {
		// draw underline under label
		float x = location.x;
		float y = location.y + ceilf(fontHeight.descent);
		view->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		view->StrokeLine(BPoint(x, y),
			BPoint(x + view->StringWidth(truncatedLabel.String()), y));
	}

	DrawLabel(view, truncatedLabel.String(), base, flags, location, textColor);

	view->PopState();
}


void
BeControlLook::GetFrameInsets(frame_type frameType, uint32 flags, float& _left,
	float& _top, float& _right, float& _bottom)
{
	// All frames have the same inset on each side.
	float inset = 0;

	switch (frameType) {
		case B_BUTTON_FRAME:
			inset = (flags & B_DEFAULT_BUTTON) != 0 ? 5 : 2;
			break;

		case B_GROUP_FRAME:
		case B_MENU_FIELD_FRAME:
			inset = 3;
			break;

		case B_SCROLL_VIEW_FRAME:
		case B_TEXT_CONTROL_FRAME:
			inset = 2;
			break;
	}

	_left = inset;
	_top = inset;
	_right = inset;
	_bottom = inset;
}


void
BeControlLook::GetBackgroundInsets(background_type backgroundType,
	uint32 flags, float& _left, float& _top, float& _right, float& _bottom)
{
	// Most backgrounds have the same inset on each side.
	float inset = 0;

	switch (backgroundType) {
		case B_BUTTON_BACKGROUND:
		case B_MENU_BACKGROUND:
		case B_MENU_BAR_BACKGROUND:
		case B_MENU_FIELD_BACKGROUND:
		case B_MENU_ITEM_BACKGROUND:
			inset = 1;
			break;
		case B_BUTTON_WITH_POP_UP_BACKGROUND:
			_left = 1;
			_top = 1;
			_right = 1 + kButtonPopUpIndicatorWidth;
			_bottom = 1;
			return;
		case B_HORIZONTAL_SCROLL_BAR_BACKGROUND:
			_left = 2;
			_top = 0;
			_right = 1;
			_bottom = 0;
			return;
		case B_VERTICAL_SCROLL_BAR_BACKGROUND:
			_left = 0;
			_top = 2;
			_right = 0;
			_bottom = 1;
			return;
	}

	_left = inset;
	_top = inset;
	_right = inset;
	_bottom = inset;
}


void
BeControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, 0.0f, 0.0f, 0.0f, 0.0f,
		base, true, flags, borders, orientation);
}


void
BeControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, float radius, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, radius, radius, radius,
		radius, base, true, flags, borders, orientation);
}


void
BeControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	uint32 flags, uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, leftTopRadius,
		rightTopRadius, leftBottomRadius, rightBottomRadius, base, true, flags,
		borders, orientation);
}


//	#pragma mark - protected methods


void
BeControlLook::_DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float, const rgb_color& base,
	const rgb_color& background, float contrast, float brightness,
	uint32 flags, uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	// set clipping constraints to updateRect
	BRegion clipping(updateRect);
	view->ConstrainClippingRegion(&clipping);

	// flags
	bool isEnabled = (flags & B_DISABLED) == 0;
	bool isDefault = (flags & B_DEFAULT_BUTTON) != 0;

	// colors
	rgb_color lighten1 = tint_color(base, B_LIGHTEN_1_TINT); // 231
	lighten1.red++; lighten1.green++; lighten1.blue++; // 232 = 231 + 1
	rgb_color lighten2 = tint_color(base, B_LIGHTEN_2_TINT);
	rgb_color lightenMax = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color darken1 = tint_color(base, B_DARKEN_1_TINT); // 184
	rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT); // 158
	rgb_color darken3 = tint_color(base, B_DARKEN_3_TINT);
	rgb_color darken4 = tint_color(base, B_DARKEN_4_TINT); // 96

	rgb_color buttonBgColor = lighten1;
	rgb_color lightColor;

	rgb_color dark1BorderColor;
	rgb_color dark2BorderColor;

	rgb_color bevelColor1;
	rgb_color bevelColor2;
	rgb_color bevelColorRBCorner;

	rgb_color borderBevelShadow;
	rgb_color borderBevelLight;

	if (isEnabled) {
		lightColor = tint_color(base, B_LIGHTEN_2_TINT);
		dark1BorderColor = darken3;
		dark2BorderColor = darken4;
		bevelColor1 = darken2;
		bevelColor2 = lighten1;

		if (isDefault) {
			borderBevelShadow = tint_color(dark1BorderColor,
				(B_NO_TINT + B_DARKEN_1_TINT) / 2);
			borderBevelLight = tint_color(dark1BorderColor, B_LIGHTEN_1_TINT);

			borderBevelLight.red = (borderBevelLight.red + base.red) / 2;
			borderBevelLight.green = (borderBevelLight.green + base.green) / 2;
			borderBevelLight.blue = (borderBevelLight.blue + base.blue) / 2;

			dark1BorderColor = darken3;
			dark2BorderColor = darken4;

			bevelColorRBCorner = borderBevelShadow;
		} else {
			borderBevelShadow = tint_color(base,
				(B_NO_TINT + B_DARKEN_1_TINT) / 2);
			borderBevelLight = buttonBgColor;
			bevelColorRBCorner = dark1BorderColor;
		}
	} else {
		lightColor = lighten2;
		dark1BorderColor = darken1;
		dark2BorderColor = darken2;
		bevelColor1 = base;
		bevelColor2 = buttonBgColor;

		if (isDefault) {
			borderBevelShadow = dark1BorderColor;
			borderBevelLight = base;
			dark1BorderColor = tint_color(dark1BorderColor, B_DARKEN_1_TINT);
			dark2BorderColor = tint_color(dark1BorderColor, 1.16);
		} else {
			borderBevelShadow = base;
			borderBevelLight = base;
		}

		bevelColorRBCorner = tint_color(base, 1.08);
	}

	if (isDefault) {
		if (isEnabled) {
			// dark border
			view->BeginLineArray(4);
			if ((borders & B_LEFT_BORDER) != 0) {
				view->AddLine(BPoint(rect.left, rect.top + 1),
					BPoint(rect.left, rect.bottom - 1), dark2BorderColor);
				rect.left++;
			}
			if ((borders & B_TOP_BORDER) != 0) {
				view->AddLine(BPoint(rect.left, rect.top),
					BPoint(rect.right - 1, rect.top), dark2BorderColor);
				rect.top++;
			}
			if ((borders & B_RIGHT_BORDER) != 0) {
				view->AddLine(BPoint(rect.right, rect.top),
					BPoint(rect.right, rect.bottom - 1), dark2BorderColor);
				rect.right--;
			}
			if ((borders & B_BOTTOM_BORDER) != 0) {
				view->AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.right, rect.bottom), dark2BorderColor);
				rect.bottom--;
			}
			view->EndLineArray();

			// bevel
			view->SetHighColor(darken1);
			view->StrokeRect(rect);

			rect.InsetBy(1, 1);

			// fill
			view->SetHighColor(lighten1);
			view->FillRect(rect);

			rect.InsetBy(2, 2);
		} else {
			// dark border
			view->BeginLineArray(4);
			if ((borders & B_LEFT_BORDER) != 0) {
				view->AddLine(BPoint(rect.left, rect.top + 1),
					BPoint(rect.left, rect.bottom - 1), darken1);
				rect.left++;
			}
			if ((borders & B_TOP_BORDER) != 0) {
				view->AddLine(BPoint(rect.left, rect.top),
					BPoint(rect.right - 1, rect.top), darken1);
				rect.top++;
			}
			if ((borders & B_RIGHT_BORDER) != 0) {
				view->AddLine(BPoint(rect.right, rect.top),
					BPoint(rect.right, rect.bottom - 1), darken1);
				rect.right--;
			}
			if ((borders & B_BOTTOM_BORDER) != 0) {
				view->AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.right, rect.bottom), darken1);
				rect.bottom--;
			}
			view->EndLineArray();

			// fill
			view->SetHighColor(lighten1);
			view->FillRect(rect);

			rect.InsetBy(3, 3);
		}
	} else {
		// if not default button, inset top and bottom by 1px
		rect.InsetBy(1, 0);
	}

	// stroke frame to draw four corners, then write on top
	view->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view->StrokeRect(rect);

	view->BeginLineArray(16);

	// external border
	view->AddLine(BPoint(rect.left, rect.top + 1),
		BPoint(rect.left, rect.bottom - 1),
		(borders & B_LEFT_BORDER) != 0 ? dark2BorderColor : darken1);
	view->AddLine(BPoint(rect.left + 1, rect.top),
		BPoint(rect.right - 1, rect.top),
		(borders & B_TOP_BORDER) != 0 ? dark2BorderColor : darken1);
	view->AddLine(BPoint(rect.right, rect.top + 1),
		BPoint(rect.right, rect.bottom - 1),
		(borders & B_RIGHT_BORDER) != 0 ? dark2BorderColor : darken1);
	view->AddLine(BPoint(rect.left + 1, rect.bottom),
		BPoint(rect.right - 1, rect.bottom),
		(borders & B_BOTTOM_BORDER) != 0 ? dark2BorderColor : darken1);

	// inset past external border
	rect.InsetBy(1, 1);

	// internal bevel
	view->AddLine(rect.LeftBottom(), rect.LeftTop(), bevelColor2);
	view->AddLine(rect.LeftTop(), rect.RightTop(), bevelColor2);
	view->AddLine(BPoint(rect.right, rect.top + 1), rect.RightBottom(),
		bevelColor1);
	view->AddLine(rect.RightBottom(), BPoint(rect.left + 1, rect.bottom),
		bevelColor1);

	// inset past internal bevel
	rect.InsetBy(1, 1);

	// internal gloss outside
	view->AddLine(BPoint(rect.left, rect.bottom + 1), rect.LeftTop(),
		lightenMax); // come down an extra pixel
	view->AddLine(rect.LeftTop(), rect.RightTop(), lightenMax);
	view->AddLine(rect.RightTop(), rect.RightBottom(), base);
	view->AddLine(rect.RightBottom(), BPoint(rect.left + 1, rect.bottom),
		base); // compensate for extra pixel

	// inset past gloss outside
	rect.InsetBy(1, 1);

	// internal gloss inside
	view->AddLine(BPoint(rect.left, rect.bottom + 1), rect.LeftTop(),
		lightenMax); // come down an extra pixel
	view->AddLine(rect.LeftTop(), rect.RightTop(), lightenMax);
	view->AddLine(rect.RightTop(), rect.RightBottom(), buttonBgColor);
	view->AddLine(rect.RightBottom(), BPoint(rect.left + 1, rect.bottom),
		buttonBgColor); // compensate for extra pixel

	// inset past gloss inside
	rect.InsetBy(1, 1);

	view->EndLineArray();

	view->PopState();
}


void
BeControlLook::_DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float, const rgb_color& base,
	bool popupIndicator, uint32 flags, uint32 borders, orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// fill the button area
	view->SetHighColor(tint_color(base, B_LIGHTEN_1_TINT));
	view->FillRect(rect);

	bool isEnabled = (flags & B_DISABLED) == 0;
	bool isActivated = (flags & B_ACTIVATED) != 0;

	if (isEnabled && isActivated) {
		// invert if clicked without altering rect
		BRect invertRect(rect);
		invertRect.left -= 3;
		invertRect.top -= 3;
		invertRect.right += 3;
		invertRect.bottom += 3;
		view->InvertRect(invertRect);
	}
}


void
BeControlLook::_DrawPopUpMarker(BView* view, const BRect& rect,
	const rgb_color& base, uint32 flags)
{
	bool isEnabled = (flags & B_DISABLED) == 0;

	BPoint position(rect.right - 8, rect.bottom - 8);
	BPoint triangle[3];
	triangle[0] = position + BPoint(-2.5, -0.5);
	triangle[1] = position + BPoint(2.5, -0.5);
	triangle[2] = position + BPoint(0.0, 2.0);

	uint32 viewFlags = view->Flags();
	view->SetFlags(viewFlags | B_SUBPIXEL_PRECISE);

	rgb_color markerColor = isEnabled
		? tint_color(base, B_DARKEN_4_TINT)
		: tint_color(base, B_DARKEN_2_TINT);

	view->SetHighColor(markerColor);
	view->FillTriangle(triangle[0], triangle[1], triangle[2]);

	view->SetFlags(viewFlags);
}


void
BeControlLook::_DrawMenuFieldBackgroundOutside(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// BeControlLook does not support rounded corners and it never will
	if (popupIndicator) {
		BRect rightRect(rect);
		rightRect.left = rect.right - 10;

		_DrawMenuFieldBackgroundInside(view, rect, updateRect,
			0, 0, 0, 0, base, flags, B_ALL_BORDERS);
		_DrawPopUpMarker(view, rightRect, base, flags);
	} else {
		_DrawMenuFieldBackgroundInside(view, rect, updateRect, 0, 0,
			0, 0, base, flags);
	}
}


void
BeControlLook::_DrawMenuFieldBackgroundInside(BView* view, BRect& rect,
	const BRect& updateRect, float, float, float, float, const rgb_color& base,
	uint32 flags, uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	// set clipping constraints to updateRect
	BRegion clipping(updateRect);
	view->ConstrainClippingRegion(&clipping);

	// flags
	bool isEnabled = (flags & B_DISABLED) == 0;

	// colors
	rgb_color darken4;
	rgb_color darken1;
	rgb_color lighten1;
	rgb_color lighten2;

	if (isEnabled) {
		darken4 = tint_color(base, B_DARKEN_4_TINT);
		darken1 = tint_color(base, B_DARKEN_1_TINT);
		lighten1 = tint_color(base, B_LIGHTEN_1_TINT);
		lighten2 = tint_color(base, B_LIGHTEN_2_TINT);
	} else {
		darken4 = tint_color(base, B_DARKEN_2_TINT);
		darken1 = tint_color(base, (B_NO_TINT + B_DARKEN_1_TINT) / 2.0);
		lighten1 = tint_color(base, (B_NO_TINT + B_LIGHTEN_1_TINT) / 2.0);
		lighten2 = tint_color(base, B_LIGHTEN_1_TINT);
	}

	// fill background
	view->SetHighColor(base);
	view->FillRect(rect);

	// draw shadow lines around bottom and right sides
	view->BeginLineArray(6);

	// bottom below item text, darker then BMenuBar
	// would normaly draw it
	view->AddLine(BPoint(rect.left, rect.bottom),
		BPoint(rect.left - 1, rect.bottom), darken4);

	// bottom below popup marker
	view->AddLine(BPoint(rect.left, rect.bottom),
		BPoint(rect.right, rect.bottom), darken4);
	// right of popup marker
	view->AddLine(BPoint(rect.right, rect.bottom - 1),
		BPoint(rect.right, rect.top), darken4);
	// top above popup marker
	view->AddLine(BPoint(rect.left, rect.top),
		BPoint(rect.right - 2, rect.top), lighten2);

	rect.top += 1;
	rect.bottom -= 1;
	rect.right -= 1;

	// bottom below popup marker
	view->AddLine(BPoint(rect.left, rect.bottom),
		BPoint(rect.right, rect.bottom), darken1);
	// right of popup marker
	view->AddLine(BPoint(rect.right, rect.bottom - 1),
		BPoint(rect.right, rect.top), darken1);

	view->EndLineArray();

	rect.bottom -= 1;
	rect.right -= 1;
	view->SetHighColor(base);
	view->FillRect(rect);

	view->PopState();
}


void
BeControlLook::_DrawScrollBarBackgroundFirst(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	BRegion clipping(updateRect);
	view->ConstrainClippingRegion(&clipping);

	bool isEnabled = (flags & B_DISABLED) == 0;
	BRect orig(rect);

	// border = 152
	rgb_color border = tint_color(base, B_DARKEN_2_TINT);
	rgb_color shine, shadow, bg;
	if (isEnabled) {
		// shine = 216, shadow = 184, bg = 200
		shine = base;
		shadow = tint_color(base, B_DARKEN_1_TINT);
		bg = tint_color(base, 1.074);
	} else {
		// shine = 255, shadow = bg = 240
		shine = tint_color(base, B_LIGHTEN_MAX_TINT);
		rgb_color lighten2 = tint_color(base, B_LIGHTEN_2_TINT);
		lighten2.red++; lighten2.green++; lighten2.blue++;
			// lighten2 = 239, 240 = 239 + 1
		shadow = bg = lighten2;
	}

	// fill background, we'll draw arrows and thumb on top
	view->SetDrawingMode(B_OP_COPY);

	view->BeginLineArray(5);
	if (orientation == B_VERTICAL) {
		// left shadow
		view->AddLine(rect.LeftTop(), rect.LeftBottom(),
			isEnabled ? shadow : shine);
		rect.left++;
		view->AddLine(rect.LeftTop(), rect.LeftBottom(), shadow);
		rect.left++;
		// top shadow
		view->AddLine(rect.LeftTop(), rect.RightTop(),
			isEnabled ? shadow : shine);
		rect.top++;
		view->AddLine(rect.LeftTop(), rect.RightTop(), shadow);
		rect.top++;
		// shine
		view->AddLine(rect.RightTop(), rect.RightBottom(), base);
		rect.right--;
	} else {
		// left shadow
		view->AddLine(rect.LeftTop(), rect.LeftBottom(), shadow);
		rect.left++;
		view->AddLine(rect.LeftTop(), rect.LeftBottom(), shadow);
		rect.left++;
		// top shadow
		view->AddLine(rect.LeftTop(), rect.RightTop(), shadow);
		rect.top++;
		view->AddLine(rect.LeftTop(), rect.RightTop(), shadow);
		rect.top++;
		// shine
		view->AddLine(rect.LeftBottom(), rect.RightBottom(), base);
		rect.bottom--;
	}
	view->EndLineArray();

	// fill bg
	view->SetHighColor(bg);
	view->FillRect(rect);

	rect = orig;

	// draw border last
	view->BeginLineArray(2);
	if (orientation == B_VERTICAL) {
		// top border
		view->AddLine(rect.LeftTop(), rect.RightTop(), border);
		// bottom border
		view->AddLine(rect.LeftBottom(), rect.RightBottom(), border);
	} else {
		// left border
		view->AddLine(rect.LeftTop(), rect.LeftBottom(), border);
		// right border
		view->AddLine(rect.RightTop(), rect.RightBottom(), border);
	}
	view->EndLineArray();

	view->PopState();
}


void
BeControlLook::_DrawScrollBarBackgroundSecond(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	view->PushState();

	BRegion clipping(updateRect);
	view->ConstrainClippingRegion(&clipping);

	bool isEnabled = (flags & B_DISABLED) == 0;

	BRect orig(rect);

	// border = 152
	rgb_color border = tint_color(base, B_DARKEN_2_TINT);
	rgb_color darkBorder, shine, shadow, bg;
	if (isEnabled) {
		// darkBorder = 96 shine = 216, shadow = 184, bg = 200
		darkBorder = tint_color(base, B_DARKEN_4_TINT);
		shine = base;
		shadow = tint_color(base, B_DARKEN_1_TINT);
		bg = tint_color(base, 1.074);
	} else {
		// darkBorder = 184, shine = 255, shadow = bg = 240
		darkBorder = tint_color(base, B_DARKEN_1_TINT);
		shine = tint_color(base, B_LIGHTEN_MAX_TINT);
		rgb_color lighten2 = tint_color(base, B_LIGHTEN_2_TINT);
		lighten2.red++; lighten2.green++; lighten2.blue++;
			// lighten2 = 239, 240 = 239 + 1
		shadow = bg = lighten2;
	}

	// fill background, we'll draw arrows and thumb on top
	view->SetDrawingMode(B_OP_COPY);

	view->BeginLineArray(3);
	if (orientation == B_VERTICAL) {
		// left shadow (no top shadow on second rect)
		view->AddLine(rect.LeftTop(), rect.LeftBottom(),
			isEnabled ? shadow : shine);
		rect.left++;
		view->AddLine(rect.LeftTop(), rect.LeftBottom(), shadow);
		rect.left++;
		// shine (use base color)
		view->AddLine(rect.RightTop(), rect.RightBottom(), base);
		rect.right--;
	} else {
		// left shadow (no top shadow on second rect)
		view->AddLine(rect.LeftTop(), rect.RightTop(),
			isEnabled ? shadow : shine);
		rect.top++;
		view->AddLine(rect.LeftTop(), rect.RightTop(), shadow);
		rect.top++;
		// shine (use base color)
		view->AddLine(rect.LeftBottom(), rect.RightBottom(), base);
		rect.bottom--;
	}
	view->EndLineArray();

	// fill bg
	view->SetHighColor(bg);
	view->FillRect(rect);

	rect = orig;

	// draw border over bg
	view->BeginLineArray(2);
	if (orientation == B_VERTICAL) {
		// top border
		view->AddLine(rect.LeftTop(), rect.RightTop(), darkBorder);
		// bottom border
		view->AddLine(rect.LeftBottom(), rect.RightBottom(), border);
	} else {
		// left border
		view->AddLine(rect.LeftTop(), rect.LeftBottom(), darkBorder);
		// right border
		view->AddLine(rect.RightTop(), rect.RightBottom(), border);
	}
	view->EndLineArray();

	view->PopState();
}

void
BeControlLook::_DrawScrollBarKnobDot(BView* view,
	float hcenter, float vmiddle, rgb_color dark, rgb_color light,
	orientation orientation)
{
	// orientation is unused
	view->BeginLineArray(4);
	view->AddLine(BPoint(hcenter + 2, vmiddle - 2),
		BPoint(hcenter + 2, vmiddle + 2), dark);
	view->AddLine(BPoint(hcenter + 2, vmiddle + 2),
		BPoint(hcenter - 2, vmiddle + 2), dark);
	view->AddLine(BPoint(hcenter - 2, vmiddle + 1),
		BPoint(hcenter - 2, vmiddle - 1), light);
	view->AddLine(BPoint(hcenter - 2, vmiddle - 1),
		BPoint(hcenter - 2, vmiddle + 1), light);
	view->EndLineArray();
}


void
BeControlLook::_DrawScrollBarKnobLine(BView* view,
	float hcenter, float vmiddle, rgb_color dark, rgb_color light,
	orientation orientation)
{
	if (orientation == B_HORIZONTAL) {
		view->BeginLineArray(4);
		view->AddLine(BPoint(hcenter, vmiddle + 3),
			BPoint(hcenter + 1, vmiddle + 3), dark);
		view->AddLine(BPoint(hcenter + 1, vmiddle + 3),
			BPoint(hcenter + 1, vmiddle - 3), dark);
		view->AddLine(BPoint(hcenter, vmiddle - 3),
			BPoint(hcenter - 1, vmiddle - 3), light);
		view->AddLine(BPoint(hcenter - 1, vmiddle - 3),
			BPoint(hcenter - 1, vmiddle + 3), light);
		view->EndLineArray();
	} else {
		view->BeginLineArray(4);
		view->AddLine(BPoint(hcenter + 3, vmiddle),
			BPoint(hcenter + 3, vmiddle + 1), dark);
		view->AddLine(BPoint(hcenter + 3, vmiddle + 1),
			BPoint(hcenter - 3, vmiddle + 1), dark);
		view->AddLine(BPoint(hcenter - 3, vmiddle),
			BPoint(hcenter - 3, vmiddle - 1), light);
		view->AddLine(BPoint(hcenter - 3, vmiddle - 1),
			BPoint(hcenter + 3, vmiddle - 1), light);
		view->EndLineArray();
	}
}

} // namespace BPrivate


extern "C" BControlLook* (instantiate_control_look)(image_id id)
{
	return new (std::nothrow)BPrivate::BeControlLook(id);
}
