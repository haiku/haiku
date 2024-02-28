/*
 * Copyright 2021-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		John Scipione, jscipione@gmail.com
 *		Nahuel Tello, ntello@unarix.com.ar
 */


/*! FlatControlLook flat Haiku */


#include "FlatControlLook.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <stdio.h>

#include <GradientLinear.h>
#include <Rect.h>
#include <Region.h>
#include <View.h>
#include <WindowPrivate.h>


namespace BPrivate {

static const float kEdgeBevelLightTint = 1.0;
static const float kEdgeBevelShadowTint = 1.0;
static const float kHoverTintFactor = 0.55;
static const float kRadius = 3.0f;

static const float kButtonPopUpIndicatorWidth = 11;


FlatControlLook::FlatControlLook()
	: HaikuControlLook()
{
}


FlatControlLook::~FlatControlLook()
{
}


// #pragma mark -


void
FlatControlLook::DrawButtonFrame(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, const rgb_color& background, uint32 flags,
	uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius, base, background,
		1.0, 1.0, flags, borders);
}


void
FlatControlLook::DrawButtonFrame(BView* view, BRect& rect, const BRect& updateRect,
	float radius, const rgb_color& base, const rgb_color& background, uint32 flags,
	uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius, base, background,
		1.0, 1.0, flags, borders);
}


void
FlatControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	const rgb_color& background, uint32 flags,
	uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius, base, background,
		1.0, 1.0, flags, borders);
}


void
FlatControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius, base, false,
		flags, borders, orientation);
}


void
FlatControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float radius, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius, base, false,
		flags, borders, orientation);
}


void
FlatControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	uint32 flags, uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius, base, false,
		flags, borders, orientation);
}


void
FlatControlLook::DrawMenuBarBackground(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// the surface edges

	// colors
	float topTint = 1.0;
	float bottomTint = 1.0;

	rgb_color customColor = base;
	bool isEnabled = (flags & B_DISABLED) != 0;
	bool isFocused = (flags & B_FOCUSED) != 0;

	if (isEnabled || isFocused) {
		customColor = tint_color(ui_color(B_WINDOW_TAB_COLOR), 1.0);
		rgb_color bevelColor1 = tint_color(customColor, 1.0);
		rgb_color bevelColor2 = tint_color(customColor, 1.0);

		topTint = 1.0;
		bottomTint = 1.0;

		_DrawFrame(view, rect,
			bevelColor1, bevelColor1,
			bevelColor2, bevelColor2,
			borders & B_TOP_BORDER);
	} else {
		rgb_color cornerColor = tint_color(customColor, 1.0);
		rgb_color bevelColorTop = tint_color(customColor, 1.0);
		rgb_color bevelColorLeft = tint_color(customColor, 1.0);
		rgb_color bevelColorRightBottom = tint_color(customColor, 1.0);

		topTint = 1.0;
		bottomTint = 1.0;

		_DrawFrame(view, rect,
			bevelColorLeft, bevelColorTop,
			bevelColorRightBottom, bevelColorRightBottom,
			cornerColor, cornerColor,
			borders);
	}

	// draw surface top
	_FillGradient(view, rect, customColor, topTint, bottomTint);
}


void
FlatControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius, base, background,
		1.0, 1.0, flags, borders);
}


void
FlatControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float radius, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, radius, radius, radius, radius, base, background, 1.0,
		1.0, flags, borders);
}


void
FlatControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius,
	float rightTopRadius, float leftBottomRadius,
	float rightBottomRadius, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, leftTopRadius, rightTopRadius, leftBottomRadius,
		rightBottomRadius, base, background, 1.0, 1.0, flags, borders);
}


void
FlatControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	_DrawMenuFieldBackgroundOutside(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius,
		base, popupIndicator, flags);
}


void
FlatControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	_DrawMenuFieldBackgroundInside(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius, base,
		flags, borders);
}


void
FlatControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float radius, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	_DrawMenuFieldBackgroundOutside(view, rect, updateRect, radius, radius,
		radius, radius, base, popupIndicator, flags);
}


void
FlatControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	_DrawMenuFieldBackgroundOutside(view, rect, updateRect, leftTopRadius,
		rightTopRadius, leftBottomRadius, rightBottomRadius, base,
		popupIndicator, flags);
}


void
FlatControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// surface top color
	rgb_color background = tint_color(base, 1.0);

	// inner bevel colors
	rgb_color bevelColor;

	bevelColor = tint_color(background, 1.0);

	// draw inner bevel
	_DrawFrame(view, rect,
		bevelColor, bevelColor,
		bevelColor, bevelColor,
		borders);

	// draw surface top
	view->SetHighColor(background);
	view->FillRect(rect);
}


void
FlatControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// surface edges
	float topTint;
	float bottomTint;
	rgb_color selectedColor = base;

	if ((flags & B_ACTIVATED) != 0) {
		topTint = 0.94;
		bottomTint = 1.1;
	} else if ((flags & B_DISABLED) != 0) {
		topTint = 1.0;
		bottomTint = 1.0;
	} else {
		topTint = 0.95;
		bottomTint = 1.1;
	}

	rgb_color bevelShadowColor = tint_color(selectedColor, bottomTint);

	// draw surface edges
	_DrawFrame(view, rect,
		bevelShadowColor, bevelShadowColor,
		bevelShadowColor, bevelShadowColor,
		borders);

	// draw surface top
	view->SetLowColor(selectedColor);
	_FillGradient(view, rect, selectedColor, topTint, bottomTint);
}


void
FlatControlLook::DrawScrollBarBorder(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// set clipping constraints to rect
	view->ClipToRect(rect);

	bool isEnabled = (flags & B_DISABLED) == 0;
	bool isFocused = (flags & B_FOCUSED) != 0;

	view->SetHighColor(tint_color(base, 1.2));

	// stroke a line around the entire scrollbar
	// take care of border highlighting, scroll target is focus view
	if (isEnabled && isFocused) {
		rgb_color borderColor = tint_color(base, 1.2);
		rgb_color highlightColor = tint_color(base, 1.2);

		view->BeginLineArray(4);

		view->AddLine(BPoint(rect.left + 1, rect.bottom),
			BPoint(rect.right, rect.bottom), borderColor);
		view->AddLine(BPoint(rect.right, rect.top + 1),
			BPoint(rect.right, rect.bottom - 1), borderColor);

		if (orientation == B_HORIZONTAL) {
			view->AddLine(BPoint(rect.left, rect.top + 1),
				BPoint(rect.left, rect.bottom), borderColor);
		} else {
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.left, rect.bottom), highlightColor);
		}

		if (orientation == B_HORIZONTAL) {
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), highlightColor);
		} else {
			view->AddLine(BPoint(rect.left + 1, rect.top),
				BPoint(rect.right, rect.top), borderColor);
		}

		view->EndLineArray();
	} else
		view->StrokeRect(rect);

	view->PopState();
}


void
FlatControlLook::DrawScrollBarButton(BView* view, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	int32 direction, orientation orientation, bool down)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	rgb_color arrowColor;

	bool isEnabled = (flags & B_DISABLED) == 0;

	if (isEnabled) {
		arrowColor = tint_color(ui_color(B_CONTROL_TEXT_COLOR), 0.6);
		// if the base color is too dark, then lets make it lighter
		if (base.IsDark()) {
			arrowColor = tint_color(ui_color(B_CONTROL_TEXT_COLOR), 1.3);;
		}
	} else {
		arrowColor = tint_color(ui_color(B_CONTROL_TEXT_COLOR), 0.4);
		// if the base color is too dark, then lets make it lighter
		if (base.IsDark()) {
			arrowColor = tint_color(ui_color(B_CONTROL_TEXT_COLOR), 1.5);;
		}
	}

	// clip to button
	view->PushState();
	view->ClipToRect(rect);

	flags &= ~B_FLAT;

	DrawScrollBarBackground(view, rect, updateRect, base, flags, orientation);
	rect.InsetBy(1, 1);
	DrawArrowShape(view, rect, updateRect, arrowColor, direction, flags, 1.0f);

	// revert clipping constraints
	view->PopState();
}


