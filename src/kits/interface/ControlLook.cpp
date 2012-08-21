/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione <jscipione@gmail.com>
 */


#include <ControlLook.h>

#include <Control.h>
#include <GradientLinear.h>
#include <Region.h>
#include <Shape.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include "ContainerWindow.h"


namespace BPrivate {


static const float kEdgeBevelLightTint = 0.59;
static const float kEdgeBevelShadowTint = 1.0735;


BControlLook::BControlLook():
	fCachedOutline(false),
	fCachedWorkspace(-1)
{
}


BControlLook::~BControlLook()
{
}


BAlignment
BControlLook::DefaultLabelAlignment() const
{
	return BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER);
}


float
BControlLook::DefaultLabelSpacing() const
{
	return ceilf(be_plain_font->Size() / 2.0);
}


float
BControlLook::DefaultItemSpacing() const
{
	return ceilf(be_plain_font->Size() * 0.85);
}


float
BControlLook::ComposeSpacing(float spacing)
{
	if (spacing == B_USE_DEFAULT_SPACING || spacing == B_USE_ITEM_SPACING) {
		return be_control_look->DefaultItemSpacing();
	} else if (spacing == B_USE_HALF_ITEM_SPACING) {
		return be_control_look->DefaultItemSpacing() * 0.5f;
	} else if (spacing == B_USE_WINDOW_INSETS) {
		return be_control_look->DefaultItemSpacing();
	} else if (spacing == B_USE_SMALL_SPACING) {
		return be_control_look->DefaultItemSpacing() * 0.7f;
	} else if (spacing == B_USE_BIG_SPACING) {
		return be_control_look->DefaultItemSpacing() * 1.3f;
	}
	return spacing;
}


uint32
BControlLook::Flags(BControl* control) const
{
	uint32 flags = 0;

	if (!control->IsEnabled())
		flags |= B_DISABLED;

	if (control->IsFocus())
		flags |= B_FOCUSED;

	if (control->Value() == B_CONTROL_ON)
		flags |= B_ACTIVATED;

	if (control->Parent() != NULL
		&& (control->Parent()->Flags() & B_DRAW_ON_CHILDREN) != 0) {
		// In this constellation, assume we want to render the control
		// against the already existing view contents of the parent view.
		flags |= B_BLEND_FRAME;
	}

	return flags;
}


// #pragma mark -


void
BControlLook::DrawButtonFrame(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, const rgb_color& background, uint32 flags,
	uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, 0.0f, 0.0f, 0.0f, 0.0f, base,
		background, 1.0, 1.0, flags, borders);
}


void
BControlLook::DrawButtonFrame(BView* view, BRect& rect, const BRect& updateRect,
	float radius, const rgb_color& base, const rgb_color& background, uint32 flags,
	uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, radius, radius, radius, radius,
		base, background, 1.0, 1.0, flags, borders);
}


void
BControlLook::DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	const rgb_color& background, uint32 flags,
	uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, leftTopRadius, rightTopRadius,
		leftBottomRadius, rightBottomRadius, base, background,
		1.0, 1.0, flags, borders);
}


void
BControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders, enum orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, 0.0f, 0.0f, 0.0f, 0.0f,
		base, flags, borders, orientation);
}


void
BControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float radius, const rgb_color& base, uint32 flags,
	uint32 borders, enum orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, radius, radius, radius,
		radius, base, flags, borders, orientation);
}


void
BControlLook::DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	uint32 flags, uint32 borders, enum orientation orientation)
{
	_DrawButtonBackground(view, rect, updateRect, leftTopRadius,
		rightTopRadius, leftBottomRadius, rightBottomRadius, base, flags,
		borders, orientation);
}


void
BControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// the surface edges

	// colors
	float topTint;
	float bottomTint;

	if ((flags & B_ACTIVATED) != 0) {
		rgb_color bevelColor1 = tint_color(base, 1.40);
		rgb_color bevelColor2 = tint_color(base, 1.25);

		topTint = 1.25;
		bottomTint = 1.20;

		_DrawFrame(view, rect,
			bevelColor1, bevelColor1,
			bevelColor2, bevelColor2,
			borders & B_TOP_BORDER);
	} else {
		rgb_color cornerColor = tint_color(base, 0.9);
		rgb_color bevelColorTop = tint_color(base, 0.5);
		rgb_color bevelColorLeft = tint_color(base, 0.7);
		rgb_color bevelColorRightBottom = tint_color(base, 1.08);

		topTint = 0.69;
		bottomTint = 1.03;

		_DrawFrame(view, rect,
			bevelColorLeft, bevelColorTop,
			bevelColorRightBottom, bevelColorRightBottom,
			cornerColor, cornerColor,
			borders);
	}

	// draw surface top
	_FillGradient(view, rect, base, topTint, bottomTint);
}


void
BControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, 0.0f, 0.0f, 0.0f, 0.0f, base,
		background, 0.6, 1.0, flags, borders);
}


void
BControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float radius, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, radius, radius, radius, radius,
		base, background, 0.6, 1.0, flags, borders);
}


void
BControlLook::DrawMenuFieldFrame(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius,
	float rightTopRadius, float leftBottomRadius,
	float rightBottomRadius, const rgb_color& base,
	const rgb_color& background, uint32 flags, uint32 borders)
{
	_DrawButtonFrame(view, rect, updateRect, leftTopRadius, rightTopRadius,
		leftBottomRadius, rightBottomRadius, base, background, 0.6, 1.0,
		flags, borders);
}


void
BControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, bool popupIndicator,
	uint32 flags)
{
	_DrawMenuFieldBackgroundOutside(view, rect, updateRect,
		0.0f, 0.0f, 0.0f, 0.0f, base, popupIndicator, flags);
}


void
BControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	_DrawMenuFieldBackgroundInside(view, rect, updateRect,
		0.0f, 0.0f, 0.0f, 0.0f, base, flags, borders);
}


void
BControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float radius, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	_DrawMenuFieldBackgroundOutside(view, rect, updateRect, radius, radius,
		radius, radius, base, popupIndicator, flags);
}


void
BControlLook::DrawMenuFieldBackground(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	_DrawMenuFieldBackgroundOutside(view, rect, updateRect, leftTopRadius,
		rightTopRadius, leftBottomRadius, rightBottomRadius, base,
		popupIndicator, flags);
}


void
BControlLook::DrawMenuBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// surface top color
	rgb_color background = tint_color(base, 0.75);

	// inner bevel colors
	rgb_color bevelLightColor;
	rgb_color bevelShadowColor;

	if ((flags & B_DISABLED) != 0) {
		bevelLightColor = tint_color(background, 0.80);
		bevelShadowColor = tint_color(background, 1.07);
	} else {
		bevelLightColor = tint_color(background, 0.6);
		bevelShadowColor = tint_color(background, 1.12);
	}

	// draw inner bevel
	_DrawFrame(view, rect,
		bevelLightColor, bevelLightColor,
		bevelShadowColor, bevelShadowColor,
		borders);

	// draw surface top
	view->SetHighColor(background);
	view->FillRect(rect);
}


void
BControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// surface edges
	float topTint;
	float bottomTint;
	rgb_color selectedColor = base;

	if ((flags & B_ACTIVATED) != 0) {
		topTint = 0.9;
		bottomTint = 1.05;
	} else if ((flags & B_DISABLED) != 0) {
		topTint = 0.80;
		bottomTint = 1.07;
	} else {
		topTint = 0.6;
		bottomTint = 1.12;
	}

	rgb_color bevelLightColor = tint_color(selectedColor, topTint);
	rgb_color bevelShadowColor = tint_color(selectedColor, bottomTint);

	// draw surface edges
	_DrawFrame(view, rect,
		bevelLightColor, bevelLightColor,
		bevelShadowColor, bevelShadowColor,
		borders);

	// draw surface top
	view->SetLowColor(selectedColor);
//	_FillGradient(view, rect, selectedColor, topTint, bottomTint);
	_FillGradient(view, rect, selectedColor, bottomTint, topTint);
}


void
BControlLook::DrawStatusBar(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, const rgb_color& barColor, float progressPosition)
{
	if (!rect.Intersects(updateRect))
		return;

	_DrawOuterResessedFrame(view, rect, base, 0.6);

	// colors
	rgb_color dark1BorderColor = tint_color(base, 1.3);
	rgb_color dark2BorderColor = tint_color(base, 1.2);
	rgb_color dark1FilledBorderColor = tint_color(barColor, 1.20);
	rgb_color dark2FilledBorderColor = tint_color(barColor, 1.45);

	BRect filledRect(rect);
	filledRect.right = progressPosition - 1;

	BRect nonfilledRect(rect);
	nonfilledRect.left = progressPosition;

	bool filledSurface = filledRect.Width() > 0;
	bool nonfilledSurface = nonfilledRect.Width() > 0;

	if (filledSurface) {
		_DrawFrame(view, filledRect,
			dark1FilledBorderColor, dark1FilledBorderColor,
			dark2FilledBorderColor, dark2FilledBorderColor);

		_FillGlossyGradient(view, filledRect, barColor, 0.55, 0.68, 0.76, 0.90);
	}

	if (nonfilledSurface) {
		_DrawFrame(view, nonfilledRect, dark1BorderColor, dark1BorderColor,
			dark2BorderColor, dark2BorderColor,
			B_TOP_BORDER | B_BOTTOM_BORDER | B_RIGHT_BORDER);

		if (nonfilledRect.left < nonfilledRect.right) {
			// shadow from fill bar, or left border
			rgb_color leftBorder = dark1BorderColor;
			if (filledSurface)
				leftBorder = tint_color(base, 0.50);
			view->SetHighColor(leftBorder);
			view->StrokeLine(nonfilledRect.LeftTop(),
				nonfilledRect.LeftBottom());
			nonfilledRect.left++;
		}

		_FillGradient(view, nonfilledRect, base, 0.25, 0.06);
	}
}


