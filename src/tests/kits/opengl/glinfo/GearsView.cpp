/*
 * Copyright 2012 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione <jscipione@gmail.com>
 */


#include "GearsView.h"

#include <Bitmap.h>
#include <Locale.h>
#include <Size.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>


static const float kStripeWidth = 30.0;

GearsView::GearsView()
	:
	BView("GearsView", B_WILL_DRAW),
	fGears(NULL)
{
	SetExplicitMinSize(BSize(64.0 + 10.0, B_SIZE_UNSET));
	SetExplicitPreferredSize(BSize(64.0 + 10.0, B_SIZE_UNLIMITED));
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	_InitGearsBitmap();
}


GearsView::~GearsView()
{
	delete fGears;
}


void
GearsView::Draw(BRect updateRect)
{
	SetHighColor(ViewColor());
	FillRect(updateRect);

	BRect stripeRect = Bounds();
	stripeRect.right = kStripeWidth;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);

	if (fGears == NULL)
		return;

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmapAsync(fGears, BPoint(5.0, 10.0));
}


//	#pragma mark -


void
GearsView::_InitGearsBitmap()
{
	fGears = BTranslationUtils::GetBitmap(B_PNG_FORMAT, "gears_64.png");
}
