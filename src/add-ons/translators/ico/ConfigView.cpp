/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ConfigView.h"
#include "ICOTranslator.h"

#include <StringView.h>
#include <CheckBox.h>

#include <stdio.h>
#include <string.h>


ConfigView::ConfigView(const BRect &frame, uint32 resize, uint32 flags)
	: BView(frame, "ICOTranslator Settings", resize, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	font_height fontHeight;
	be_bold_font->GetHeight(&fontHeight);
	float height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	BRect rect(10, 10, 200, 10 + height);
	BStringView *stringView = new BStringView(rect, "title", "Windows Icon Images");
	stringView->SetFont(be_bold_font);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect.OffsetBy(0, height + 10);
	char version[256];
	sprintf(version, "Version %d.%d.%d, %s",
		int(B_TRANSLATION_MAJOR_VERSION(ICO_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(ICO_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(ICO_TRANSLATOR_VERSION)),
		__DATE__);
	stringView = new BStringView(rect, "version", version);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	GetFontHeight(&fontHeight);
	height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	rect.OffsetBy(0, height + 5);
	stringView = new BStringView(rect, "copyright", B_UTF8_COPYRIGHT "2005-2006 Haiku Inc.");
	stringView->ResizeToPreferred();
	AddChild(stringView);
	
	rect.OffsetBy(0, height + 20);
	BCheckBox *checkBox = new BCheckBox(rect, "color", "Write 32 bit images on true color input", NULL);
	checkBox->ResizeToPreferred();
	AddChild(checkBox);

	rect.OffsetBy(0, height + 10);
	checkBox = new BCheckBox(rect, "size", "Enforce valid icon sizes", NULL);
	checkBox->ResizeToPreferred();
	checkBox->SetValue(1);
	AddChild(checkBox);

	rect.OffsetBy(0, height + 15);
	stringView = new BStringView(rect, "valid1", "Valid icon sizes are 16, 32, or 48");
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect.OffsetBy(0, height + 5);
	stringView = new BStringView(rect, "valid2", "pixel in either direction.");
	stringView->ResizeToPreferred();
	AddChild(stringView);
}


ConfigView::~ConfigView()
{
}