void
BControlLook::DrawCheckBox(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags)
{
	if (!rect.Intersects(updateRect))
		return;

	rgb_color dark1BorderColor;
	rgb_color dark2BorderColor;
	rgb_color navigationColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

	if ((flags & B_DISABLED) != 0) {
		_DrawOuterResessedFrame(view, rect, base, 0.0, 1.0, flags);

		dark1BorderColor = tint_color(base, 1.15);
		dark2BorderColor = tint_color(base, 1.15);
	} else if ((flags & B_CLICKED) != 0) {
		dark1BorderColor = tint_color(base, 1.50);
		dark2BorderColor = tint_color(base, 1.48);

		_DrawFrame(view, rect,
			dark1BorderColor, dark1BorderColor,
			dark2BorderColor, dark2BorderColor);

		dark2BorderColor = dark1BorderColor;
	} else {
		_DrawOuterResessedFrame(view, rect, base, 0.6, 1.0, flags);

		dark1BorderColor = tint_color(base, 1.40);
		dark2BorderColor = tint_color(base, 1.38);
	}

	if ((flags & B_FOCUSED) != 0) {
		dark1BorderColor = navigationColor;
		dark2BorderColor = navigationColor;
	}

	_DrawFrame(view, rect,
		dark1BorderColor, dark1BorderColor,
		dark2BorderColor, dark2BorderColor);

	if ((flags & B_DISABLED) != 0) {
		_FillGradient(view, rect, base, 0.4, 0.2);
	} else {
		_FillGradient(view, rect, base, 0.15, 0.0);
	}

	rgb_color markColor;
	if (_RadioButtonAndCheckBoxMarkColor(base, markColor, flags)) {
		view->SetHighColor(markColor);

		rect.InsetBy(2, 2);
		view->SetPenSize(max_c(1.0, ceilf(rect.Width() / 3.5)));
		view->SetDrawingMode(B_OP_OVER);

		view->StrokeLine(rect.LeftTop(), rect.RightBottom());
		view->StrokeLine(rect.LeftBottom(), rect.RightTop());
	}
}


void
BControlLook::DrawRadioButton(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags)
{
	if (!rect.Intersects(updateRect))
		return;

	rgb_color borderColor;
	rgb_color bevelLight;
	rgb_color bevelShadow;
	rgb_color navigationColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

	if ((flags & B_DISABLED) != 0) {
		borderColor = tint_color(base, 1.15);
		bevelLight = base;
		bevelShadow = base;
	} else if ((flags & B_CLICKED) != 0) {
		borderColor = tint_color(base, 1.50);
		bevelLight = borderColor;
		bevelShadow = borderColor;
	} else {
		borderColor = tint_color(base, 1.45);
		bevelLight = tint_color(base, 0.55);
		bevelShadow = tint_color(base, 1.11);
	}

	if ((flags & B_FOCUSED) != 0) {
		borderColor = navigationColor;
	}

	BGradientLinear bevelGradient;
	bevelGradient.AddColor(bevelShadow, 0);
	bevelGradient.AddColor(bevelLight, 255);
	bevelGradient.SetStart(rect.LeftTop());
	bevelGradient.SetEnd(rect.RightBottom());

	view->FillEllipse(rect, bevelGradient);
	rect.InsetBy(1, 1);

	bevelGradient.MakeEmpty();
	bevelGradient.AddColor(borderColor, 0);
	bevelGradient.AddColor(tint_color(borderColor, 0.8), 255);
	view->FillEllipse(rect, bevelGradient);
	rect.InsetBy(1, 1);

	float topTint;
	float bottomTint;
	if ((flags & B_DISABLED) != 0) {
		topTint = 0.4;
		bottomTint = 0.2;
	} else {
		topTint = 0.15;
		bottomTint = 0.0;
	}

	BGradientLinear gradient;
	_MakeGradient(gradient, rect, base, topTint, bottomTint);
	view->FillEllipse(rect, gradient);

	rgb_color markColor;
	if (_RadioButtonAndCheckBoxMarkColor(base, markColor, flags)) {
		view->SetHighColor(markColor);
		rect.InsetBy(3, 3);
		view->FillEllipse(rect);
	}
}


void
BControlLook::DrawScrollBarBackground(BView* view, BRect& rect1, BRect& rect2,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	enum orientation orientation)
{
	DrawScrollBarBackground(view, rect1, updateRect, base, flags, orientation);
	DrawScrollBarBackground(view, rect2, updateRect, base, flags, orientation);
}


void
BControlLook::DrawScrollBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	enum orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	float gradient1Tint;
	float gradient2Tint;
	float darkEdge1Tint;
	float darkEdge2Tint;
	float shadowTint;

	if ((flags & B_DISABLED) != 0) {
		gradient1Tint = 0.9;
		gradient2Tint = 0.8;
		darkEdge1Tint = B_DARKEN_2_TINT;
		darkEdge2Tint = B_DARKEN_2_TINT;
		shadowTint = gradient1Tint;
	} else {
		gradient1Tint = 1.10;
		gradient2Tint = 1.05;
		darkEdge1Tint = B_DARKEN_3_TINT;
		darkEdge2Tint = B_DARKEN_2_TINT;
		shadowTint = gradient1Tint;
	}

	rgb_color darkEdge1 = tint_color(base, darkEdge1Tint);
	rgb_color darkEdge2 = tint_color(base, darkEdge2Tint);
	rgb_color shadow = tint_color(base, shadowTint);

	if (orientation == B_HORIZONTAL) {
		// dark vertical line on left edge
		if (rect.Width() > 0) {
			view->SetHighColor(darkEdge1);
			view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
			rect.left++;
		}
		// dark vertical line on right edge
		if (rect.Width() >= 0) {
			view->SetHighColor(darkEdge2);
			view->StrokeLine(rect.RightTop(), rect.RightBottom());
			rect.right--;
		}
		// vertical shadow line after left edge
		if (rect.Width() >= 0) {
			view->SetHighColor(shadow);
			view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
			rect.left++;
		}
		// fill
		if (rect.Width() >= 0) {
			_FillGradient(view, rect, base, gradient1Tint, gradient2Tint,
				orientation);
		}
	} else {
		// dark vertical line on top edge
		if (rect.Height() > 0) {
			view->SetHighColor(darkEdge1);
			view->StrokeLine(rect.LeftTop(), rect.RightTop());
			rect.top++;
		}
		// dark vertical line on bottom edge
		if (rect.Height() >= 0) {
			view->SetHighColor(darkEdge2);
			view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
			rect.bottom--;
		}
		// horizontal shadow line after top edge
		if (rect.Height() >= 0) {
			view->SetHighColor(shadow);
			view->StrokeLine(rect.LeftTop(), rect.RightTop());
			rect.top++;
		}
		// fill
		if (rect.Height() >= 0) {
			_FillGradient(view, rect, base, gradient1Tint, gradient2Tint,
				orientation);
		}
	}
}


