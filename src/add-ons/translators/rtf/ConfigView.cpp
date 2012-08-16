/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ConfigView.h"
#include "RTFTranslator.h"

#include <Catalog.h>
#include <StringView.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


ConfigView::ConfigView(const BRect &frame, uint32 resize, uint32 flags)
	: BView(frame, B_TRANSLATE("RTF-Translator Settings"), resize, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	font_height fontHeight;
	be_bold_font->GetHeight(&fontHeight);
	float height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	BRect rect(10, 10, 200, 10 + height);
	BStringView *stringView = new BStringView(rect, "title", 
		B_TRANSLATE("Rich Text Format (RTF) translator"));
	stringView->SetFont(be_bold_font);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	float maxWidth = stringView->Bounds().Width();

	rect.OffsetBy(0, height + 10);
	char version[256];
	snprintf(version, sizeof(version), B_TRANSLATE("Version %d.%d.%d, %s"),
		static_cast<int>(B_TRANSLATION_MAJOR_VERSION(RTF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_MINOR_VERSION(RTF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_REVISION_VERSION(
			RTF_TRANSLATOR_VERSION)), __DATE__);
	stringView = new BStringView(rect, "version", version);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	if (stringView->Bounds().Width() > maxWidth)
		maxWidth = stringView->Bounds().Width();

	GetFontHeight(&fontHeight);
	height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	rect.OffsetBy(0, height + 5);
	stringView = new BStringView(rect, 
		"Copyright", B_UTF8_COPYRIGHT "2004-2006 Haiku Inc.");
	stringView->ResizeToPreferred();
	AddChild(stringView);

	if (maxWidth + 20 > Bounds().Width())
		ResizeTo(maxWidth + 20, Bounds().Height());
}


ConfigView::~ConfigView()
{
}

