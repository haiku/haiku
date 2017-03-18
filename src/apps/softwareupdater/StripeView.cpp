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


StripeView::StripeView(BBitmap* icon)
	:
	BView("StripeView", B_WILL_DRAW),
	fIcon(icon),
	fWidth(0.0),
	fStripeWidth(0.0)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	if (icon != NULL) {
		fStripeWidth = icon->Bounds().Width();
		fWidth = 2 * fStripeWidth + 2.0f;
	}
}


void
StripeView::Draw(BRect updateRect)
{
	if (fIcon == NULL)
		return;

	SetHighColor(ViewColor());
	FillRect(updateRect);

	BRect stripeRect = Bounds();
	stripeRect.right = fStripeWidth;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmapAsync(fIcon, BPoint(fStripeWidth / 2.0f, 10.0f));
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
		*_height = fStripeWidth + 20.0f;
}


BSize
StripeView::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(fWidth, B_SIZE_UNLIMITED));
}
