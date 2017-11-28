/*
 * Copyright 2007-2016 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood <leavengood@gmail.com>
 *		John Scipione <jscipione@gmail.com>
 *		Joseph Groover <looncraz@looncraz.net>
 *		Brian Hill <supernova@tycho.email>
 */


#include "StripeView.h"

#include <LayoutUtils.h>


static const float kTopOffset = 10.0f;
static const int kIconStripeWidth = 30;


StripeView::StripeView(BBitmap& icon)
	:
	BView("StripeView", B_WILL_DRAW),
	fIcon(icon),
	fIconSize(0.0),
	fPreferredWidth(0.0),
	fPreferredHeight(0.0)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	if (fIcon.IsValid()) {
		fIconSize = fIcon.Bounds().Width();
		// Use the same scaling as a BAlert
		int32 scale = icon_layout_scale();
		fPreferredWidth = 18 * scale + fIcon.Bounds().Width();
		fPreferredHeight = 6 * scale + fIcon.Bounds().Height();
	}
}


void
StripeView::Draw(BRect updateRect)
{
	if (fIconSize == 0)
		return;

	SetHighColor(ViewColor());
	FillRect(updateRect);

	BRect stripeRect = Bounds();
	int32 iconLayoutScale = icon_layout_scale();
	stripeRect.right = kIconStripeWidth * iconLayoutScale;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmapAsync(&fIcon, BPoint(stripeRect.right - (fIconSize / 2.0),
		6 * iconLayoutScale));
}


BSize
StripeView::PreferredSize()
{
	return BSize(fPreferredWidth, B_SIZE_UNSET);
}


void
StripeView::GetPreferredSize(float* _width, float* _height)
{
	if (_width != NULL)
		*_width = fPreferredWidth;

	if (_height != NULL)
		*_height = fPreferredHeight;
}


BSize
StripeView::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(fPreferredWidth, B_SIZE_UNLIMITED));
}