void
FlatControlLook::DrawScrollBarBackground(BView* view, BRect& rect1,
	BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


void
FlatControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// set clipping constraints to rect
	view->ClipToRect(rect);

	bool isEnabled = (flags & B_DISABLED) == 0;

	// fill background, we'll draw arrows and thumb on top
	view->SetDrawingMode(B_OP_COPY);

	float gradient1Tint = 1.08;
	float gradient2Tint = 0.95;

	if (orientation == B_HORIZONTAL) {
		// dark vertical line on left edge
		// fill
		if (rect.Width() >= 0) {
			_FillGradient(view, rect, base, gradient1Tint, gradient2Tint,
				orientation);
		}
	} else {
		// dark vertical line on top edge
		// fill
		if (rect.Height() >= 0) {
			_FillGradient(view, rect, base, gradient1Tint, gradient2Tint,
				orientation);
		}
	}

	view->PopState();
}


void
FlatControlLook::DrawScrollBarThumb(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	orientation orientation, uint32 knobStyle)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	view->PushState();

	// set clipping constraints to rect
	view->ClipToRect(rect);

	// flags
	bool isEnabled = (flags & B_DISABLED) == 0;

	// colors
	rgb_color thumbColor = tint_color(ui_color(B_SCROLL_BAR_THUMB_COLOR), 1.09);
	rgb_color base_panel = ui_color(B_PANEL_BACKGROUND_COLOR);

	rgb_color light, dark, dark1, dark2;
	light = tint_color(base_panel, B_DARKEN_1_TINT);
	dark = tint_color(base_panel, B_DARKEN_1_TINT);
	dark1 = tint_color(base_panel, B_DARKEN_1_TINT);
	dark2 = tint_color(base_panel, B_DARKEN_1_TINT);

	// draw thumb over background
	view->SetDrawingMode(B_OP_OVER);
	view->SetHighColor(dark1);

	// draw scroll thumb
	if (isEnabled) {
		// fill the clickable surface of the thumb
		// set clipping constraints to updateRect
		BRegion clipping(updateRect);
		DrawScrollBarBackground(view, rect, updateRect, base_panel, flags, orientation);
		if (orientation == B_HORIZONTAL)
			rect.InsetBy(0, 2);
		else
			rect.InsetBy(2, 0);
		view->SetHighColor(base_panel);
		view->FillRect(rect);

		_DrawNonFlatButtonBackground(view, rect, updateRect, clipping, kRadius + 1, kRadius + 1,
			kRadius + 1, kRadius + 1, thumbColor, false, flags, B_ALL_BORDERS, orientation);
	} else {
		DrawScrollBarBackground(view, rect, updateRect, base_panel, flags, orientation);
	}

	knobStyle = B_KNOB_LINES; // Hard set of the knobstyle

	// draw knob style
	if (knobStyle != B_KNOB_NONE && isEnabled) {
		rgb_color knobLight = isEnabled
			? tint_color(thumbColor, 0.85)
			: tint_color(base_panel, 1.05);
		rgb_color knobDark = isEnabled
			? tint_color(thumbColor, 1.35)
			: tint_color(base_panel, 1.05);

		if (knobStyle == B_KNOB_DOTS) {
			// draw dots on the scroll bar thumb
			float hcenter = rect.left + rect.Width() / 2;
			float vmiddle = rect.top + rect.Height() / 2;
			BRect knob(hcenter, vmiddle, hcenter, vmiddle);

			if (orientation == B_HORIZONTAL) {
				view->SetHighColor(knobDark);
				view->FillRect(knob);
				view->SetHighColor(knobLight);
				view->FillRect(knob.OffsetByCopy(1, 1));

				float spacer = rect.Height();

				if (rect.left + 3 < hcenter - spacer) {
					view->SetHighColor(knobDark);
					view->FillRect(knob.OffsetByCopy(-spacer, 0));
					view->SetHighColor(knobLight);
					view->FillRect(knob.OffsetByCopy(-spacer + 1, 1));
				}

				if (rect.right - 3 > hcenter + spacer) {
					view->SetHighColor(knobDark);
					view->FillRect(knob.OffsetByCopy(spacer, 0));
					view->SetHighColor(knobLight);
					view->FillRect(knob.OffsetByCopy(spacer + 1, 1));
				}
			} else {
				// B_VERTICAL
				view->SetHighColor(knobDark);
				view->FillRect(knob);
				view->SetHighColor(knobLight);
				view->FillRect(knob.OffsetByCopy(1, 1));

				float spacer = rect.Width();

				if (rect.top + 3 < vmiddle - spacer) {
					view->SetHighColor(knobDark);
					view->FillRect(knob.OffsetByCopy(0, -spacer));
					view->SetHighColor(knobLight);
					view->FillRect(knob.OffsetByCopy(1, -spacer + 1));
				}

				if (rect.bottom - 3 > vmiddle + spacer) {
					view->SetHighColor(knobDark);
					view->FillRect(knob.OffsetByCopy(0, spacer));
					view->SetHighColor(knobLight);
					view->FillRect(knob.OffsetByCopy(1, spacer + 1));
				}
			}
		} else if (knobStyle == B_KNOB_LINES && isEnabled) {
			// draw lines on the scroll bar thumb
			if (orientation == B_HORIZONTAL) {
				float middle = rect.Width() / 2;

				view->BeginLineArray(6);
				view->AddLine(
					BPoint(rect.left + middle - 3, rect.top + 2),
					BPoint(rect.left + middle - 3, rect.bottom - 2),
					knobDark);
				view->AddLine(
					BPoint(rect.left + middle, rect.top + 2),
					BPoint(rect.left + middle, rect.bottom - 2),
					knobDark);
				view->AddLine(
					BPoint(rect.left + middle + 3, rect.top + 2),
					BPoint(rect.left + middle + 3, rect.bottom - 2),
					knobDark);
				view->AddLine(
					BPoint(rect.left + middle - 2, rect.top + 2),
					BPoint(rect.left + middle - 2, rect.bottom - 2),
					knobLight);
				view->AddLine(
					BPoint(rect.left + middle + 1, rect.top + 2),
					BPoint(rect.left + middle + 1, rect.bottom - 2),
					knobLight);
				view->AddLine(
					BPoint(rect.left + middle + 4, rect.top + 2),
					BPoint(rect.left + middle + 4, rect.bottom - 2),
					knobLight);
				view->EndLineArray();
			} else {
				// B_VERTICAL
				float middle = rect.Height() / 2;

				view->BeginLineArray(6);
				view->AddLine(
					BPoint(rect.left + 2, rect.top + middle - 3),
					BPoint(rect.right - 2, rect.top + middle - 3),
					knobDark);
				view->AddLine(
					BPoint(rect.left + 2, rect.top + middle),
					BPoint(rect.right - 2, rect.top + middle),
					knobDark);
				view->AddLine(
					BPoint(rect.left + 2, rect.top + middle + 3),
					BPoint(rect.right - 2, rect.top + middle + 3),
					knobDark);
				view->AddLine(
					BPoint(rect.left + 2, rect.top + middle - 2),
					BPoint(rect.right - 2, rect.top + middle - 2),
					knobLight);
				view->AddLine(
					BPoint(rect.left + 2, rect.top + middle + 1),
					BPoint(rect.right - 2, rect.top + middle + 1),
					knobLight);
				view->AddLine(
					BPoint(rect.left + 2, rect.top + middle + 4),
					BPoint(rect.right - 2, rect.top + middle + 4),
					knobLight);
				view->EndLineArray();
			}
		}
	}

	view->PopState();
}