void
BControlLook::DrawScrollViewFrame(BView* view, BRect& rect,
	const BRect& updateRect, BRect verticalScrollBarFrame,
	BRect horizontalScrollBarFrame, const rgb_color& base,
	border_style border, uint32 flags, uint32 _borders)
{
	// calculate scroll corner rect before messing with the "rect"
	BRect scrollCornerFillRect(rect.right, rect.bottom,
		rect.right, rect.bottom);
	if (horizontalScrollBarFrame.IsValid())
		scrollCornerFillRect.left = horizontalScrollBarFrame.right + 1;
	if (verticalScrollBarFrame.IsValid())
		scrollCornerFillRect.top = verticalScrollBarFrame.bottom + 1;

	if (border == B_NO_BORDER) {
		if (scrollCornerFillRect.IsValid()) {
			view->SetHighColor(base);
			view->FillRect(scrollCornerFillRect);
		}
		return;
	}

	bool excludeScrollCorner = border == B_FANCY_BORDER
		&& horizontalScrollBarFrame.IsValid()
		&& verticalScrollBarFrame.IsValid();

	uint32 borders = _borders;
	if (excludeScrollCorner) {
		rect.bottom = horizontalScrollBarFrame.top;
		rect.right = verticalScrollBarFrame.left;
		borders &= ~(B_RIGHT_BORDER | B_BOTTOM_BORDER);
	}

	rgb_color scrollbarFrameColor = tint_color(base, B_DARKEN_2_TINT);

	if (border == B_FANCY_BORDER)
		_DrawOuterResessedFrame(view, rect, base, 1.0, 1.0, flags, borders);

	if ((flags & B_FOCUSED) != 0) {
		rgb_color focusColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
		_DrawFrame(view, rect, focusColor, focusColor, focusColor, focusColor,
			borders);
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


void
BControlLook::DrawArrowShape(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 direction, uint32 flags, float tint)
{
	BPoint tri1, tri2, tri3;
	float hInset = rect.Width() / 3;
	float vInset = rect.Height() / 3;
	rect.InsetBy(hInset, vInset);

	switch (direction) {
		case B_LEFT_ARROW:
			tri1.Set(rect.right, rect.top);
			tri2.Set(rect.right - rect.Width() / 1.33,
				(rect.top + rect.bottom + 1) /2 );
			tri3.Set(rect.right, rect.bottom + 1);
			break;
		case B_RIGHT_ARROW:
			tri1.Set(rect.left, rect.bottom + 1);
			tri2.Set(rect.left + rect.Width() / 1.33,
				(rect.top + rect.bottom + 1) / 2);
			tri3.Set(rect.left, rect.top);
			break;
		case B_UP_ARROW:
			tri1.Set(rect.left, rect.bottom);
			tri2.Set((rect.left + rect.right + 1) / 2,
				rect.bottom - rect.Height() / 1.33);
			tri3.Set(rect.right + 1, rect.bottom);
			break;
		case B_DOWN_ARROW:
		default:
			tri1.Set(rect.left, rect.top);
			tri2.Set((rect.left + rect.right + 1) / 2,
				rect.top + rect.Height() / 1.33);
			tri3.Set(rect.right + 1, rect.top);
			break;
	}
	// offset triangle if down
	if ((flags & B_ACTIVATED) != 0)
		view->MovePenTo(BPoint(1, 1));
	else
		view->MovePenTo(BPoint(0, 0));

	BShape arrowShape;
	arrowShape.MoveTo(tri1);
	arrowShape.LineTo(tri2);
	arrowShape.LineTo(tri3);

	if ((flags & B_DISABLED) != 0)
		tint = (tint + B_NO_TINT + B_NO_TINT) / 3;

	view->SetHighColor(tint_color(base, tint));

	float penSize = view->PenSize();
	drawing_mode mode = view->DrawingMode();

	view->SetPenSize(ceilf(hInset / 2.0));
	view->SetDrawingMode(B_OP_OVER);
	view->StrokeShape(&arrowShape);

	view->SetPenSize(penSize);
	view->SetDrawingMode(mode);
}


rgb_color
BControlLook::SliderBarColor(const rgb_color& base)
{
	return tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT);
}


void
BControlLook::DrawSliderBar(BView* view, BRect rect, const BRect& updateRect,
	const rgb_color& base, rgb_color leftFillColor, rgb_color rightFillColor,
	float sliderScale, uint32 flags, enum orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
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

	// reset clipping constraints
	view->ConstrainClippingRegion(NULL);
}


void
BControlLook::DrawSliderBar(BView* view, BRect rect, const BRect& updateRect,
	const rgb_color& base, rgb_color fillColor, uint32 flags,
	enum orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
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
	BRegion region(rect);
	region.Exclude(barRect);
	view->ConstrainClippingRegion(&region);

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
		frameLightTint = 1.20;
		frameShadowTint = 1.25;
		fillLightTint = 0.9;
		fillShadowTint = 1.05;
		edgeLightAlpha = 12;
		edgeShadowAlpha = 12;
		frameLightAlpha = 40;
		frameShadowAlpha = 45;

		fillColor.red = uint8(fillColor.red * 0.4 + base.red * 0.6);
		fillColor.green = uint8(fillColor.green * 0.4 + base.green * 0.6);
		fillColor.blue = uint8(fillColor.blue * 0.4 + base.blue * 0.6);
	} else {
		edgeLightTint = 0.65;
		edgeShadowTint = 1.07;
		frameLightTint = 1.40;
		frameShadowTint = 1.50;
		fillLightTint = 0.8;
		fillShadowTint = 1.1;
		edgeLightAlpha = 15;
		edgeShadowAlpha = 15;
		frameLightAlpha = 92;
		frameShadowAlpha = 107;
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

	view->ConstrainClippingRegion(NULL);

	view->BeginLineArray(4);
	if (orientation == B_HORIZONTAL) {
		view->AddLine(barRect.LeftTop(), barRect.RightTop(), edgeShadowColor);
		view->AddLine(barRect.LeftBottom(), barRect.RightBottom(),
			edgeLightColor);
		barRect.InsetBy(0, 1);
		view->AddLine(barRect.LeftTop(), barRect.RightTop(), frameShadowColor);
		view->AddLine(barRect.LeftBottom(), barRect.RightBottom(),
			frameLightColor);
		barRect.InsetBy(0, 1);
	} else {
		view->AddLine(barRect.LeftTop(), barRect.LeftBottom(), edgeShadowColor);
		view->AddLine(barRect.RightTop(), barRect.RightBottom(),
			edgeLightColor);
		barRect.InsetBy(1, 0);
		view->AddLine(barRect.LeftTop(), barRect.LeftBottom(), frameShadowColor);
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
BControlLook::DrawSliderThumb(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, enum orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// figure out frame color
	rgb_color frameLightColor;
	rgb_color frameShadowColor;
	rgb_color shadowColor = (rgb_color){ 0, 0, 0, 60 };

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

	_DrawFrame(view, rect, frameLightColor, frameLightColor,
		frameShadowColor, frameShadowColor);

	flags &= ~B_ACTIVATED;
	DrawButtonBackground(view, rect, updateRect, base, flags);

	// thumb shadow
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetHighColor(shadowColor);
	originalRect.left++;
	originalRect.top++;
	view->StrokeLine(originalRect.LeftBottom(), originalRect.RightBottom());
	originalRect.bottom--;
	view->StrokeLine(originalRect.RightTop(), originalRect.RightBottom());

	// thumb edge
	if (orientation == B_HORIZONTAL) {
		rect.InsetBy(0, floorf(rect.Height() / 4));
		rect.left = floorf((rect.left + rect.right) / 2);
		rect.right = rect.left + 1;
		shadowColor = tint_color(base, B_DARKEN_2_TINT);
		shadowColor.alpha = 128;
		view->SetHighColor(shadowColor);
		view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
		rgb_color lightColor = tint_color(base, B_LIGHTEN_2_TINT);
		lightColor.alpha = 128;
		view->SetHighColor(lightColor);
		view->StrokeLine(rect.RightTop(), rect.RightBottom());
	} else {
		rect.InsetBy(floorf(rect.Width() / 4), 0);
		rect.top = floorf((rect.top + rect.bottom) / 2);
		rect.bottom = rect.top + 1;
		shadowColor = tint_color(base, B_DARKEN_2_TINT);
		shadowColor.alpha = 128;
		view->SetHighColor(shadowColor);
		view->StrokeLine(rect.LeftTop(), rect.RightTop());
		rgb_color lightColor = tint_color(base, B_LIGHTEN_2_TINT);
		lightColor.alpha = 128;
		view->SetHighColor(lightColor);
		view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
	}

	view->SetDrawingMode(B_OP_COPY);
}


void
BControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	enum orientation orientation)
{
	DrawSliderTriangle(view, rect, updateRect, base, base, flags, orientation);
}


void
BControlLook::DrawSliderTriangle(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, const rgb_color& fill,
	uint32 flags, enum orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// figure out frame color
	rgb_color frameLightColor;
	rgb_color frameShadowColor;
	rgb_color shadowColor = (rgb_color){ 0, 0, 0, 60 };

	float topTint = 0.49;
	float middleTint1 = 0.62;
	float middleTint2 = 0.76;
	float bottomTint = 0.90;

	if ((flags & B_DISABLED) != 0) {
		topTint = (topTint + B_NO_TINT) / 2;
		middleTint1 = (middleTint1 + B_NO_TINT) / 2;
		middleTint2 = (middleTint2 + B_NO_TINT) / 2;
		bottomTint = (bottomTint + B_NO_TINT) / 2;
	} else if ((flags & B_HOVER) != 0) {
		static const float kHoverTintFactor = 0.85;
		topTint *= kHoverTintFactor;
		middleTint1 *= kHoverTintFactor;
		middleTint2 *= kHoverTintFactor;
		bottomTint *= kHoverTintFactor;
	}

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

	// make room for the shadow
	rect.right--;
	rect.bottom--;

	uint32 viewFlags = view->Flags();
	view->SetFlags(viewFlags | B_SUBPIXEL_PRECISE);
	view->SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);

	float centerh = (rect.left + rect.right) / 2;
	float centerv = (rect.top + rect.bottom) / 2;

	BShape shape;
	if (orientation == B_HORIZONTAL) {
		shape.MoveTo(BPoint(rect.left + 0.5, rect.bottom + 0.5));
		shape.LineTo(BPoint(rect.right + 0.5, rect.bottom + 0.5));
		shape.LineTo(BPoint(rect.right + 0.5, rect.bottom - 1 + 0.5));
		shape.LineTo(BPoint(centerh + 0.5, rect.top + 0.5));
		shape.LineTo(BPoint(rect.left + 0.5, rect.bottom - 1 + 0.5));
	} else {
		shape.MoveTo(BPoint(rect.right + 0.5, rect.top + 0.5));
		shape.LineTo(BPoint(rect.right + 0.5, rect.bottom + 0.5));
		shape.LineTo(BPoint(rect.right - 1 + 0.5, rect.bottom + 0.5));
		shape.LineTo(BPoint(rect.left + 0.5, centerv + 0.5));
		shape.LineTo(BPoint(rect.right - 1 + 0.5, rect.top + 0.5));
	}
	shape.Close();

	view->MovePenTo(BPoint(1, 1));

	view->SetDrawingMode(B_OP_ALPHA);
	view->SetHighColor(shadowColor);
	view->StrokeShape(&shape);

	view->MovePenTo(B_ORIGIN);

	view->SetDrawingMode(B_OP_COPY);
	view->SetHighColor(frameLightColor);
	view->StrokeShape(&shape);

	rect.InsetBy(1, 1);
	shape.Clear();
	if (orientation == B_HORIZONTAL) {
		shape.MoveTo(BPoint(rect.left, rect.bottom + 1));
		shape.LineTo(BPoint(rect.right + 1, rect.bottom + 1));
		shape.LineTo(BPoint(centerh + 0.5, rect.top));
	} else {
		shape.MoveTo(BPoint(rect.right + 1, rect.top));
		shape.LineTo(BPoint(rect.right + 1, rect.bottom + 1));
		shape.LineTo(BPoint(rect.left, centerv + 0.5));
	}
	shape.Close();

	BGradientLinear gradient;
	if ((flags & B_DISABLED) != 0) {
		_MakeGradient(gradient, rect, fill, topTint, bottomTint);
	} else {
		_MakeGlossyGradient(gradient, rect, fill, topTint, middleTint1,
			middleTint2, bottomTint);
	}

	view->FillShape(&shape, gradient);

	view->SetFlags(viewFlags);
}


void
BControlLook::DrawSliderHashMarks(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, int32 count,
	hash_mark_location location, uint32 flags, enum orientation orientation)
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

	int32 hashMarkCount = max_c(count, 2);
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

	if (location & B_HASH_MARKS_BOTTOM) {

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
BControlLook::DrawActiveTab(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// clipping constraints for the view
	BRegion clipping;
	view->GetClippingRegion(&clipping);

	// intersect constraints with updateRect
	BRegion updateClipping(updateRect);
	clipping.IntersectWith(&updateClipping);

	// clip to the intersection
	view->ConstrainClippingRegion(&clipping);

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
		frameLightColor = tint_color(base, 1.25);
		frameShadowColor = tint_color(base, 1.30);
		bevelLightColor = tint_color(base, 0.8);
		bevelShadowColor = tint_color(base, 1.07);
		fillGradient.AddColor(tint_color(base, 0.85), 0);
		fillGradient.AddColor(base, 255);
	} else {
		edgeLightColor = tint_color(base, 0.80);
		edgeShadowColor = tint_color(base, 1.03);
		frameLightColor = tint_color(base, 1.30);
		frameShadowColor = tint_color(base, 1.30);
		bevelLightColor = tint_color(base, 0.6);
		bevelShadowColor = tint_color(base, 1.07);
		fillGradient.AddColor(tint_color(base, 0.75), 0);
		fillGradient.AddColor(tint_color(base, 1.03), 255);
	}

	static const float kRoundCornerRadius = 4.0f;

	// left top corner dimensions
	BRect leftTopCorner(rect);
	leftTopCorner.right = floorf(leftTopCorner.left + kRoundCornerRadius);
	leftTopCorner.bottom = floorf(rect.top + kRoundCornerRadius);
	clipping.Exclude(leftTopCorner);

	// draw the left top corner
	_DrawRoundCornerLeftTop(view, leftTopCorner, updateRect, base,
		edgeShadowColor, frameLightColor, bevelLightColor,
		fillGradient);

	// right top corner dimensions
	BRect rightTopCorner(rect);
	rightTopCorner.right = floorf(rect.right);
	rightTopCorner.left = floorf(rightTopCorner.right - kRoundCornerRadius);
	rightTopCorner.bottom = floorf(rect.top + kRoundCornerRadius);
	clipping.Exclude(rightTopCorner);

	// draw the right top corner
	_DrawRoundCornerRightTop(view, rightTopCorner, updateRect, base,
		edgeShadowColor, edgeLightColor, frameLightColor,
		frameShadowColor, bevelLightColor, bevelShadowColor,
		fillGradient);

	// clip out the corners
	view->ConstrainClippingRegion(&clipping);

	// draw the rest of frame and fill
	_DrawFrame(view, rect, edgeShadowColor, edgeShadowColor, edgeLightColor,
		edgeLightColor,
		borders & (B_LEFT_BORDER | B_TOP_BORDER | B_RIGHT_BORDER));
	if ((borders & B_LEFT_BORDER) == 0)
		rect.left++;
	if ((borders & B_RIGHT_BORDER) == 0)
		rect.right--;

	_DrawFrame(view, rect, frameLightColor, frameLightColor, frameShadowColor,
		frameShadowColor, B_LEFT_BORDER | B_TOP_BORDER | B_RIGHT_BORDER);

	_DrawFrame(view, rect, bevelLightColor, bevelLightColor, bevelShadowColor,
		bevelShadowColor);

	view->FillRect(rect, fillGradient);

	// reset clipping constraints
	view->ConstrainClippingRegion(NULL);
}


void
BControlLook::DrawInactiveTab(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 flags, uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

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
		frameLightColor = tint_color(base, 1.25);
		frameShadowColor = tint_color(base, 1.30);
		bevelLightColor = tint_color(base, 0.8);
		bevelShadowColor = tint_color(base, 1.07);
		fillGradient.AddColor(tint_color(base, 0.85), 0);
		fillGradient.AddColor(base, 255);
	} else {
		edgeLightColor = tint_color(base, 0.80);
		edgeShadowColor = tint_color(base, 1.03);
		frameLightColor = tint_color(base, 1.30);
		frameShadowColor = tint_color(base, 1.30);
		bevelLightColor = tint_color(base, 1.10);
		bevelShadowColor = tint_color(base, 1.17);
		fillGradient.AddColor(tint_color(base, 1.12), 0);
		fillGradient.AddColor(tint_color(base, 1.08), 255);
	}

	// active tabs stand out at the top, but this is an inactive tab
	view->SetHighColor(base);
	view->FillRect(BRect(rect.left, rect.top, rect.right, rect.top + 4));
	rect.top += 4;

	// frame and fill
	_DrawFrame(view, rect, edgeShadowColor, edgeShadowColor, edgeLightColor,
		edgeLightColor,
		borders & (B_LEFT_BORDER | B_TOP_BORDER | B_RIGHT_BORDER));

	_DrawFrame(view, rect, frameLightColor, frameLightColor, frameShadowColor,
		frameShadowColor,
		borders & (B_LEFT_BORDER | B_TOP_BORDER | B_RIGHT_BORDER));

	if (rect.IsValid()) {
		_DrawFrame(view, rect, bevelShadowColor, bevelShadowColor,
			bevelLightColor, bevelLightColor, B_LEFT_BORDER & ~borders);
	} else {
		if ((B_LEFT_BORDER & ~borders) != 0)
			rect.left++;
	}

	view->FillRect(rect, fillGradient);
}


