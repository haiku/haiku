/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */

#include "HVIFView.h"
#include "HVIFTranslator.h"

#include <StringView.h>

#include <stdio.h>


HVIFView::HVIFView(const BRect &frame, const char *name, uint32 resizeMode,
	uint32 flags)
	:	BView(frame, name, resizeMode, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	font_height fontHeight;
	be_bold_font->GetHeight(&fontHeight);
	float height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	BRect rect(10, 10, 200, 10 + height);
	BStringView *stringView = new BStringView(rect, "title",
		"Native Haiku Icon Format Translator");
	stringView->SetFont(be_bold_font);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect.OffsetBy(0, height + 10);
	char version[256];
	snprintf(version, sizeof(version), "Version %d.%d.%d, %s",
		int(B_TRANSLATION_MAJOR_VERSION(HVIF_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(HVIF_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(HVIF_TRANSLATOR_VERSION)),
		__DATE__);
	stringView = new BStringView(rect, "version", version);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	GetFontHeight(&fontHeight);
	height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	rect.OffsetBy(0, height + 5);
	stringView = new BStringView(rect, "copyright",
		B_UTF8_COPYRIGHT"2009 Haiku Inc.");
	stringView->ResizeToPreferred();
	AddChild(stringView);
}
