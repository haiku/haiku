/*
 * Copyright 2007-2016 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood <leavengood@gmail.com>
 *		John Scipione <jscipione@gmail.com>
 *		Joseph Groover <looncraz@looncraz.net>
 *		Brian Hill <supernova@warpmail.net>
 */


#include "StripeView.h"

#include <LayoutUtils.h>


static const float kTopOffset = 10.0f;


StripeView::StripeView(BBitmap& icon)
	:
	BView("StripeView", B_WILL_DRAW),
	fIcon(icon),
	fIconSize(0.0),
	fWidth(0.0)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	if (fIcon.IsValid()) {
		fIconSize = fIcon.Bounds().Width();
		fWidth = 2 * fIconSize + 2.0f;
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
	stripeRect.right = fIconSize;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmapAsync(&fIcon, BPoint(fIconSize / 2.0f, kTopOffset));
}


BSize
StripeView::PreferredSize()
{
	return BSize(fWidth, B_SIZE_UNSET);
}


void
StripeView::GetPreferredSize(float* _width, float* _height)
{
	if (_width != NULL)
		*_width = fWidth;

	if (_height != NULL)
		*_height = fIconSize + 2 * kTopOffset;
}


BSize
StripeView::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(fWidth, B_SIZE_UNLIMITED));
}