void
BControlLook::DrawSplitter(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, enum orientation orientation, uint32 flags,
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


// #pragma mark -


void
BControlLook::DrawBorder(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, border_style border, uint32 flags, uint32 borders)
{
	if (border == B_NO_BORDER)
		return;

	rgb_color scrollbarFrameColor = tint_color(base, B_DARKEN_2_TINT);
	if ((flags & B_FOCUSED) != 0)
		scrollbarFrameColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

	if (border == B_FANCY_BORDER)
		_DrawOuterResessedFrame(view, rect, base, 1.0, 1.0, flags, borders);

	_DrawFrame(view, rect, scrollbarFrameColor, scrollbarFrameColor,
		scrollbarFrameColor, scrollbarFrameColor, borders);
}


void
BControlLook::DrawRaisedBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	rgb_color lightColor;
	rgb_color shadowColor;

	if ((flags & B_DISABLED) != 0) {
		lightColor = base;
		shadowColor = base;
	} else {
		lightColor = tint_color(base, 0.85);
		shadowColor = tint_color(base, 1.07);
	}

	_DrawFrame(view, rect, lightColor, lightColor, shadowColor, shadowColor,
		borders);
}


void
BControlLook::DrawTextControlBorder(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!rect.Intersects(updateRect))
		return;

	rgb_color dark1BorderColor;
	rgb_color dark2BorderColor;
	rgb_color navigationColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

	if ((flags & B_DISABLED) != 0) {
		_DrawOuterResessedFrame(view, rect, base, 0.0, 1.0, flags, borders);

		if ((flags & B_BLEND_FRAME) != 0)
			dark1BorderColor = (rgb_color){ 0, 0, 0, 40 };
		else
			dark1BorderColor = tint_color(base, 1.15);
		dark2BorderColor = dark1BorderColor;
	} else if ((flags & B_CLICKED) != 0) {
		dark1BorderColor = tint_color(base, 1.50);
		dark2BorderColor = tint_color(base, 1.49);

		// BCheckBox uses this to indicate the clicked state...
		_DrawFrame(view, rect,
			dark1BorderColor, dark1BorderColor,
			dark2BorderColor, dark2BorderColor);

		dark2BorderColor = dark1BorderColor;
	} else {
		_DrawOuterResessedFrame(view, rect, base, 0.6, 1.0, flags, borders);

		if ((flags & B_BLEND_FRAME) != 0) {
			dark1BorderColor = (rgb_color){ 0, 0, 0, 102 };
			dark2BorderColor = (rgb_color){ 0, 0, 0, 97 };
		} else {
			dark1BorderColor = tint_color(base, 1.40);
			dark2BorderColor = tint_color(base, 1.38);
		}
	}

	if ((flags & B_DISABLED) == 0 && (flags & B_FOCUSED) != 0) {
		dark1BorderColor = navigationColor;
		dark2BorderColor = navigationColor;
	}

	if ((flags & B_BLEND_FRAME) != 0) {
		drawing_mode oldMode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);

		_DrawFrame(view, rect,
			dark1BorderColor, dark1BorderColor,
			dark2BorderColor, dark2BorderColor, borders);

		view->SetDrawingMode(oldMode);
	} else {
		_DrawFrame(view, rect,
			dark1BorderColor, dark1BorderColor,
			dark2BorderColor, dark2BorderColor, borders);
	}
}


void
BControlLook::DrawGroupFrame(BView* view, BRect& rect, const BRect& updateRect,
	const rgb_color& base, uint32 borders)
{
	rgb_color frameColor = tint_color(base, 1.30);
	rgb_color bevelLight = tint_color(base, 0.8);
	rgb_color bevelShadow = tint_color(base, 1.03);

	_DrawFrame(view, rect, bevelShadow, bevelShadow, bevelLight, bevelLight,
		borders);

	_DrawFrame(view, rect, frameColor, frameColor, frameColor, frameColor,
		borders);

	_DrawFrame(view, rect, bevelLight, bevelLight, bevelShadow, bevelShadow,
		borders);
}


void
BControlLook::DrawLabel(BView* view, const char* label, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags)
{
	DrawLabel(view, label, rect, updateRect, base, flags,
		DefaultLabelAlignment());
}


void
BControlLook::DrawLabel(BView* view, const char* label, BRect rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	const BAlignment& alignment)
{
	if (!rect.Intersects(updateRect))
		return;

	// truncate the label if necessary and get the width and height
	BString truncatedLabel(label);

	BFont font;
	view->GetFont(&font);

	float width = rect.Width();
	font.TruncateString(&truncatedLabel, B_TRUNCATE_END, width);
	width = font.StringWidth(truncatedLabel.String());

	font_height fontHeight;
	font.GetHeight(&fontHeight);
	float height = ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);

	// handle alignment
	BPoint location;

	switch (alignment.horizontal) {
	default:
		case B_ALIGN_LEFT:
			location.x = rect.left;
			break;
		case B_ALIGN_RIGHT:
			location.x = rect.right - width;
			break;
		case B_ALIGN_CENTER:
			location.x = (rect.left + rect.right - width) / 2.0f;
			break;
	}

	switch (alignment.vertical) {
		case B_ALIGN_TOP:
			location.y = rect.top + ceilf(fontHeight.ascent);
			break;
		default:
		case B_ALIGN_MIDDLE:
			location.y = floorf((rect.top + rect.bottom - height) / 2.0f + 0.5f)
				+ ceilf(fontHeight.ascent);
			break;
		case B_ALIGN_BOTTOM:
			location.y = rect.bottom - ceilf(fontHeight.descent);
			break;
	}

	DrawLabel(view, truncatedLabel.String(), base, flags, location);
}