void
FlatControlLook::DrawScrollViewFrame(BView* view, BRect& rect,
	const BRect& updateRect, BRect verticalScrollBarFrame,
	BRect horizontalScrollBarFrame, const rgb_color& base,
	border_style borderStyle, uint32 flags, uint32 _borders)
{
	// calculate scroll corner rect before messing with the "rect"
	BRect scrollCornerFillRect(rect.right, rect.bottom,
		rect.right, rect.bottom);

	if (horizontalScrollBarFrame.IsValid())
		scrollCornerFillRect.left = horizontalScrollBarFrame.right + 1;

	if (verticalScrollBarFrame.IsValid())
		scrollCornerFillRect.top = verticalScrollBarFrame.bottom + 1;

	if (borderStyle == B_NO_BORDER) {
		if (scrollCornerFillRect.IsValid()) {
			view->SetHighColor(base);
			view->FillRect(scrollCornerFillRect);
		}
		return;
	}

	bool excludeScrollCorner = borderStyle == B_FANCY_BORDER
		&& horizontalScrollBarFrame.IsValid()
		&& verticalScrollBarFrame.IsValid();

	uint32 borders = _borders;
	if (excludeScrollCorner) {
		rect.bottom = horizontalScrollBarFrame.top;
		rect.right = verticalScrollBarFrame.left;
		borders &= ~(B_RIGHT_BORDER | B_BOTTOM_BORDER);
	}

	rgb_color scrollbarFrameColor = tint_color(base, 1.2);

	if (borderStyle == B_FANCY_BORDER)
		_DrawOuterResessedFrame(view, rect, base, 1.0, 1.0, flags, borders);

	if ((flags & B_FOCUSED) != 0) {
		_DrawFrame(view, rect, scrollbarFrameColor, scrollbarFrameColor,
			scrollbarFrameColor, scrollbarFrameColor, borders);
	} else {
		_DrawFrame(view, rect, scrollbarFrameColor, scrollbarFrameColor,
			scrollbarFrameColor, scrollbarFrameColor, borders);
	}

	if (excludeScrollCorner) {
		horizontalScrollBarFrame.InsetBy(-1, -1);

		// do not overdraw the top edge
		horizontalScrollBarFrame.top += 2;
		borders = _borders;
		borders &= ~B_TOP_BORDER;
		_DrawOuterResessedFrame(view, horizontalScrollBarFrame, base,
			1.0, 1.0, flags, borders);
		_DrawFrame(view, horizontalScrollBarFrame, scrollbarFrameColor,
			scrollbarFrameColor, scrollbarFrameColor, scrollbarFrameColor,
			borders);

		verticalScrollBarFrame.InsetBy(-1, -1);

		// do not overdraw the left edge
		verticalScrollBarFrame.left += 2;
		borders = _borders;
		borders &= ~B_LEFT_BORDER;
		_DrawOuterResessedFrame(view, verticalScrollBarFrame, base,
			1.0, 1.0, flags, borders);
		_DrawFrame(view, verticalScrollBarFrame, scrollbarFrameColor,
			scrollbarFrameColor, scrollbarFrameColor, scrollbarFrameColor,
			borders);

		// exclude recessed frame
		scrollCornerFillRect.top++;
		scrollCornerFillRect.left++;
	}

	if (scrollCornerFillRect.IsValid()) {
		view->SetHighColor(base);
		view->FillRect(scrollCornerFillRect);
	}
}


rgb_color
FlatControlLook::SliderBarColor(const rgb_color& base)
{
	return base.IsLight() ? tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 1.05) :
		tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 0.95);
}


void
FlatControlLook::DrawSliderBar(BView* view, BRect rect, const BRect& updateRect,
	const rgb_color& base, rgb_color leftFillColor, rgb_color rightFillColor,
	float sliderScale, uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

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
	view->PushState();
	view->ClipToRect(rightBarSide);

	DrawSliderBar(view, rect, updateRect, base, leftFillColor, flags, orientation);

	view->PopState();

	view->PushState();
	view->ClipToRect(leftBarSide);

	DrawSliderBar(view, rect, updateRect, base, rightFillColor, flags,
		orientation);

	// restore the clipping constraints of the view
	view->PopState();
}


void
FlatControlLook::DrawSliderBar(BView* view, BRect rect, const BRect& updateRect,
	const rgb_color& base, rgb_color fillColor, uint32 flags,
	orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// separate the rect into corners
	BRect leftCorner(rect);
	BRect rightCorner(rect);
	BRect barRect(rect);

	if (orientation == B_HORIZONTAL) {
		leftCorner.right = leftCorner.left + leftCorner.Height();
		rightCorner.left = rightCorner.right - rightCorner.Height();
		barRect.left += ceilf(barRect.Height() / 2);
		barRect.right -= ceilf(barRect.Height() / 2);
	} else {
		leftCorner.bottom = leftCorner.top + leftCorner.Width();
		rightCorner.top = rightCorner.bottom - rightCorner.Width();
		barRect.top += ceilf(barRect.Width() / 2);
		barRect.bottom -= ceilf(barRect.Width() / 2);
	}

	// fill the background for the corners, exclude the middle bar for now
	view->PushState();
	view->ClipToRect(rect);
	view->ClipToInverseRect(barRect);

	if ((flags & B_BLEND_FRAME) == 0) {
		view->SetHighColor(base);
		view->FillRect(rect);
	}

	// figure out the tints to be used
	float edgeLightTint;
	float edgeShadowTint;
	float frameLightTint;
	float frameShadowTint;
	float fillLightTint;
	float fillShadowTint;
	uint8 edgeLightAlpha;
	uint8 edgeShadowAlpha;
	uint8 frameLightAlpha;
	uint8 frameShadowAlpha;

	if ((flags & B_DISABLED) != 0) {
		edgeLightTint = 1.0;
		edgeShadowTint = 1.0;
		frameLightTint = 1.05;
		frameShadowTint = 1.05;
		fillLightTint = 0.8;
		fillShadowTint = 0.8;
		edgeLightAlpha = 12;
		edgeShadowAlpha = 12;
		frameLightAlpha = 40;
		frameShadowAlpha = 45;

		fillColor.red = uint8(fillColor.red * 0.4 + base.red * 0.6);
		fillColor.green = uint8(fillColor.green * 0.4 + base.green * 0.6);
		fillColor.blue = uint8(fillColor.blue * 0.4 + base.blue * 0.6);
	} else {
		edgeLightTint = 1.0;
		edgeShadowTint = 1.0;
		frameLightTint = 1.20;
		frameShadowTint = 1.20;
		fillLightTint = 0.9;
		fillShadowTint = 0.9;
		edgeLightAlpha = 15;
		edgeShadowAlpha = 15;
		frameLightAlpha = 102;
		frameShadowAlpha = 117;
	}

	rgb_color edgeLightColor;
	rgb_color edgeShadowColor;
	rgb_color frameLightColor;
	rgb_color frameShadowColor;
	rgb_color fillLightColor = tint_color(fillColor, fillLightTint);
	rgb_color fillShadowColor = tint_color(fillColor, fillShadowTint);

	drawing_mode oldMode = view->DrawingMode();

	if ((flags & B_BLEND_FRAME) != 0) {
		edgeLightColor = (rgb_color){ 255, 255, 255, edgeLightAlpha };
		edgeShadowColor = (rgb_color){ 0, 0, 0, edgeShadowAlpha };
		frameLightColor = (rgb_color){ 0, 0, 0, frameLightAlpha };
		frameShadowColor = (rgb_color){ 0, 0, 0, frameShadowAlpha };

		view->SetDrawingMode(B_OP_ALPHA);
	} else {
		edgeLightColor = tint_color(base, edgeLightTint);
		edgeShadowColor = tint_color(base, edgeShadowTint);
		frameLightColor = tint_color(fillColor, frameLightTint);
		frameShadowColor = tint_color(fillColor, frameShadowTint);
	}

	if (orientation == B_HORIZONTAL) {
		_DrawRoundBarCorner(view, leftCorner, updateRect, edgeLightColor,
			edgeShadowColor, frameLightColor, frameShadowColor, fillLightColor,
			fillShadowColor, 1.0, 1.0, 0.0, -1.0, orientation);

		_DrawRoundBarCorner(view, rightCorner, updateRect, edgeLightColor,
			edgeShadowColor, frameLightColor, frameShadowColor, fillLightColor,
			fillShadowColor, 0.0, 1.0, -1.0, -1.0, orientation);
	} else {
		_DrawRoundBarCorner(view, leftCorner, updateRect, edgeLightColor,
			edgeShadowColor, frameLightColor, frameShadowColor, fillLightColor,
			fillShadowColor, 1.0, 1.0, -1.0, 0.0, orientation);

		_DrawRoundBarCorner(view, rightCorner, updateRect, edgeLightColor,
			edgeShadowColor, frameLightColor, frameShadowColor, fillLightColor,
			fillShadowColor, 1.0, 0.0, -1.0, -1.0, orientation);
	}

	view->PopState();

	view->BeginLineArray(4);
	if (orientation == B_HORIZONTAL) {
		view->AddLine(barRect.LeftTop(), barRect.RightTop(),
			edgeShadowColor);
		view->AddLine(barRect.LeftBottom(), barRect.RightBottom(),
			edgeLightColor);
		barRect.InsetBy(0, 1);
		view->AddLine(barRect.LeftTop(), barRect.RightTop(),
			frameShadowColor);
		view->AddLine(barRect.LeftBottom(), barRect.RightBottom(),
			frameLightColor);
		barRect.InsetBy(0, 1);
	} else {
		view->AddLine(barRect.LeftTop(), barRect.LeftBottom(),
			edgeShadowColor);
		view->AddLine(barRect.RightTop(), barRect.RightBottom(),
			edgeLightColor);
		barRect.InsetBy(1, 0);
		view->AddLine(barRect.LeftTop(), barRect.LeftBottom(),
			frameShadowColor);
		view->AddLine(barRect.RightTop(), barRect.RightBottom(),
			frameLightColor);
		barRect.InsetBy(1, 0);
	}
	view->EndLineArray();

	view->SetDrawingMode(oldMode);

	_FillGradient(view, barRect, fillColor, fillShadowTint, fillLightTint,
		orientation);
}


