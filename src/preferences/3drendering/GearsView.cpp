/*
 * Copyright 2012 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		John Scipione <jscipione@gmail.com>
 */


#include "GearsView.h"

#include <Catalog.h>
#include <Locale.h>
#include <Size.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "GearsView"


static const float kStripeWidth = 30.0;

GearsView::GearsView()
	:
	BView("GearsView", B_WILL_DRAW),
	fGears(NULL)
{
	SetExplicitPreferredSize(BSize(64 + 5, B_SIZE_UNLIMITED));
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	_InitGearsBitmap();
}


GearsView::~GearsView()
{
	delete fGears;
}


void
GearsView::_InitGearsBitmap()
{
	fGears = BTranslationUtils::GetBitmap(B_PNG_FORMAT, "gears_64.png");
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
	DrawBitmapAsync(fGears, BPoint(5, 70));
}
