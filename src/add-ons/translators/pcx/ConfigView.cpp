/*
 * Copyright 2008, Jérôme Duval, korli@users.berlios.de. All rights reserved.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ConfigView.h"
#include "PCXTranslator.h"

#include <Catalog.h>
#include <StringView.h>
#include <CheckBox.h>

#include <stdio.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


ConfigView::ConfigView(const BRect &frame, uint32 resize, uint32 flags)
	: BView(frame, B_TRANSLATE("PCXTranslator Settings"), resize, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	font_height fontHeight;
	be_bold_font->GetHeight(&fontHeight);
	float height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	BRect rect(10, 10, 200, 10 + height);
	BStringView *stringView = new BStringView(rect, "title", 
		B_TRANSLATE("PCX images"));
	stringView->SetFont(be_bold_font);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect.OffsetBy(0, height + 10);
	char version[256];
	sprintf(version, B_TRANSLATE("Version %d.%d.%d, %s"),
		int(B_TRANSLATION_MAJOR_VERSION(PCX_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(PCX_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(PCX_TRANSLATOR_VERSION)),
		__DATE__);
	stringView = new BStringView(rect, "version", version);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	GetFontHeight(&fontHeight);
	height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	rect.OffsetBy(0, height + 5);
	stringView = new BStringView(rect, "copyright", B_UTF8_COPYRIGHT "2008 Haiku Inc.");
	stringView->ResizeToPreferred();
	AddChild(stringView);
}


ConfigView::~ConfigView()
{
}