void
FlatControlLook::DrawSliderThumb(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, orientation orientation)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	rgb_color thumbColor = tint_color(ui_color(B_SCROLL_BAR_THUMB_COLOR), 1.0);

	// figure out frame color
	rgb_color frameLightColor;
	rgb_color frameShadowColor;
	rgb_color shadowColor;

	if (base.IsLight())
		shadowColor = tint_color(ui_color(B_CONTROL_TEXT_COLOR), 0.5);
	else
		shadowColor = tint_color(ui_color(B_CONTROL_TEXT_COLOR), 1.55);

	if ((flags & B_FOCUSED) != 0) {
		// focused
		frameLightColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
		frameShadowColor = frameLightColor;
	} else {
		// figure out the tints to be used
		float frameLightTint;
		float frameShadowTint;

		if ((flags & B_DISABLED) != 0) {
			frameLightTint = 1.30;
			frameShadowTint = 1.35;
			shadowColor.alpha = 30;
		} else {
			frameLightTint = 1.6;
			frameShadowTint = 1.65;
		}

		frameLightColor = tint_color(base, frameLightTint);
		frameShadowColor = tint_color(base, frameShadowTint);
	}

	BRect originalRect(rect);
	rect.right--;
	rect.bottom--;

	_DrawFrame(view, rect, shadowColor, shadowColor, shadowColor, shadowColor);

	flags &= ~B_ACTIVATED;
	flags &= ~B_FLAT;
	DrawScrollBarBackground(view, rect, updateRect, base, flags, orientation);

	// thumb edge
	if (orientation == B_HORIZONTAL) {
		rect.InsetBy(0, floorf(rect.Height() / 4));
		rect.left = floorf((rect.left + rect.right) / 2);
		rect.right = rect.left;
		shadowColor = tint_color(thumbColor, 1.5);
		view->SetHighColor(shadowColor);
		view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
		rgb_color lightColor = tint_color(thumbColor, 1.0);
		view->SetHighColor(lightColor);
		view->StrokeLine(rect.RightTop(), rect.RightBottom());
	} else {
		rect.InsetBy(floorf(rect.Width() / 4), 0);
		rect.top = floorf((rect.top + rect.bottom) / 2);
		rect.bottom = rect.top + 1;
		shadowColor = tint_color(thumbColor, 1.5);
		view->SetHighColor(shadowColor);
		view->StrokeLine(rect.LeftTop(), rect.RightTop());
		rgb_color lightColor = tint_color(thumbColor, 1.0);
		view->SetHighColor(lightColor);
		view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
	}

	view->SetDrawingMode(B_OP_COPY);
}


void
FlatControlLook::DrawActiveTab(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, uint32 side, int32, int32, int32, int32)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// Snap the rectangle to pixels to avoid rounding errors.
	rect.left = floorf(rect.left);
	rect.right = floorf(rect.right);
	rect.top = floorf(rect.top);
	rect.bottom = floorf(rect.bottom);

	// save the clipping constraints of the view
	view->PushState();

	// set clipping constraints to rect
	view->ClipToRect(rect);

	rgb_color edgeShadowColor;
	rgb_color edgeLightColor;
	rgb_color frameShadowColor;
	rgb_color frameLightColor;
	rgb_color bevelShadowColor;
	rgb_color bevelLightColor;
	BGradientLinear fillGradient;
	fillGradient.SetStart(rect.LeftTop() + BPoint(3, 3));
	fillGradient.SetEnd(rect.LeftBottom() + BPoint(3, -3));

	if ((flags & B_DISABLED) != 0) {
		edgeLightColor = base;
		edgeShadowColor = base;
		frameLightColor = tint_color(base, 1.0);
		frameShadowColor = tint_color(base, 1.30);
		bevelLightColor = tint_color(base, 0.8);
		bevelShadowColor = tint_color(base, 1.07);
		fillGradient.AddColor(tint_color(base, 0.85), 0);
		fillGradient.AddColor(base, 255);
	} else {
		edgeLightColor = tint_color(base, 0.95);
		edgeShadowColor = tint_color(base, 1.03);
		frameLightColor = tint_color(base, 1.30);
		frameShadowColor = tint_color(base, 1.30);
		bevelLightColor = tint_color(base, 0.9);
		bevelShadowColor = tint_color(base, 1.07);
		fillGradient.AddColor(tint_color(base, 0.95), 0);
		fillGradient.AddColor(tint_color(base, 1.0), 255);
	}

	static const float kRoundCornerRadius = 2.0f;

	// left top corner dimensions
	BRect leftTopCorner(rect);
	leftTopCorner.right = floorf(leftTopCorner.left + kRoundCornerRadius);
	leftTopCorner.bottom = floorf(rect.top + kRoundCornerRadius);

	// right top corner dimensions
	BRect rightTopCorner(rect);
	rightTopCorner.left = floorf(rightTopCorner.right - kRoundCornerRadius);
	rightTopCorner.bottom = floorf(rect.top + kRoundCornerRadius);

	// left bottom corner dimensions
	BRect leftBottomCorner(rect);
	leftBottomCorner.right = floorf(leftBottomCorner.left + kRoundCornerRadius);
	leftBottomCorner.top = floorf(rect.bottom - kRoundCornerRadius);

	// right bottom corner dimensions
	BRect rightBottomCorner(rect);
	rightBottomCorner.left = floorf(rightBottomCorner.right
		- kRoundCornerRadius);
	rightBottomCorner.top = floorf(rect.bottom - kRoundCornerRadius);

	BRect roundCorner[2];

	switch (side) {
		case B_TOP_BORDER:
			roundCorner[0] = leftTopCorner;
			roundCorner[1] = rightTopCorner;

			// draw the left top corner
			_DrawRoundCornerLeftTop(view, leftTopCorner, updateRect, base,
				edgeShadowColor, frameLightColor, bevelLightColor,
				fillGradient);
			// draw the right top corner
			_DrawRoundCornerRightTop(view, rightTopCorner, updateRect, base,
				edgeShadowColor, edgeLightColor, frameLightColor,
				frameShadowColor, bevelLightColor, bevelShadowColor,
				fillGradient);
			break;
		case B_BOTTOM_BORDER:
			roundCorner[0] = leftBottomCorner;
			roundCorner[1] = rightBottomCorner;

			// draw the left bottom corner
			_DrawRoundCornerLeftBottom(view, leftBottomCorner, updateRect, base,
				edgeShadowColor, edgeLightColor, frameLightColor,
				frameShadowColor, bevelLightColor, bevelShadowColor,
				fillGradient);
			// draw the right bottom corner
			_DrawRoundCornerRightBottom(view, rightBottomCorner, updateRect,
				base, edgeLightColor, frameShadowColor, bevelShadowColor,
				fillGradient);
			break;
		case B_LEFT_BORDER:
			roundCorner[0] = leftTopCorner;
			roundCorner[1] = leftBottomCorner;

			// draw the left top corner
			_DrawRoundCornerLeftTop(view, leftTopCorner, updateRect, base,
				edgeShadowColor, frameLightColor, bevelLightColor,
				fillGradient);
			// draw the left bottom corner
			_DrawRoundCornerLeftBottom(view, leftBottomCorner, updateRect, base,
				edgeShadowColor, edgeLightColor, frameLightColor,
				frameShadowColor, bevelLightColor, bevelShadowColor,
				fillGradient);
			break;
		case B_RIGHT_BORDER:
			roundCorner[0] = rightTopCorner;
			roundCorner[1] = rightBottomCorner;

			// draw the right top corner
			_DrawRoundCornerRightTop(view, rightTopCorner, updateRect, base,
				edgeShadowColor, edgeLightColor, frameLightColor,
				frameShadowColor, bevelLightColor, bevelShadowColor,
				fillGradient);
			// draw the right bottom corner
			_DrawRoundCornerRightBottom(view, rightBottomCorner, updateRect,
				base, edgeLightColor, frameShadowColor, bevelShadowColor,
				fillGradient);
			break;
	}

	// clip out the corners
	view->ClipToInverseRect(roundCorner[0]);
	view->ClipToInverseRect(roundCorner[1]);

	uint32 bordersToDraw = 0;
	switch (side) {
		case B_TOP_BORDER:
			bordersToDraw = (B_LEFT_BORDER | B_TOP_BORDER | B_RIGHT_BORDER);
			break;
		case B_BOTTOM_BORDER:
			bordersToDraw = (B_LEFT_BORDER | B_BOTTOM_BORDER | B_RIGHT_BORDER);
			break;
		case B_LEFT_BORDER:
			bordersToDraw = (B_LEFT_BORDER | B_BOTTOM_BORDER | B_TOP_BORDER);
			break;
		case B_RIGHT_BORDER:
			bordersToDraw = (B_RIGHT_BORDER | B_BOTTOM_BORDER | B_TOP_BORDER);
			break;
	}

	// draw the rest of frame and fill
	_DrawFrame(view, rect, edgeShadowColor, edgeShadowColor, edgeLightColor,
		edgeLightColor, borders);
	if (side == B_TOP_BORDER || side == B_BOTTOM_BORDER) {
		if ((borders & B_LEFT_BORDER) == 0)
			rect.left++;
		if ((borders & B_RIGHT_BORDER) == 0)
			rect.right--;
	} else if (side == B_LEFT_BORDER || side == B_RIGHT_BORDER) {
		if ((borders & B_TOP_BORDER) == 0)
			rect.top++;
		if ((borders & B_BOTTOM_BORDER) == 0)
			rect.bottom--;
	}

	_DrawFrame(view, rect, frameLightColor, frameLightColor, frameShadowColor,
		frameShadowColor, bordersToDraw);

	_DrawFrame(view, rect, bevelLightColor, bevelLightColor, bevelShadowColor,
		bevelShadowColor);

	view->FillRect(rect, fillGradient);

	// restore the clipping constraints of the view
	view->PopState();
}


