/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include "LogoView.h"

#include <Bitmap.h>
#include <GroupLayout.h>
#include <Message.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>


LogoView::LogoView()
	:	
	BView("LogoView", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE, NULL),
	fLogo(NULL)
{
	SetViewColor(255, 255, 255);
	fLogo = BTranslationUtils::GetBitmap(B_PNG_FORMAT, "logo.png");
	if (fLogo) {
		SetExplicitMinSize(
			BSize(fLogo->Bounds().Width(), fLogo->Bounds().Height() + 6));
	}
}


LogoView::~LogoView()
{
	delete fLogo;
}


void
LogoView::Draw(BRect update)
{
	if (!fLogo)
		return;
	
	BRect bounds(Bounds());
	BPoint placement;
	placement.x = (bounds.left + bounds.right - fLogo->Bounds().Width()) / 2;
	placement.y = (bounds.top + bounds.bottom - fLogo->Bounds().Height()) / 2;

	DrawBitmap(fLogo, placement);
	rgb_color borderColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_3_TINT);
	SetHighColor(borderColor);
	StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
}


void
LogoView::GetPreferredSize(float* _width, float* _height)
{
	float width = 0.0;
	float height = 0.0;
	if (fLogo) {
		width = fLogo->Bounds().Width();
		height = fLogo->Bounds().Height();
	}
	if (_width)
		*_width = width;
	if (_height)
		*_height = height;
}
