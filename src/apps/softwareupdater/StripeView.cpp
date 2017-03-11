/*
 * Copyright 2007-2016 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood <leavengood@gmail.com>
 *		John Scipione <jscipione@gmail.com>
 *		Joseph Groover <looncraz@looncraz.net>
 */


#include "StripeView.h"


static const float kStripeWidth = 30.0;


StripeView::StripeView(BBitmap* icon)
	:
	BView("StripeView", B_WILL_DRAW),
	fIcon(icon)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	float width = 0.0f;
	if (icon != NULL)
		width += icon->Bounds().Width() + 32.0f;

	SetExplicitSize(BSize(width, B_SIZE_UNSET));
	SetExplicitPreferredSize(BSize(width, B_SIZE_UNLIMITED));
}


StripeView::~StripeView()
{
}


void
StripeView::Draw(BRect updateRect)
{
	if (fIcon == NULL)
		return;

	SetHighColor(ViewColor());
	FillRect(updateRect);

	BRect stripeRect = Bounds();
	stripeRect.right = kStripeWidth;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmapAsync(fIcon, BPoint(15.0f, 10.0f));
}


void
StripeView::SetIcon(BBitmap* icon)
{
	if (fIcon != NULL)
		delete fIcon;

	fIcon = icon;

	float width = 0.0f;
	if (icon != NULL)
		width += icon->Bounds().Width() + 32.0f;

	SetExplicitSize(BSize(width, B_SIZE_UNSET));
	SetExplicitPreferredSize(BSize(width, B_SIZE_UNLIMITED));
}