void
FlatControlLook::DrawSplitter(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, orientation orientation, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	rgb_color background;
	if ((flags & (B_CLICKED | B_ACTIVATED)) != 0)
		background = tint_color(base, B_DARKEN_1_TINT);
	else
		background = base;

	rgb_color light = tint_color(background, 1.9);
	rgb_color shadow = tint_color(background, 1.9);

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
			view->StrokeLine(dot, dot, B_SOLID_LOW);
			view->SetHighColor(col2);
			dot.x++;
			view->StrokeLine(dot, dot, B_SOLID_LOW);
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
			view->StrokeLine(dot, dot, B_SOLID_LOW);
			view->SetHighColor(col2);
			dot.y++;
			view->StrokeLine(dot, dot, B_SOLID_LOW);
			dot.y -= 1.0;
			// next pixel
			num++;
			dot.x++;
		}
	}
}


// #pragma mark -


void
FlatControlLook::DrawBorder(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, border_style borderStyle, uint32 flags,
	uint32 borders)
{
	if (borderStyle == B_NO_BORDER)
		return;

	rgb_color scrollbarFrameColor = tint_color(base, 1.0);

	_DrawFrame(view, rect, scrollbarFrameColor, scrollbarFrameColor,
		scrollbarFrameColor, scrollbarFrameColor, borders);
}


void
FlatControlLook::DrawRaisedBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	_DrawFrame(view, rect, base, base, base, base, borders);
}


void
FlatControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	rgb_color dark1BorderColor;
	rgb_color navigationColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
	rgb_color invalidColor = ui_color(B_FAILURE_COLOR);
	rgb_color documentBackground = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	rgb_color customColor2 = tint_color(documentBackground, 1.0);
	dark1BorderColor = tint_color(customColor2, 0.5);

	if ((flags & B_DISABLED) == 0 && (flags & B_FOCUSED) != 0) {
		if (base.IsDark())
			documentBackground = tint_color(documentBackground, 0.9);
		else
			documentBackground = tint_color(documentBackground, 1.5);
	}

	if ((flags & B_DISABLED) == 0 && (flags & B_INVALID) != 0)
		documentBackground = tint_color(invalidColor, 0.5);

	if ((flags & B_BLEND_FRAME) != 0) {
		drawing_mode oldMode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);

		_DrawButtonFrame(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius,
			documentBackground, base, false, false, flags, borders);

		view->SetDrawingMode(oldMode);
	} else {

		_DrawButtonFrame(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius,
			documentBackground, base, false, false, flags, borders);
	}
}


void
FlatControlLook::DrawGroupFrame(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 borders)
{
	rgb_color frameColor = tint_color(base, 1.1);

	if (base.IsDark())
		frameColor = tint_color(base, 0.95);

	// Draws only one flat frame:
	_DrawFrame(view, rect, frameColor, frameColor, frameColor, frameColor, borders);
}


void
FlatControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, kRadius, kRadius, kRadius, kRadius, base, true,
		flags, borders, orientation);
}


void
FlatControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, float radius, const rgb_color& base, uint32 flags,
	uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, radius, radius, radius,
		radius, base, true, flags, borders, orientation);
}


void
FlatControlLook::DrawButtonWithPopUpBackground(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	uint32 flags, uint32 borders, orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, leftTopRadius,
		rightTopRadius, leftBottomRadius, rightBottomRadius, base, true, flags,
		borders, orientation);
}


// #pragma mark -