void
BControlLook::DrawLabel(BView* view, const char* label, const rgb_color& base,
	uint32 flags, const BPoint& where)
{
	// setup the text color
	// TODO: Should either use the ui_color(B_CONTROL_TEXT_COLOR) here,
	// or elliminate that constant alltogether (stippi: +1).

	BWindow* window = view->Window();
	bool isDesktop = window
		&& window->Feel() == kPrivateDesktopWindowFeel
		&& window->Look() == kPrivateDesktopWindowLook
		&& view->Parent()
		&& view->Parent()->Parent() == NULL
		&& (flags & B_IGNORE_OUTLINE) == 0;

	rgb_color	low;
	rgb_color	color;
	rgb_color	glowColor;

	if (isDesktop)
		low = view->Parent()->ViewColor();
	else
		low = base;

	if (low.red + low.green + low.blue > 128 * 3) {
		color = tint_color(low, B_DARKEN_MAX_TINT);
		glowColor = kWhite;
	} else {
		color = tint_color(low, B_LIGHTEN_MAX_TINT);
		glowColor = kBlack;
	}

	if ((flags & B_DISABLED) != 0) {
		color.red = (uint8)(((int32)low.red + color.red + 1) / 2);
		color.green = (uint8)(((int32)low.green + color.green + 1) / 2);
		color.blue = (uint8)(((int32)low.blue + color.blue + 1) / 2);
	}

	drawing_mode oldMode = view->DrawingMode();

	if (isDesktop) {
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
			if (glowColor == kWhite) {
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
			} else if (glowColor == kBlack) {
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

	view->SetHighColor(color);
	view->SetDrawingMode(B_OP_OVER);
	view->DrawString(label, where);
	view->SetDrawingMode(oldMode);
}


void
BControlLook::SetBackgroundInfo(const BMessage& backgroundInfo)
{
	fBackgroundInfo = backgroundInfo;
	fCachedWorkspace = -1;
}


// #pragma mark -


void
BControlLook::_DrawButtonFrame(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	const rgb_color& background, float contrast, float brightness,
	uint32 flags, uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// clipping constraints for the view
	BRegion clipping;
	view->GetClippingRegion(&clipping);

	// intersect constraints with updateRect
	BRegion updateClipping(updateRect);
	clipping.IntersectWith(&updateClipping);

	// clip to the intersection
	view->ConstrainClippingRegion(&clipping);

	// outer edge colors
	rgb_color edgeLightColor;
	rgb_color edgeShadowColor;

	// default button frame color
	// TODO: B_BLEND_FRAME
	float defaultIndicatorTint = 1.2;
	if ((flags & B_DISABLED) != 0)
		defaultIndicatorTint = (B_NO_TINT + defaultIndicatorTint) / 2;

	rgb_color defaultIndicatorColor = tint_color(base, defaultIndicatorTint);
	rgb_color cornerBgColor;

	drawing_mode oldMode = view->DrawingMode();

	if ((flags & B_DEFAULT_BUTTON) != 0) {
		cornerBgColor = defaultIndicatorColor;
		edgeLightColor = _EdgeLightColor(defaultIndicatorColor,
			contrast * ((flags & B_DISABLED) != 0 ? 0.3 : 0.8),
			brightness * ((flags & B_DISABLED) != 0 ? 1.0 : 0.9), flags);
		edgeShadowColor = _EdgeShadowColor(defaultIndicatorColor,
			contrast * ((flags & B_DISABLED) != 0 ? 0.3 : 0.8),
			brightness * ((flags & B_DISABLED) != 0 ? 1.0 : 0.9), flags);

		// draw default button indicator
		view->SetHighColor(background);
		view->FillRect(rect);
		view->SetHighColor(base);
		view->StrokeRoundRect(rect, leftTopRadius, leftTopRadius);
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

		edgeLightColor = _EdgeLightColor(background,
			contrast * ((flags & B_DISABLED) != 0 ? 0.0 : 1.0),
			brightness * 1.0, flags);
		edgeShadowColor = _EdgeShadowColor(background,
			contrast * (flags & B_DISABLED) != 0 ? 0.0 : 1.0,
			brightness * 1.0, flags);
	}

	// frame colors
	rgb_color frameLightColor  = _FrameLightColor(base, flags);
	rgb_color frameShadowColor = _FrameShadowColor(base, flags);

	// rounded corners

	if ((borders & B_LEFT_BORDER) != 0 && (borders & B_TOP_BORDER) != 0
		&& leftTopRadius > 0) {
		// draw left top rounded corner
		BRect leftTopCorner(floorf(rect.left), floorf(rect.top),
			floorf(rect.left + leftTopRadius),
			floorf(rect.top + leftTopRadius));
		clipping.Exclude(leftTopCorner);
		_DrawRoundCornerFrameLeftTop(view, leftTopCorner, updateRect,
			cornerBgColor, edgeShadowColor, frameLightColor);
	}

	if ((borders & B_TOP_BORDER) != 0 && (borders & B_RIGHT_BORDER) != 0
		&& rightTopRadius > 0) {
		// draw right top rounded corner
		BRect rightTopCorner(floorf(rect.right - rightTopRadius),
			floorf(rect.top), floorf(rect.right),
			floorf(rect.top + rightTopRadius));
		clipping.Exclude(rightTopCorner);
		_DrawRoundCornerFrameRightTop(view, rightTopCorner, updateRect,
			cornerBgColor, edgeShadowColor, edgeLightColor,
			frameLightColor, frameShadowColor);
	}

	if ((borders & B_LEFT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& leftBottomRadius > 0) {
		// draw left bottom rounded corner
		BRect leftBottomCorner(floorf(rect.left),
			floorf(rect.bottom - leftBottomRadius),
			floorf(rect.left + leftBottomRadius), floorf(rect.bottom));
		clipping.Exclude(leftBottomCorner);
		_DrawRoundCornerFrameLeftBottom(view, leftBottomCorner, updateRect,
			cornerBgColor, edgeShadowColor, edgeLightColor,
			frameLightColor, frameShadowColor);
	}

	if ((borders & B_RIGHT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& rightBottomRadius > 0) {
		// draw right bottom rounded corner
		BRect rightBottomCorner(floorf(rect.right - rightBottomRadius),
			floorf(rect.bottom - rightBottomRadius), floorf(rect.right),
			floorf(rect.bottom));
		clipping.Exclude(rightBottomCorner);
		_DrawRoundCornerFrameRightBottom(view, rightBottomCorner,
			updateRect, cornerBgColor, edgeLightColor, frameShadowColor);
	}

	// clip out the corners
	view->ConstrainClippingRegion(&clipping);

	// draw outer edge
	if ((flags & B_DEFAULT_BUTTON) != 0) {
		_DrawOuterResessedFrame(view, rect, defaultIndicatorColor,
			contrast * ((flags & B_DISABLED) != 0 ? 0.3 : 0.8),
			brightness * ((flags & B_DISABLED) != 0 ? 1.0 : 0.9),
			flags, borders);
	} else {
		_DrawOuterResessedFrame(view, rect, background,
			contrast * ((flags & B_DISABLED) != 0 ? 0.0 : 1.0),
			brightness * 1.0, flags, borders);
	}

	view->SetDrawingMode(oldMode);

	// draw frame
	if ((flags & B_BLEND_FRAME) != 0) {
		drawing_mode oldDrawingMode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);

		_DrawFrame(view, rect, frameLightColor, frameLightColor,
			frameShadowColor, frameShadowColor, borders);

		view->SetDrawingMode(oldDrawingMode);
	} else {
		_DrawFrame(view, rect, frameLightColor, frameLightColor,
			frameShadowColor, frameShadowColor, borders);
	}

	view->ConstrainClippingRegion(NULL);
}


void
BControlLook::_DrawOuterResessedFrame(BView* view, BRect& rect,
	const rgb_color& base, float contrast, float brightness, uint32 flags,
	uint32 borders)
{
	rgb_color edgeLightColor = _EdgeLightColor(base, contrast,
		brightness, flags);
	rgb_color edgeShadowColor = _EdgeShadowColor(base, contrast,
		brightness, flags);

	if ((flags & B_BLEND_FRAME) != 0) {
		// assumes the background has already been painted
		drawing_mode oldDrawingMode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);

		_DrawFrame(view, rect, edgeShadowColor, edgeShadowColor,
			edgeLightColor, edgeLightColor, borders);

		view->SetDrawingMode(oldDrawingMode);
	} else {
		_DrawFrame(view, rect, edgeShadowColor, edgeShadowColor,
			edgeLightColor, edgeLightColor, borders);
	}
}


void
BControlLook::_DrawFrame(BView* view, BRect& rect, const rgb_color& left,
	const rgb_color& top, const rgb_color& right, const rgb_color& bottom,
	uint32 borders)
{
	view->BeginLineArray(4);

	if (borders & B_LEFT_BORDER) {
		view->AddLine(
			BPoint(rect.left, rect.bottom),
			BPoint(rect.left, rect.top), left);
		rect.left++;
	}
	if (borders & B_TOP_BORDER) {
		view->AddLine(
			BPoint(rect.left, rect.top),
			BPoint(rect.right, rect.top), top);
		rect.top++;
	}
	if (borders & B_RIGHT_BORDER) {
		view->AddLine(
			BPoint(rect.right, rect.top),
			BPoint(rect.right, rect.bottom), right);
		rect.right--;
	}
	if (borders & B_BOTTOM_BORDER) {
		view->AddLine(
			BPoint(rect.left, rect.bottom),
			BPoint(rect.right, rect.bottom), bottom);
		rect.bottom--;
	}

	view->EndLineArray();
}


void
BControlLook::_DrawFrame(BView* view, BRect& rect, const rgb_color& left,
	const rgb_color& top, const rgb_color& right, const rgb_color& bottom,
	const rgb_color& rightTop, const rgb_color& leftBottom, uint32 borders)
{
	view->BeginLineArray(6);

	if (borders & B_TOP_BORDER) {
		if (borders & B_RIGHT_BORDER) {
			view->AddLine(
				BPoint(rect.left, rect.top),
				BPoint(rect.right - 1, rect.top), top);
			view->AddLine(
				BPoint(rect.right, rect.top),
				BPoint(rect.right, rect.top), rightTop);
		} else {
			view->AddLine(
				BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), top);
		}
		rect.top++;
	}

	if (borders & B_LEFT_BORDER) {
		view->AddLine(
			BPoint(rect.left, rect.top),
			BPoint(rect.left, rect.bottom - 1), left);
		view->AddLine(
			BPoint(rect.left, rect.bottom),
			BPoint(rect.left, rect.bottom), leftBottom);
		rect.left++;
	}

	if (borders & B_BOTTOM_BORDER) {
		view->AddLine(
			BPoint(rect.left, rect.bottom),
			BPoint(rect.right, rect.bottom), bottom);
		rect.bottom--;
	}

	if (borders & B_RIGHT_BORDER) {
		view->AddLine(
			BPoint(rect.right, rect.bottom),
			BPoint(rect.right, rect.top), right);
		rect.right--;
	}

	view->EndLineArray();
}


void
BControlLook::_DrawButtonBackground(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	uint32 flags, uint32 borders, enum orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// clipping constraints for the view
	BRegion clipping;
	view->GetClippingRegion(&clipping);

	// intersect constraints with updateRect
	BRegion updateClipping(updateRect);
	clipping.IntersectWith(&updateClipping);

	// clip to the intersection
	view->ConstrainClippingRegion(&clipping);

	// inner bevel colors
	rgb_color bevelLightColor  = _BevelLightColor(base, flags);
	rgb_color bevelShadowColor = _BevelShadowColor(base, flags);

	// button background color
	rgb_color buttonBgColor;
	if ((flags & B_DISABLED) != 0)
		buttonBgColor = tint_color(base, 0.7);
	else
		buttonBgColor = tint_color(base, B_LIGHTEN_1_TINT);

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
		_DrawRoundCornerBackgroundLeftTop(view, leftTopCorner, updateRect,
			bevelLightColor, fillGradient);
	}

	if ((borders & B_TOP_BORDER) != 0 && (borders & B_RIGHT_BORDER) != 0
		&& rightTopRadius > 0) {
		// draw right top rounded corner
		BRect rightTopCorner(floorf(rect.right - rightTopRadius + 2.0),
			floorf(rect.top), floorf(rect.right),
			floorf(rect.top + rightTopRadius - 2.0));
		clipping.Exclude(rightTopCorner);
		_DrawRoundCornerBackgroundRightTop(view, rightTopCorner,
			updateRect, bevelLightColor, bevelShadowColor, fillGradient);
	}

	if ((borders & B_LEFT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& leftBottomRadius > 0) {
		// draw left bottom rounded corner
		BRect leftBottomCorner(floorf(rect.left),
			floorf(rect.bottom - leftBottomRadius + 2.0),
			floorf(rect.left + leftBottomRadius - 2.0),
			floorf(rect.bottom));
		clipping.Exclude(leftBottomCorner);
		_DrawRoundCornerBackgroundLeftBottom(view, leftBottomCorner,
			updateRect, bevelLightColor, bevelShadowColor, fillGradient);
	}

	if ((borders & B_RIGHT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& rightBottomRadius > 0) {
		// draw right bottom rounded corner
		BRect rightBottomCorner(floorf(rect.right - rightBottomRadius + 2.0),
			floorf(rect.bottom - rightBottomRadius + 2.0), floorf(rect.right),
			floorf(rect.bottom));
		clipping.Exclude(rightBottomCorner);
		_DrawRoundCornerBackgroundRightBottom(view, rightBottomCorner,
			updateRect, bevelShadowColor, fillGradient);
	}

	// clip out the corners
	view->ConstrainClippingRegion(&clipping);

	// draw inner bevel

	if ((flags & B_ACTIVATED) != 0) {
		view->BeginLineArray(4);

		// shadow along left/top borders
		if (borders & B_LEFT_BORDER) {
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.left, rect.bottom), bevelLightColor);
			rect.left++;
		}
		if (borders & B_TOP_BORDER) {
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), bevelLightColor);
			rect.top++;
		}

		// softer shadow along left/top borders
		if (borders & B_LEFT_BORDER) {
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.left, rect.bottom), bevelShadowColor);
			rect.left++;
		}
		if (borders & B_TOP_BORDER) {
			view->AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), bevelShadowColor);
			rect.top++;
		}

		view->EndLineArray();
	} else {
		_DrawFrame(view, rect,
			bevelLightColor, bevelLightColor,
			bevelShadowColor, bevelShadowColor,
			buttonBgColor, buttonBgColor, borders);
	}

	// fill in the background
	view->FillRect(rect, fillGradient);

	// reset clipping constraints
	view->ConstrainClippingRegion(NULL);
}


