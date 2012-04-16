/*
 * Copyright 2002-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*! A view with information about the STXTTranslator. */


#include "STXTView.h"
#include "STXTTranslator.h"

#include <Catalog.h>
#include <StringView.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "STXTView"


STXTView::STXTView(const BRect &frame, const char *name, uint32 resizeMode,
		uint32 flags, TranslatorSettings *settings)
	: BView(frame, name, resizeMode, flags)
{
	fSettings = settings;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	font_height fontHeight;
	be_bold_font->GetHeight(&fontHeight);
	float height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	BRect rect(10, 10, 200, 10 + height);
	BStringView *stringView = new BStringView(rect, "title",
		B_TRANSLATE("StyledEdit files translator"));
	stringView->SetFont(be_bold_font);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	float maxWidth = stringView->Bounds().Width();

	rect.OffsetBy(0, height + 10);
	char version[256];
	snprintf(version, sizeof(version), "Version %d.%d.%d, %s",
		int(B_TRANSLATION_MAJOR_VERSION(STXT_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(STXT_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(STXT_TRANSLATOR_VERSION)),
		__DATE__);
	stringView = new BStringView(rect, "version", version);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	if (stringView->Bounds().Width() > maxWidth)
		maxWidth = stringView->Bounds().Width();

	GetFontHeight(&fontHeight);
	height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	rect.OffsetBy(0, height + 5);
	stringView = new BStringView(rect, "Copyright",
		B_UTF8_COPYRIGHT "2002-2006 Haiku Inc.");
	stringView->ResizeToPreferred();
	AddChild(stringView);

	if (maxWidth + 20 > Bounds().Width())
		ResizeTo(maxWidth + 20, Bounds().Height());
}


STXTView::~STXTView()
{
	fSettings->Release();
}