void
FlatControlLook::_DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	const rgb_color& background, float contrast, float brightness,
	uint32 flags, uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	rgb_color customColor = background; // custom color for borders
	rgb_color customColor2 = tint_color(background, 1.3);

	if (base.IsDark())
		customColor2 = tint_color(ui_color(B_CONTROL_TEXT_COLOR), 1.5);


	// save the clipping constraints of the view
	view->PushState();

	// set clipping constraints to rect
	view->ClipToRect(rect);

	// If the button is flat and neither activated nor otherwise highlighted
	// (mouse hovering or focussed), draw it flat.
	if ((flags & B_FLAT) != 0 && (flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) == 0
		&& ((flags & (B_HOVER | B_FOCUSED)) == 0 || (flags & B_DISABLED) != 0)) {
		_DrawFrame(view, rect, background, background, background, background, borders);
		_DrawFrame(view, rect, background, background, background, background, borders);
		view->PopState();
		return;
	}

	// outer edge colors
	rgb_color edgeLightColor;
	rgb_color edgeShadowColor;

	// default button frame color
	rgb_color defaultIndicatorColor = ui_color(B_WINDOW_TAB_COLOR);
	rgb_color cornerBgColor = background;

	if ((flags & B_DISABLED) != 0) {
		defaultIndicatorColor = disable_color(defaultIndicatorColor, background);
	}

	drawing_mode oldMode = view->DrawingMode();

	if ((flags & B_DEFAULT_BUTTON) != 0) {
		cornerBgColor = background;
		edgeLightColor = background;
		edgeShadowColor = background;

		// Draw default button indicator
		// Allow a 1-pixel border of the background to come through.
		rect.InsetBy(1, 1);

		view->SetHighColor(defaultIndicatorColor);
		view->StrokeRoundRect(rect, leftTopRadius, leftTopRadius);
		rect.InsetBy(1, 1);

		view->StrokeRoundRect(rect, leftTopRadius, leftTopRadius);
		rect.InsetBy(1, 1);
	} else {
		cornerBgColor = background;
		if ((flags & B_BLEND_FRAME) != 0) {
			// set the background color to transparent for the case
			// that we are on the desktop
			cornerBgColor.alpha = 0;
			view->SetDrawingMode(B_OP_ALPHA);
		}
	}

	// frame colors
	rgb_color frameLightColor = customColor2; // _FrameLightColor(base, flags);
	rgb_color frameShadowColor = customColor2; // _FrameShadowColor(base, flags);
	edgeLightColor = background;
	edgeShadowColor = background;

	// rounded corners

	if ((borders & B_LEFT_BORDER) != 0 && (borders & B_TOP_BORDER) != 0
		&& leftTopRadius > 0) {
		// draw left top rounded corner
		BRect leftTopCorner(floorf(rect.left), floorf(rect.top),
			floorf(rect.left + leftTopRadius),
			floorf(rect.top + leftTopRadius));
		BRect cornerRect(leftTopCorner);
		_DrawRoundCornerFrameLeftTop(view, leftTopCorner, updateRect,
			cornerBgColor, edgeShadowColor, frameLightColor);
		view->ClipToInverseRect(cornerRect);
	}

	if ((borders & B_TOP_BORDER) != 0 && (borders & B_RIGHT_BORDER) != 0
		&& rightTopRadius > 0) {
		// draw right top rounded corner
		BRect rightTopCorner(floorf(rect.right - rightTopRadius),
			floorf(rect.top), floorf(rect.right),
			floorf(rect.top + rightTopRadius));
		BRect cornerRect(rightTopCorner);
		_DrawRoundCornerFrameRightTop(view, rightTopCorner, updateRect,
			cornerBgColor, edgeShadowColor, edgeLightColor,
			frameLightColor, frameShadowColor);
		view->ClipToInverseRect(cornerRect);
	}

	if ((borders & B_LEFT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& leftBottomRadius > 0) {
		// draw left bottom rounded corner
		BRect leftBottomCorner(floorf(rect.left),
			floorf(rect.bottom - leftBottomRadius),
			floorf(rect.left + leftBottomRadius), floorf(rect.bottom));
		BRect cornerRect(leftBottomCorner);
		_DrawRoundCornerFrameLeftBottom(view, leftBottomCorner, updateRect,
			cornerBgColor, edgeShadowColor, edgeLightColor,
			frameLightColor, frameShadowColor);
		view->ClipToInverseRect(cornerRect);
	}

	if ((borders & B_RIGHT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& rightBottomRadius > 0) {
		// draw right bottom rounded corner
		BRect rightBottomCorner(floorf(rect.right - rightBottomRadius),
			floorf(rect.bottom - rightBottomRadius), floorf(rect.right),
			floorf(rect.bottom));
		BRect cornerRect(rightBottomCorner);
		_DrawRoundCornerFrameRightBottom(view, rightBottomCorner,
			updateRect, cornerBgColor, edgeLightColor, frameShadowColor);
		view->ClipToInverseRect(cornerRect);
	}

	// draw outer edge
	if ((flags & B_DEFAULT_BUTTON) != 0) {
		_DrawOuterResessedFrame(view, rect, background, 0, 0, flags, borders);
	} else {
		if ((flags & B_FOCUSED) != 0)
			_DrawOuterResessedFrame(view, rect, tint_color(background, 1.15), 0, 0);
		else
			_DrawOuterResessedFrame(view, rect, background, 0, 0, flags, borders);
	}

	view->SetDrawingMode(oldMode);

	// draw frame
	if ((flags & B_BLEND_FRAME) != 0) {
		drawing_mode oldDrawingMode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);

		_DrawFrame(view, rect, frameLightColor, frameLightColor, frameShadowColor, frameShadowColor,
			borders);

		view->SetDrawingMode(oldDrawingMode);
	} else {
		_DrawFrame(view, rect, frameLightColor, frameLightColor, frameShadowColor, frameShadowColor,
			borders);
	}

	// restore the clipping constraints of the view
	view->PopState();
}


void
FlatControlLook::_DrawOuterResessedFrame(BView* view, BRect& rect,
	const rgb_color& base, float contrast, float brightness, uint32 flags,
	uint32 borders)
{
	rgb_color edgeLightColor = tint_color(base, 1.04);
	rgb_color edgeShadowColor = tint_color(base, 1.04);

	if ((flags & B_BLEND_FRAME) != 0) {
		// assumes the background has already been painted
		drawing_mode oldDrawingMode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);

		_DrawFrame(view, rect, edgeShadowColor, edgeShadowColor, edgeLightColor, edgeLightColor,
			borders);

		view->SetDrawingMode(oldDrawingMode);
	} else {
		_DrawFrame(view, rect, edgeShadowColor, edgeShadowColor, edgeLightColor, edgeLightColor,
			borders);
	}
}


void
FlatControlLook::_DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	bool popupIndicator, uint32 flags, uint32 borders, orientation orientation)
{
	rgb_color customColor = base;

	if (!ShouldDraw(view, rect, updateRect))
		return;

	// save the clipping constraints of the view
	view->PushState();

	// set clipping constraints to updateRect
	view->ClipToRect(rect);

	// If is a default button, set backcolor to the tab color.
	if ((flags & B_DEFAULT_BUTTON) != 0)
	{
		rgb_color textcolor = ui_color(B_CONTROL_TEXT_COLOR);
		// if the text color is too light, then make it using B_WINDOW_TAB_COLOR
		if (textcolor.red + textcolor.green + textcolor.blue >= 128 * 3)
			customColor = tint_color(ui_color(B_WINDOW_TAB_COLOR), 1.4);
		else
			customColor = ui_color(B_WINDOW_TAB_COLOR);
	}

	// If the button is flat and neither activated nor otherwise highlighted
	// (mouse hovering or focussed), draw it flat.
	if ((flags & B_FLAT) != 0
		&& (flags & (B_ACTIVATED | B_PARTIALLY_ACTIVATED)) == 0
		&& ((flags & (B_HOVER | B_FOCUSED)) == 0
			|| (flags & B_DISABLED) != 0)) {
		_DrawFlatButtonBackground(view, rect, updateRect, customColor, popupIndicator,
			flags, borders, orientation);
	} else {
		BRegion clipping(rect);
		_DrawNonFlatButtonBackground(view, rect, updateRect, clipping,
			leftTopRadius, rightTopRadius, leftBottomRadius, rightBottomRadius,
			customColor, popupIndicator, flags, borders, orientation);
	}

	// restore the clipping constraints of the view
	view->PopState();
}