void
BControlLook::_DrawMenuFieldBackgroundOutside(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	bool popupIndicator, uint32 flags)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	if (popupIndicator) {
		BRect leftRect(rect);
		leftRect.right -= 10;

		BRect rightRect(rect);
		rightRect.left = rightRect.right - 9;

		_DrawMenuFieldBackgroundInside(view, leftRect, updateRect,
			leftTopRadius, 0.0f, leftBottomRadius, 0.0f, base, flags,
			B_LEFT_BORDER | B_TOP_BORDER | B_BOTTOM_BORDER);

		_DrawMenuFieldBackgroundInside(view, rightRect, updateRect,
			0.0f, rightTopRadius, 0.0f, rightBottomRadius, base, flags,
			B_TOP_BORDER | B_RIGHT_BORDER | B_BOTTOM_BORDER);

		// popup marker
		BPoint center(roundf((rightRect.left + rightRect.right) / 2.0),
					  roundf((rightRect.top + rightRect.bottom) / 2.0));
		BPoint triangle[3];
		triangle[0] = center + BPoint(-2.5, -0.5);
		triangle[1] = center + BPoint(2.5, -0.5);
		triangle[2] = center + BPoint(0.0, 2.0);

		uint32 viewFlags = view->Flags();
		view->SetFlags(viewFlags | B_SUBPIXEL_PRECISE);

		rgb_color markColor;
		if ((flags & B_DISABLED) != 0)
			markColor = tint_color(base, 1.35);
		else
			markColor = tint_color(base, 1.65);

		view->SetHighColor(markColor);
		view->FillTriangle(triangle[0], triangle[1], triangle[2]);

		// draw a line on the left of the popup frame
		rgb_color bevelShadowColor = _BevelShadowColor(base, flags);
		view->SetHighColor(bevelShadowColor);
		BPoint leftTopCorner(floorf(rightRect.left - 1.0),
			floorf(rightRect.top - 1.0));
		BPoint leftBottomCorner(floorf(rightRect.left - 1.0),
			floorf(rightRect.bottom + 1.0));
		view->StrokeLine(leftTopCorner, leftBottomCorner);

		view->SetFlags(viewFlags);

		rect = leftRect;
	} else {
		_DrawMenuFieldBackgroundInside(view, rect, updateRect, leftTopRadius,
			rightTopRadius, leftBottomRadius, rightBottomRadius, base, flags);
	}
}