void
FlatControlLook::_DrawNonFlatButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, BRegion& clipping, float leftTopRadius,
	float rightTopRadius, float leftBottomRadius, float rightBottomRadius,
	const rgb_color& base, bool popupIndicator, uint32 flags, uint32 borders,
	orientation orientation)
{
	// inner bevel colors
	rgb_color bevelLightColor = _BevelLightColor(base, flags);
	rgb_color bevelShadowColor = _BevelShadowColor(base, flags);

	// button background color
	rgb_color buttonBgColor;

	buttonBgColor = tint_color(base, 1.04);

	// surface top gradient
	BGradientLinear fillGradient;
	_MakeButtonGradient(fillGradient, rect, base, flags, orientation);

	// rounded corners

	if ((borders & B_LEFT_BORDER) != 0 && (borders & B_TOP_BORDER) != 0
		&& leftTopRadius > 0) {
		// draw left top rounded corner
		BRect leftTopCorner(floorf(rect.left), floorf(rect.top),
			floorf(rect.left + leftTopRadius - 2.0),
			floorf(rect.top + leftTopRadius - 2.0));
		clipping.Exclude(leftTopCorner);
		BRect cornerRect(leftTopCorner);
		_DrawRoundCornerBackgroundLeftTop(view, leftTopCorner, updateRect,
			bevelLightColor, fillGradient);
		view->ClipToInverseRect(cornerRect);
	}

	if ((borders & B_TOP_BORDER) != 0 && (borders & B_RIGHT_BORDER) != 0
		&& rightTopRadius > 0) {
		// draw right top rounded corner
		BRect rightTopCorner(floorf(rect.right - rightTopRadius + 2.0),
			floorf(rect.top), floorf(rect.right),
			floorf(rect.top + rightTopRadius - 2.0));
		clipping.Exclude(rightTopCorner);
		BRect cornerRect(rightTopCorner);
		_DrawRoundCornerBackgroundRightTop(view, rightTopCorner,
			updateRect, bevelLightColor, bevelShadowColor, fillGradient);
		view->ClipToInverseRect(cornerRect);
	}

	if ((borders & B_LEFT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& leftBottomRadius > 0) {
		// draw left bottom rounded corner
		BRect leftBottomCorner(floorf(rect.left),
			floorf(rect.bottom - leftBottomRadius + 2.0),
			floorf(rect.left + leftBottomRadius - 2.0),
			floorf(rect.bottom));
		clipping.Exclude(leftBottomCorner);
		BRect cornerRect(leftBottomCorner);
		_DrawRoundCornerBackgroundLeftBottom(view, leftBottomCorner,
			updateRect, bevelLightColor, bevelShadowColor, fillGradient);
		view->ClipToInverseRect(cornerRect);
	}

	if ((borders & B_RIGHT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& rightBottomRadius > 0) {
		// draw right bottom rounded corner
		BRect rightBottomCorner(floorf(rect.right - rightBottomRadius + 2.0),
			floorf(rect.bottom - rightBottomRadius + 2.0), floorf(rect.right),
			floorf(rect.bottom));
		clipping.Exclude(rightBottomCorner);
		BRect cornerRect(rightBottomCorner);
		_DrawRoundCornerBackgroundRightBottom(view, rightBottomCorner,
			updateRect, bevelShadowColor, fillGradient);
		view->ClipToInverseRect(cornerRect);
	}

	// draw inner bevel

	if ((flags & B_ACTIVATED) != 0) {
		view->BeginLineArray(4);

		// shadow along left/top borders
		if (borders & B_LEFT_BORDER) {
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.left, rect.bottom), buttonBgColor);
			rect.left++;
		}
		if (borders & B_TOP_BORDER) {
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), buttonBgColor);
			rect.top++;
		}

		// softer shadow along left/top borders
		if (borders & B_LEFT_BORDER) {
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.left, rect.bottom), buttonBgColor);
			rect.left++;
		}
		if (borders & B_TOP_BORDER) {
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), buttonBgColor);
			rect.top++;
		}

		view->EndLineArray();
	} else {
		_DrawFrame(view, rect,
			buttonBgColor, buttonBgColor,
			buttonBgColor, buttonBgColor,
			buttonBgColor, buttonBgColor, borders);
	}

	if (popupIndicator) {
		BRect indicatorRect(rect);
		rect.right -= kButtonPopUpIndicatorWidth;
		indicatorRect.left = rect.right + 3;
			// 2 pixels for the separator

		// Even when depressed we want the pop-up indicator background and
		// separator to cover the area up to the top.
		if ((flags & B_ACTIVATED) != 0)
			indicatorRect.top--;

		// draw the separator
		rgb_color separatorBaseColor = base;
		if ((flags & B_ACTIVATED) != 0)
			separatorBaseColor = tint_color(base, B_DARKEN_1_TINT);

		// rgb_color separatorLightColor = tint_color(base, B_DARKEN_1_TINT);
		rgb_color separatorShadowColor = tint_color(base, B_DARKEN_1_TINT);

		view->BeginLineArray(2);

		view->AddLine(BPoint(indicatorRect.left - 2, indicatorRect.top),
			BPoint(indicatorRect.left - 2, indicatorRect.bottom),
			separatorShadowColor);
		view->AddLine(BPoint(indicatorRect.left - 1, indicatorRect.top),
			BPoint(indicatorRect.left - 1, indicatorRect.bottom),
			separatorShadowColor);

		view->EndLineArray();

		// draw background and pop-up marker
		_DrawMenuFieldBackgroundInside(view, indicatorRect, updateRect, kRadius, rightTopRadius,
			kRadius, rightBottomRadius, base, flags, 0);

		if ((flags & B_ACTIVATED) != 0)
			indicatorRect.top++;

		_DrawPopUpMarker(view, indicatorRect, ui_color(B_MENU_ITEM_TEXT_COLOR), flags);
	}

	// fill in the background
	view->FillRect(rect, fillGradient);
}


void
FlatControlLook::_DrawPopUpMarker(BView* view, const BRect& rect,
	const rgb_color& base, uint32 flags)
{
	BPoint center(roundf((rect.left + rect.right) / 2.0),
		roundf((rect.top + rect.bottom) / 2.0));
	BPoint triangle[3];
	triangle[0] = center + BPoint(-2.5, -0.5);
	triangle[1] = center + BPoint(2.5, -0.5);
	triangle[2] = center + BPoint(0.0, 2.0);

	uint32 viewFlags = view->Flags();
	view->SetFlags(viewFlags | B_SUBPIXEL_PRECISE);

	rgb_color markColor;
	if ((flags & B_DISABLED) != 0)
		markColor = tint_color(base, 1.0);
	else
		markColor = tint_color(base, 1.0);

	view->SetHighColor(markColor);
	view->FillTriangle(triangle[0], triangle[1], triangle[2]);

	view->SetFlags(viewFlags);
}


void
FlatControlLook::_DrawMenuFieldBackgroundOutside(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	rgb_color indicatorColor;

	if (base.IsDark())
		indicatorColor = tint_color(base, 0.95);
	else
		indicatorColor = tint_color(base, 1.05);

	if (popupIndicator) {
		const float indicatorWidth = ComposeSpacing(kButtonPopUpIndicatorWidth);
		const float spacing = (indicatorWidth <= 11.0f) ? 1.0f : roundf(indicatorWidth / 11.0f);

		BRect leftRect(rect);
		leftRect.right -= indicatorWidth - spacing;

		BRect rightRect(rect);
		rightRect.left = rightRect.right - (indicatorWidth - spacing * 2);

		_DrawMenuFieldBackgroundInside(view, leftRect, updateRect, leftTopRadius, 0.0f,
			leftBottomRadius, 0.0f, base, flags, B_LEFT_BORDER | B_TOP_BORDER | B_BOTTOM_BORDER);

		_DrawMenuFieldBackgroundInside(view, rightRect, updateRect, 0.0f, rightTopRadius, 0.0f,
			rightBottomRadius, indicatorColor, flags,
			B_TOP_BORDER | B_RIGHT_BORDER | B_BOTTOM_BORDER);

		_DrawPopUpMarker(view, rightRect, ui_color(B_MENU_ITEM_TEXT_COLOR), flags);

		// draw a line on the left of the popup frame
		rgb_color bevelShadowColor = tint_color(indicatorColor, 1.1);
		if (base.IsDark())
			bevelShadowColor = tint_color(indicatorColor, 0.9);
		view->SetHighColor(bevelShadowColor);

		BPoint leftTopCorner(floorf(rightRect.left - spacing), floorf(rightRect.top - spacing));
		BPoint leftBottomCorner(floorf(rightRect.left - spacing),
			floorf(rightRect.bottom + spacing));

		for (float i = 0; i < spacing; i++)
			view->StrokeLine(leftTopCorner + BPoint(i, 0), leftBottomCorner + BPoint(i, 0));

		rect = leftRect;
	} else {
		_DrawMenuFieldBackgroundInside(view, rect, updateRect, leftTopRadius,
			rightTopRadius, leftBottomRadius, rightBottomRadius, base, flags,
			B_TOP_BORDER | B_RIGHT_BORDER | B_BOTTOM_BORDER);
	}
}


void
FlatControlLook::_DrawMenuFieldBackgroundInside(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	uint32 flags, uint32 borders)
{
	if (!ShouldDraw(view, rect, updateRect))
		return;

	// save the clipping constraints of the view
	view->PushState();

	// set clipping constraints to rect
	view->ClipToRect(rect);

	// frame colors
	rgb_color frameLightColor = _FrameLightColor(base, flags);
	rgb_color frameShadowColor = _FrameShadowColor(base, flags);

	// indicator background color
	rgb_color indicatorBase;
	if ((borders & B_LEFT_BORDER) != 0)
		indicatorBase = base;
	else {
		if ((flags & B_DISABLED) != 0)
			indicatorBase = tint_color(base, 1.05);
		else
			indicatorBase = tint_color(base, 1);
	}

	// bevel colors
	rgb_color cornerColor = tint_color(indicatorBase, 1.0);
	rgb_color bevelColor1 = tint_color(indicatorBase, 1.0);
	rgb_color bevelColor2 = tint_color(indicatorBase, 1.0);
	rgb_color bevelColor3 = tint_color(indicatorBase, 1.0);

	if ((flags & B_DISABLED) != 0) {
		cornerColor = tint_color(indicatorBase, 1.0);
		bevelColor1 = tint_color(indicatorBase, 1.0);
		bevelColor2 = tint_color(indicatorBase, 1.0);
		bevelColor3 = tint_color(indicatorBase, 1.0);
	} else {
		cornerColor = tint_color(indicatorBase, 1.0);
		bevelColor1 = tint_color(indicatorBase, 1.0);
		bevelColor2 = tint_color(indicatorBase, 1.0);
		bevelColor3 = tint_color(indicatorBase, 1.0);
	}

	// surface top gradient
	BGradientLinear fillGradient;
	_MakeButtonGradient(fillGradient, rect, indicatorBase, flags);

	// rounded corners

	if ((borders & B_LEFT_BORDER) != 0 && (borders & B_TOP_BORDER) != 0
		&& leftTopRadius > 0) {
		// draw left top rounded corner
		BRect leftTopCorner(floorf(rect.left), floorf(rect.top),
			floorf(rect.left + leftTopRadius - 2.0),
			floorf(rect.top + leftTopRadius - 2.0));
		BRect cornerRect(leftTopCorner);

		view->PushState();
		view->ClipToRect(cornerRect);

		BRect ellipseRect(leftTopCorner);
		ellipseRect.InsetBy(-0.0, -0.0);
		ellipseRect.right = ellipseRect.left + ellipseRect.Width() * 2;
		ellipseRect.bottom = ellipseRect.top + ellipseRect.Height() * 2;

		// draw the frame (again)
		view->SetHighColor(frameLightColor);
		view->FillEllipse(ellipseRect);

		// draw the bevel and background
		_DrawRoundCornerBackgroundLeftTop(view, leftTopCorner, updateRect,
			bevelColor1, fillGradient);

		view->PopState();
		view->ClipToInverseRect(cornerRect);
	}

	if ((borders & B_TOP_BORDER) != 0 && (borders & B_RIGHT_BORDER) != 0
		&& rightTopRadius > 0) {
		// draw right top rounded corner
		BRect rightTopCorner(floorf(rect.right - rightTopRadius + 2.0),
			floorf(rect.top), floorf(rect.right),
			floorf(rect.top + rightTopRadius - 2.0));
		BRect cornerRect(rightTopCorner);

		view->PushState();
		view->ClipToRect(cornerRect);

		BRect ellipseRect(rightTopCorner);
		ellipseRect.InsetBy(-0.0, -0.0);
		ellipseRect.left = ellipseRect.right - ellipseRect.Width() * 2;
		ellipseRect.bottom = ellipseRect.top + ellipseRect.Height() * 2;

		// draw the frame (again)
		if (frameLightColor == frameShadowColor) {
			view->SetHighColor(frameLightColor);
			view->FillEllipse(ellipseRect);
		} else {
			BGradientLinear gradient;
			gradient.AddColor(frameLightColor, 0);
			gradient.AddColor(frameShadowColor, 255);
			gradient.SetStart(rightTopCorner.LeftTop());
			gradient.SetEnd(rightTopCorner.RightBottom());
			view->FillEllipse(ellipseRect, gradient);
		}

		// draw the bevel and background
		_DrawRoundCornerBackgroundRightTop(view, rightTopCorner, updateRect,
			bevelColor1, bevelColor3, fillGradient);

		view->PopState();
		view->ClipToInverseRect(cornerRect);
	}

	if ((borders & B_LEFT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& leftBottomRadius > 0) {
		// draw left bottom rounded corner
		BRect leftBottomCorner(floorf(rect.left),
			floorf(rect.bottom - leftBottomRadius + 2.0),
			floorf(rect.left + leftBottomRadius - 2.0),
			floorf(rect.bottom));
		BRect cornerRect(leftBottomCorner);

		view->PushState();
		view->ClipToRect(cornerRect);

		BRect ellipseRect(leftBottomCorner);
		ellipseRect.InsetBy(-0.0, -0.0);
		ellipseRect.right = ellipseRect.left + ellipseRect.Width() * 2;
		ellipseRect.top = ellipseRect.bottom - ellipseRect.Height() * 2;

		// draw the frame (again)
		if (frameLightColor == frameShadowColor) {
			view->SetHighColor(frameLightColor);
			view->FillEllipse(ellipseRect);
		} else {
			BGradientLinear gradient;
			gradient.AddColor(frameLightColor, 0);
			gradient.AddColor(frameShadowColor, 255);
			gradient.SetStart(leftBottomCorner.LeftTop());
			gradient.SetEnd(leftBottomCorner.RightBottom());
			view->FillEllipse(ellipseRect, gradient);
		}

		// draw the bevel and background
		_DrawRoundCornerBackgroundLeftBottom(view, leftBottomCorner,
			updateRect, bevelColor2, bevelColor3, fillGradient);

		view->PopState();
		view->ClipToInverseRect(cornerRect);
	}

	if ((borders & B_RIGHT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& rightBottomRadius > 0) {
		// draw right bottom rounded corner
		BRect rightBottomCorner(floorf(rect.right - rightBottomRadius + 2.0),
			floorf(rect.bottom - rightBottomRadius + 2.0), floorf(rect.right),
			floorf(rect.bottom));
		BRect cornerRect(rightBottomCorner);

		view->PushState();
		view->ClipToRect(cornerRect);

		BRect ellipseRect(rightBottomCorner);
		ellipseRect.InsetBy(-0.0, -0.0);
		ellipseRect.left = ellipseRect.right - ellipseRect.Width() * 2;
		ellipseRect.top = ellipseRect.bottom - ellipseRect.Height() * 2;

		// draw the frame (again)
		view->SetHighColor(frameShadowColor);
		view->FillEllipse(ellipseRect);

		// draw the bevel and background
		_DrawRoundCornerBackgroundRightBottom(view, rightBottomCorner,
			updateRect, bevelColor3, fillGradient);

		view->PopState();
		view->ClipToInverseRect(cornerRect);
	}

	// fill in the background
	view->FillRect(rect, fillGradient);

	// restore the clipping constraints of the view
	view->PopState();
}

rgb_color
FlatControlLook::_EdgeLightColor(const rgb_color& base, float contrast,
	float brightness, uint32 flags)
{
	return base;
}


rgb_color
FlatControlLook::_EdgeShadowColor(const rgb_color& base, float contrast,
	float brightness, uint32 flags)
{
	return base;
}




rgb_color
FlatControlLook::_BevelLightColor(const rgb_color& base, uint32 flags)
{
	rgb_color bevelLightColor = tint_color(base, 1.0);
	return bevelLightColor;
}


rgb_color
FlatControlLook::_BevelShadowColor(const rgb_color& base, uint32 flags)
{
	rgb_color bevelShadowColor = tint_color(base, 1.0);
	return bevelShadowColor;
}


void
FlatControlLook::_MakeGradient(BGradientLinear& gradient, const BRect& rect,
	const rgb_color& base, float topTint, float bottomTint,
	orientation orientation) const
{
	gradient.AddColor(tint_color(base, 0.97), 0);
	gradient.AddColor(tint_color(base, 1.0), 255);
	gradient.SetStart(rect.LeftTop());
	if (orientation == B_HORIZONTAL)
		gradient.SetEnd(rect.LeftBottom());
	else
		gradient.SetEnd(rect.RightTop());
}


void
FlatControlLook::_MakeGlossyGradient(BGradientLinear& gradient, const BRect& rect,
	const rgb_color& base, float topTint, float middle1Tint,
	float middle2Tint, float bottomTint,
	orientation orientation) const
{
	gradient.AddColor(tint_color(base, topTint), 0);
	gradient.AddColor(tint_color(base, bottomTint), 255);
	gradient.SetStart(rect.LeftTop());
	if (orientation == B_HORIZONTAL)
		gradient.SetEnd(rect.LeftBottom());
	else
		gradient.SetEnd(rect.RightTop());
}


void
FlatControlLook::_MakeButtonGradient(BGradientLinear& gradient, BRect& rect,
	const rgb_color& base, uint32 flags, orientation orientation) const
{
	float topTint = 0.99;
	float middleTint1 = 0.99;
	float middleTint2 = 1.0;
	float bottomTint = 1.05;

	if ((flags & B_ACTIVATED) != 0) {
		topTint = 1.11;
		bottomTint = 1.08;
	}

	if ((flags & B_DISABLED) != 0) {
		topTint = (topTint + B_NO_TINT) / 2;
		middleTint1 = (middleTint1 + B_NO_TINT) / 2;
		middleTint2 = (middleTint2 + B_NO_TINT) / 2;
		bottomTint = (bottomTint + B_NO_TINT) / 2;
	} else if ((flags & B_HOVER) != 0) {
		topTint = 1.0;
		middleTint1 = 1.0;
		middleTint2 = 1.0;
		bottomTint = 1.0;
	}

	if ((flags & B_ACTIVATED) != 0) {
		_MakeGradient(gradient, rect, base, topTint, bottomTint, orientation);
	} else {
		_MakeGlossyGradient(gradient, rect, base, topTint, middleTint1,
			middleTint2, bottomTint, orientation);
	}
}


} // bprivate


extern "C" BControlLook* (instantiate_control_look)(image_id id)
{
	return new (std::nothrow)BPrivate::FlatControlLook();
}