void
BControlLook::_DrawMenuFieldBackgroundInside(BView* view, BRect& rect,
	const BRect& updateRect, float leftTopRadius, float rightTopRadius,
	float leftBottomRadius, float rightBottomRadius, const rgb_color& base,
	uint32 flags, uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// clipping constraints for the view
	BRegion clipping;
	view->GetClippingRegion(&clipping);

	// intersect constraints with updateRect
	BRegion updateClipping(updateRect);
	clipping.IntersectWith(&updateClipping);

	// clip to the intersection
	view->ConstrainClippingRegion(&clipping);

	// frame colors
	rgb_color frameLightColor  = _FrameLightColor(base, flags);
	rgb_color frameShadowColor = _FrameShadowColor(base, flags);

	// indicator background color
	rgb_color indicatorBase;
	if ((borders & B_LEFT_BORDER) != 0)
		indicatorBase = base;
	else {
		if ((flags & B_DISABLED) != 0)
			indicatorBase = tint_color(base, 1.05);
		else
			indicatorBase = tint_color(base, 1.12);
	}

	// bevel colors
	rgb_color cornerColor = tint_color(indicatorBase, 0.85);
	rgb_color bevelColor1 = tint_color(indicatorBase, 0.3);
	rgb_color bevelColor2 = tint_color(indicatorBase, 0.5);
	rgb_color bevelColor3 = tint_color(indicatorBase, 1.03);

	if ((flags & B_DISABLED) != 0) {
		cornerColor = tint_color(indicatorBase, 0.8);
		bevelColor1 = tint_color(indicatorBase, 0.7);
		bevelColor2 = tint_color(indicatorBase, 0.8);
		bevelColor3 = tint_color(indicatorBase, 1.01);
	} else {
		cornerColor = tint_color(indicatorBase, 0.85);
		bevelColor1 = tint_color(indicatorBase, 0.3);
		bevelColor2 = tint_color(indicatorBase, 0.5);
		bevelColor3 = tint_color(indicatorBase, 1.03);
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
		clipping.Exclude(leftTopCorner);

		BRegion cornerClipping(leftTopCorner);
		view->ConstrainClippingRegion(&cornerClipping);

		BRect ellipseRect(leftTopCorner);
		ellipseRect.InsetBy(-1.0, -1.0);
		ellipseRect.right = ellipseRect.left + ellipseRect.Width() * 2;
		ellipseRect.bottom = ellipseRect.top + ellipseRect.Height() * 2;

		// draw the frame (again)
		view->SetHighColor(frameLightColor);
		view->FillEllipse(ellipseRect);

		// draw the bevel and background
		_DrawRoundCornerBackgroundLeftTop(view, leftTopCorner, updateRect,
			bevelColor1, fillGradient);
	}

	if ((borders & B_TOP_BORDER) != 0 && (borders & B_RIGHT_BORDER) != 0
		&& rightTopRadius > 0) {
		// draw right top rounded corner
		BRect rightTopCorner(floorf(rect.right - rightTopRadius + 2.0),
			floorf(rect.top), floorf(rect.right),
			floorf(rect.top + rightTopRadius - 2.0));
		clipping.Exclude(rightTopCorner);

		BRegion cornerClipping(rightTopCorner);
		view->ConstrainClippingRegion(&cornerClipping);

		BRect ellipseRect(rightTopCorner);
		ellipseRect.InsetBy(-1.0, -1.0);
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
	}

	if ((borders & B_LEFT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& leftBottomRadius > 0) {
		// draw left bottom rounded corner
		BRect leftBottomCorner(floorf(rect.left),
			floorf(rect.bottom - leftBottomRadius + 2.0),
			floorf(rect.left + leftBottomRadius - 2.0),
			floorf(rect.bottom));
		clipping.Exclude(leftBottomCorner);

		BRegion cornerClipping(leftBottomCorner);
		view->ConstrainClippingRegion(&cornerClipping);

		BRect ellipseRect(leftBottomCorner);
		ellipseRect.InsetBy(-1.0, -1.0);
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
	}

	if ((borders & B_RIGHT_BORDER) != 0 && (borders & B_BOTTOM_BORDER) != 0
		&& rightBottomRadius > 0) {
		// draw right bottom rounded corner
		BRect rightBottomCorner(floorf(rect.right - rightBottomRadius + 2.0),
			floorf(rect.bottom - rightBottomRadius + 2.0), floorf(rect.right),
			floorf(rect.bottom));
		clipping.Exclude(rightBottomCorner);

		BRegion cornerClipping(rightBottomCorner);
		view->ConstrainClippingRegion(&cornerClipping);

		BRect ellipseRect(rightBottomCorner);
		ellipseRect.InsetBy(-1.0, -1.0);
		ellipseRect.left = ellipseRect.right - ellipseRect.Width() * 2;
		ellipseRect.top = ellipseRect.bottom - ellipseRect.Height() * 2;

		// draw the frame (again)
		view->SetHighColor(frameShadowColor);
		view->FillEllipse(ellipseRect);

		// draw the bevel and background
		_DrawRoundCornerBackgroundRightBottom(view, rightBottomCorner,
			updateRect, bevelColor3, fillGradient);
	}

	// clip out the corners
	view->ConstrainClippingRegion(&clipping);

	// draw the bevel
	_DrawFrame(view, rect,
		bevelColor2, bevelColor1,
		bevelColor3, bevelColor3,
		cornerColor, cornerColor,
		borders);

	// fill in the background
	view->FillRect(rect, fillGradient);

	// reset clipping constraints
	view->ConstrainClippingRegion(NULL);
}


void
BControlLook::_DrawRoundCornerLeftTop(BView* view, BRect& cornerRect,
	const BRect& updateRect, const rgb_color& background,
	const rgb_color& edgeColor, const rgb_color& frameColor,
	const rgb_color& bevelColor, const BGradientLinear& fillGradient)
{
	_DrawRoundCornerFrameLeftTop(view, cornerRect, updateRect,
		background, edgeColor, frameColor);
	_DrawRoundCornerBackgroundLeftTop(view, cornerRect, updateRect,
		bevelColor, fillGradient);
}


void
BControlLook::_DrawRoundCornerFrameLeftTop(BView* view, BRect& cornerRect,
	const BRect& updateRect, const rgb_color& background,
	const rgb_color& edgeColor, const rgb_color& frameColor)
{
	// constrain clipping region to corner
	BRegion clipping(cornerRect);
	view->ConstrainClippingRegion(&clipping);

	// background
	view->SetHighColor(background);
	view->FillRect(cornerRect);

	// outer edge
	BRect ellipseRect(cornerRect);
	ellipseRect.right = ellipseRect.left + ellipseRect.Width() * 2;
	ellipseRect.bottom = ellipseRect.top + ellipseRect.Height() * 2;

	view->SetHighColor(edgeColor);
	view->FillEllipse(ellipseRect);

	// frame
	ellipseRect.InsetBy(1, 1);
	cornerRect.left++;
	cornerRect.top++;
	view->SetHighColor(frameColor);
	view->FillEllipse(ellipseRect);

	// prepare for bevel
	cornerRect.left++;
	cornerRect.top++;
}


void
BControlLook::_DrawRoundCornerBackgroundLeftTop(BView* view, BRect& cornerRect,
	const BRect& updateRect, const rgb_color& bevelColor,
	const BGradientLinear& fillGradient)
{
	// constrain clipping region to corner
	BRegion clipping(cornerRect);
	view->ConstrainClippingRegion(&clipping);

	BRect ellipseRect(cornerRect);
	ellipseRect.right = ellipseRect.left + ellipseRect.Width() * 2;
	ellipseRect.bottom = ellipseRect.top + ellipseRect.Height() * 2;

	// bevel
	view->SetHighColor(bevelColor);
	view->FillEllipse(ellipseRect);

	// gradient
	ellipseRect.InsetBy(1, 1);
	view->FillEllipse(ellipseRect, fillGradient);
}


void
BControlLook::_DrawRoundCornerRightTop(BView* view, BRect& cornerRect,
	const BRect& updateRect, const rgb_color& background,
	const rgb_color& edgeTopColor, const rgb_color& edgeRightColor,
	const rgb_color& frameTopColor, const rgb_color& frameRightColor,
	const rgb_color& bevelTopColor, const rgb_color& bevelRightColor,
	const BGradientLinear& fillGradient)
{
	_DrawRoundCornerFrameRightTop(view, cornerRect, updateRect,
		background, edgeTopColor, edgeRightColor, frameTopColor,
		frameRightColor);
	_DrawRoundCornerBackgroundRightTop(view, cornerRect, updateRect,
		bevelTopColor, bevelRightColor, fillGradient);
}


void
BControlLook::_DrawRoundCornerFrameRightTop(BView* view, BRect& cornerRect,
	const BRect& updateRect, const rgb_color& background,
	const rgb_color& edgeTopColor, const rgb_color& edgeRightColor,
	const rgb_color& frameTopColor, const rgb_color& frameRightColor)
{
	// constrain clipping region to corner
	BRegion clipping(cornerRect);
	view->ConstrainClippingRegion(&clipping);

	// background
	view->SetHighColor(background);
	view->FillRect(cornerRect);

	// outer edge
	BRect ellipseRect(cornerRect);
	ellipseRect.left = ellipseRect.right - ellipseRect.Width() * 2;
	ellipseRect.bottom = ellipseRect.top + ellipseRect.Height() * 2;

	BGradientLinear gradient;
	gradient.AddColor(edgeTopColor, 0);
	gradient.AddColor(edgeRightColor, 255);
	gradient.SetStart(cornerRect.LeftTop());
	gradient.SetEnd(cornerRect.RightBottom());
	view->FillEllipse(ellipseRect, gradient);

	// frame
	ellipseRect.InsetBy(1, 1);
	cornerRect.right--;
	cornerRect.top++;
	if (frameTopColor == frameRightColor) {
		view->SetHighColor(frameTopColor);
		view->FillEllipse(ellipseRect);
	} else {
		gradient.SetColor(0, frameTopColor);
		gradient.SetColor(1, frameRightColor);
		gradient.SetStart(cornerRect.LeftTop());
		gradient.SetEnd(cornerRect.RightBottom());
		view->FillEllipse(ellipseRect, gradient);
	}

	// prepare for bevel
	cornerRect.right--;
	cornerRect.top++;
}


void
BControlLook::_DrawRoundCornerBackgroundRightTop(BView* view, BRect& cornerRect,
	const BRect& updateRect, const rgb_color& bevelTopColor,
	const rgb_color& bevelRightColor, const BGradientLinear& fillGradient)
{
	// constrain clipping region to corner
	BRegion clipping(cornerRect);
	view->ConstrainClippingRegion(&clipping);

	BRect ellipseRect(cornerRect);
	ellipseRect.left = ellipseRect.right - ellipseRect.Width() * 2;
	ellipseRect.bottom = ellipseRect.top + ellipseRect.Height() * 2;

	// bevel
	BGradientLinear gradient;
	gradient.AddColor(bevelTopColor, 0);
	gradient.AddColor(bevelRightColor, 255);
	gradient.SetStart(cornerRect.LeftTop());
	gradient.SetEnd(cornerRect.RightBottom());
	view->FillEllipse(ellipseRect, gradient);

	// gradient
	ellipseRect.InsetBy(1, 1);
	view->FillEllipse(ellipseRect, fillGradient);
}


void
BControlLook::_DrawRoundCornerLeftBottom(BView* view, BRect& cornerRect,
	const BRect& updateRect, const rgb_color& background,
	const rgb_color& edgeLeftColor, const rgb_color& edgeBottomColor,
	const rgb_color& frameLeftColor, const rgb_color& frameBottomColor,
	const rgb_color& bevelLeftColor, const rgb_color& bevelBottomColor,
	const BGradientLinear& fillGradient)
{
	_DrawRoundCornerFrameLeftBottom(view, cornerRect, updateRect,
		background, edgeLeftColor, edgeBottomColor, frameLeftColor,
		frameBottomColor);
	_DrawRoundCornerBackgroundLeftBottom(view, cornerRect, updateRect,
		bevelLeftColor, bevelBottomColor, fillGradient);
}


void
BControlLook::_DrawRoundCornerFrameLeftBottom(BView* view, BRect& cornerRect,
	const BRect& updateRect, const rgb_color& background,
	const rgb_color& edgeLeftColor, const rgb_color& edgeBottomColor,
	const rgb_color& frameLeftColor, const rgb_color& frameBottomColor)
{
	// constrain clipping region to corner
	BRegion clipping(cornerRect);
	view->ConstrainClippingRegion(&clipping);

	// background
	view->SetHighColor(background);
	view->FillRect(cornerRect);

	// outer edge
	BRect ellipseRect(cornerRect);
	ellipseRect.right = ellipseRect.left + ellipseRect.Width() * 2;
	ellipseRect.top = ellipseRect.bottom - ellipseRect.Height() * 2;

	BGradientLinear gradient;
	gradient.AddColor(edgeLeftColor, 0);
	gradient.AddColor(edgeBottomColor, 255);
	gradient.SetStart(cornerRect.LeftTop());
	gradient.SetEnd(cornerRect.RightBottom());
	view->FillEllipse(ellipseRect, gradient);

	// frame
	ellipseRect.InsetBy(1, 1);
	cornerRect.left++;
	cornerRect.bottom--;
	if (frameLeftColor == frameBottomColor) {
		view->SetHighColor(frameLeftColor);
		view->FillEllipse(ellipseRect);
	} else {
		gradient.SetColor(0, frameLeftColor);
		gradient.SetColor(1, frameBottomColor);
		gradient.SetStart(cornerRect.LeftTop());
		gradient.SetEnd(cornerRect.RightBottom());
		view->FillEllipse(ellipseRect, gradient);
	}

	// prepare for bevel
	cornerRect.left++;
	cornerRect.bottom--;
}


void
BControlLook::_DrawRoundCornerBackgroundLeftBottom(BView* view, BRect& cornerRect,
	const BRect& updateRect, const rgb_color& bevelLeftColor,
	const rgb_color& bevelBottomColor, const BGradientLinear& fillGradient)
{
	// constrain clipping region to corner
	BRegion clipping(cornerRect);
	view->ConstrainClippingRegion(&clipping);

	BRect ellipseRect(cornerRect);
	ellipseRect.right = ellipseRect.left + ellipseRect.Width() * 2;
	ellipseRect.top = ellipseRect.bottom - ellipseRect.Height() * 2;

	// bevel
	BGradientLinear gradient;
	gradient.AddColor(bevelLeftColor, 0);
	gradient.AddColor(bevelBottomColor, 255);
	gradient.SetStart(cornerRect.LeftTop());
	gradient.SetEnd(cornerRect.RightBottom());
	view->FillEllipse(ellipseRect, gradient);

	// gradient
	ellipseRect.InsetBy(1, 1);
	view->FillEllipse(ellipseRect, fillGradient);

	view->PopState();
}


void
BControlLook::_DrawRoundCornerRightBottom(BView* view, BRect& cornerRect,
	const BRect& updateRect, const rgb_color& background,
	const rgb_color& edgeColor, const rgb_color& frameColor,
	const rgb_color& bevelColor, const BGradientLinear& fillGradient)
{
	_DrawRoundCornerFrameRightBottom(view, cornerRect, updateRect,
		background, edgeColor, frameColor);
	_DrawRoundCornerBackgroundRightBottom(view, cornerRect, updateRect,
		bevelColor, fillGradient);
}


void
BControlLook::_DrawRoundCornerFrameRightBottom(BView* view, BRect& cornerRect,
	const BRect& updateRect, const rgb_color& background,
	const rgb_color& edgeColor, const rgb_color& frameColor)
{
	// constrain clipping region to corner
	BRegion clipping(cornerRect);
	view->ConstrainClippingRegion(&clipping);

	// background
	view->SetHighColor(background);
	view->FillRect(cornerRect);

	// outer edge
	BRect ellipseRect(cornerRect);
	ellipseRect.left = ellipseRect.right - ellipseRect.Width() * 2;
	ellipseRect.top = ellipseRect.bottom - ellipseRect.Height() * 2;

	view->SetHighColor(edgeColor);
	view->FillEllipse(ellipseRect);

	// frame
	ellipseRect.InsetBy(1, 1);
	cornerRect.right--;
	cornerRect.bottom++;
	view->SetHighColor(frameColor);
	view->FillEllipse(ellipseRect);

	// prepare for bevel
	cornerRect.left++;
	cornerRect.bottom--;
}


void
BControlLook::_DrawRoundCornerBackgroundRightBottom(BView* view,
	BRect& cornerRect, const BRect& updateRect, const rgb_color& bevelColor,
	const BGradientLinear& fillGradient)
{
	// constrain clipping region to corner
	BRegion clipping(cornerRect);
	view->ConstrainClippingRegion(&clipping);

	BRect ellipseRect(cornerRect);
	ellipseRect.left = ellipseRect.right - ellipseRect.Width() * 2;
	ellipseRect.top = ellipseRect.bottom - ellipseRect.Height() * 2;

	// bevel
	view->SetHighColor(bevelColor);
	view->FillEllipse(ellipseRect);

	// gradient
	ellipseRect.InsetBy(1, 1);
	view->FillEllipse(ellipseRect, fillGradient);
}


void
BControlLook::_DrawRoundBarCorner(BView* view, BRect& rect,
	const BRect& updateRect,
	const rgb_color& edgeLightColor, const rgb_color& edgeShadowColor,
	const rgb_color& frameLightColor, const rgb_color& frameShadowColor,
	const rgb_color& fillLightColor, const rgb_color& fillShadowColor,
	float leftInset, float topInset, float rightInset, float bottomInset,
	enum orientation orientation)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	BGradientLinear gradient;
	gradient.AddColor(edgeShadowColor, 0);
	gradient.AddColor(edgeLightColor, 255);
	gradient.SetStart(rect.LeftTop());
	if (orientation == B_HORIZONTAL)
		gradient.SetEnd(rect.LeftBottom());
	else
		gradient.SetEnd(rect.RightTop());

	view->FillEllipse(rect, gradient);

	rect.left += leftInset;
	rect.top += topInset;
	rect.right += rightInset;
	rect.bottom += bottomInset;

	gradient.MakeEmpty();
	gradient.AddColor(frameShadowColor, 0);
	gradient.AddColor(frameLightColor, 255);
	gradient.SetStart(rect.LeftTop());
	if (orientation == B_HORIZONTAL)
		gradient.SetEnd(rect.LeftBottom());
	else
		gradient.SetEnd(rect.RightTop());

	view->FillEllipse(rect, gradient);

	rect.left += leftInset;
	rect.top += topInset;
	rect.right += rightInset;
	rect.bottom += bottomInset;

	gradient.MakeEmpty();
	gradient.AddColor(fillShadowColor, 0);
	gradient.AddColor(fillLightColor, 255);
	gradient.SetStart(rect.LeftTop());
	if (orientation == B_HORIZONTAL)
		gradient.SetEnd(rect.LeftBottom());
	else
		gradient.SetEnd(rect.RightTop());

	view->FillEllipse(rect, gradient);
}


rgb_color
BControlLook::_EdgeLightColor(const rgb_color& base, float contrast,
	float brightness, uint32 flags)
{
	rgb_color edgeLightColor;

	if ((flags & B_BLEND_FRAME) != 0) {
		uint8 alpha = uint8(20 * contrast);
		uint32 white = uint8(255 * brightness);

		edgeLightColor = (rgb_color){ white, white, white, alpha };
	} else {
		// colors
		float tintLight = kEdgeBevelLightTint;

		if (contrast == 0.0)
			tintLight = B_NO_TINT;
		else if (contrast != 1.0)
			tintLight = B_NO_TINT + (tintLight - B_NO_TINT) * contrast;

		edgeLightColor = tint_color(base, tintLight);

		if (brightness < 1.0) {
			edgeLightColor.red = uint8(edgeLightColor.red * brightness);
			edgeLightColor.green = uint8(edgeLightColor.green * brightness);
			edgeLightColor.blue = uint8(edgeLightColor.blue * brightness);
		}
	}

	return edgeLightColor;
}


rgb_color
BControlLook::_EdgeShadowColor(const rgb_color& base, float contrast,
	float brightness, uint32 flags)
{
	rgb_color edgeShadowColor;

	if ((flags & B_BLEND_FRAME) != 0) {
		uint8 alpha = uint8(20 * contrast);
		edgeShadowColor = (rgb_color){ 0, 0, 0, alpha };
	} else {
		float tintShadow = kEdgeBevelShadowTint;

		if (contrast == 0.0)
			tintShadow = B_NO_TINT;
		else if (contrast != 1.0)
			tintShadow = B_NO_TINT + (tintShadow - B_NO_TINT) * contrast;

		edgeShadowColor = tint_color(base, tintShadow);

		if (brightness < 1.0) {
			edgeShadowColor.red = uint8(edgeShadowColor.red * brightness);
			edgeShadowColor.green = uint8(edgeShadowColor.green * brightness);
			edgeShadowColor.blue = uint8(edgeShadowColor.blue * brightness);
		}
	}

	return edgeShadowColor;
}


rgb_color
BControlLook::_FrameLightColor(const rgb_color& base, uint32 flags)
{
	if ((flags & B_FOCUSED) != 0)
		return ui_color(B_KEYBOARD_NAVIGATION_COLOR);

	if ((flags & B_ACTIVATED) != 0)
		return _FrameShadowColor(base, flags & ~B_ACTIVATED);

	rgb_color frameLightColor;

	if ((flags & B_DISABLED) != 0) {
		// TODO: B_BLEND_FRAME
		frameLightColor = tint_color(base, 1.145);

		if ((flags & B_DEFAULT_BUTTON) != 0)
			frameLightColor = tint_color(frameLightColor, 1.14);
	} else {
		if ((flags & B_BLEND_FRAME) != 0)
			frameLightColor = (rgb_color){ 0, 0, 0, 75 };
		else
			frameLightColor = tint_color(base, 1.33);

		if ((flags & B_DEFAULT_BUTTON) != 0)
			frameLightColor = tint_color(frameLightColor, 1.35);
	}

	return frameLightColor;
}


rgb_color
BControlLook::_FrameShadowColor(const rgb_color& base, uint32 flags)
{
	if ((flags & B_FOCUSED) != 0)
		return ui_color(B_KEYBOARD_NAVIGATION_COLOR);

	if ((flags & B_ACTIVATED) != 0)
		return _FrameLightColor(base, flags & ~B_ACTIVATED);

	rgb_color frameShadowColor;

	if ((flags & B_DISABLED) != 0) {
		// TODO: B_BLEND_FRAME
		frameShadowColor = tint_color(base, 1.24);

		if ((flags & B_DEFAULT_BUTTON) != 0) {
			frameShadowColor = tint_color(base, 1.145);
			frameShadowColor = tint_color(frameShadowColor, 1.12);
		}
	} else {
		if ((flags & B_DEFAULT_BUTTON) != 0) {
			if ((flags & B_BLEND_FRAME) != 0)
				frameShadowColor = (rgb_color){ 0, 0, 0, 75 };
			else
				frameShadowColor = tint_color(base, 1.33);

			frameShadowColor = tint_color(frameShadowColor, 1.5);
		} else {
			if ((flags & B_BLEND_FRAME) != 0)
				frameShadowColor = (rgb_color){ 0, 0, 0, 95 };
			else
				frameShadowColor = tint_color(base, 1.47);
		}
	}

	return frameShadowColor;
}


rgb_color
BControlLook::_BevelLightColor(const rgb_color& base, uint32 flags)
{
	rgb_color bevelLightColor = tint_color(base, 0.2);

	if ((flags & B_DISABLED) != 0)
		bevelLightColor = tint_color(base, B_LIGHTEN_1_TINT);

	if ((flags & B_ACTIVATED) != 0)
		bevelLightColor = tint_color(base, B_DARKEN_1_TINT);

	return bevelLightColor;
}


rgb_color
BControlLook::_BevelShadowColor(const rgb_color& base, uint32 flags)
{
	rgb_color bevelShadowColor = tint_color(base, 1.08);

	if ((flags & B_DISABLED) != 0)
		bevelShadowColor = base;

	if ((flags & B_ACTIVATED) != 0)
		bevelShadowColor = tint_color(base, B_DARKEN_1_TINT);

	return bevelShadowColor;
}


void
BControlLook::_FillGradient(BView* view, const BRect& rect,
	const rgb_color& base, float topTint, float bottomTint,
	enum orientation orientation)
{
	BGradientLinear gradient;
	_MakeGradient(gradient, rect, base, topTint, bottomTint, orientation);
	view->FillRect(rect, gradient);
}


void
BControlLook::_FillGlossyGradient(BView* view, const BRect& rect,
	const rgb_color& base, float topTint, float middle1Tint,
	float middle2Tint, float bottomTint, enum orientation orientation)
{
	BGradientLinear gradient;
	_MakeGlossyGradient(gradient, rect, base, topTint, middle1Tint,
		middle2Tint, bottomTint, orientation);
	view->FillRect(rect, gradient);
}


void
BControlLook::_MakeGradient(BGradientLinear& gradient, const BRect& rect,
	const rgb_color& base, float topTint, float bottomTint,
	enum orientation orientation) const
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
BControlLook::_MakeGlossyGradient(BGradientLinear& gradient, const BRect& rect,
	const rgb_color& base, float topTint, float middle1Tint,
	float middle2Tint, float bottomTint,
	enum orientation orientation) const
{
	gradient.AddColor(tint_color(base, topTint), 0);
	gradient.AddColor(tint_color(base, middle1Tint), 132);
	gradient.AddColor(tint_color(base, middle2Tint), 136);
	gradient.AddColor(tint_color(base, bottomTint), 255);
	gradient.SetStart(rect.LeftTop());
	if (orientation == B_HORIZONTAL)
		gradient.SetEnd(rect.LeftBottom());
	else
		gradient.SetEnd(rect.RightTop());
}


void
BControlLook::_MakeButtonGradient(BGradientLinear& gradient, BRect& rect,
	const rgb_color& base, uint32 flags, enum orientation orientation) const
{
	float topTint = 0.49;
	float middleTint1 = 0.62;
	float middleTint2 = 0.76;
	float bottomTint = 0.90;

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
		static const float kHoverTintFactor = 0.85;
		topTint *= kHoverTintFactor;
		middleTint1 *= kHoverTintFactor;
		middleTint2 *= kHoverTintFactor;
		bottomTint *= kHoverTintFactor;
	}

	if ((flags & B_ACTIVATED) != 0) {
		_MakeGradient(gradient, rect, base, topTint, bottomTint, orientation);
	} else {
		_MakeGlossyGradient(gradient, rect, base, topTint, middleTint1,
			middleTint2, bottomTint, orientation);
	}
}



bool
BControlLook::_RadioButtonAndCheckBoxMarkColor(const rgb_color& base,
	rgb_color& color, uint32 flags) const
{
	if ((flags & (B_ACTIVATED | B_CLICKED)) == 0) {
		// no mark to be drawn at all
		return false;
	}

	color = ui_color(B_CONTROL_MARK_COLOR);

	float mix = 1.0;

	if ((flags & B_DISABLED) != 0) {
		// activated, but disabled
		mix = 0.4;
	} else if ((flags & B_CLICKED) != 0) {
		if ((flags & B_ACTIVATED) != 0) {
			// loosing activation
			mix = 0.7;
		} else {
			// becoming activated
			mix = 0.3;
		}
	} else {
		// simply activated
	}

	color.red = uint8(color.red * mix + base.red * (1.0 - mix));
	color.green = uint8(color.green * mix + base.green * (1.0 - mix));
	color.blue = uint8(color.blue * mix + base.blue * (1.0 - mix));

	return true;
}


// NOTE: May come from a add-on in the future. Initialized in
// InterfaceDefs.cpp
BControlLook* be_control_look = NULL;


} // namespace BPrivate
