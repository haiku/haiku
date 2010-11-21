/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include "ConfigView.h"
#include "WebPTranslator.h"

#include <CheckBox.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <StringView.h>

#include <stdio.h>
#include <string.h>


ConfigView::ConfigView(uint32 flags)
	: BView("WebPTranslator Settings", flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BStringView* title = new BStringView("title", "WebP Images");
	title->SetFont(be_bold_font);

	char versionString[256];
	sprintf(versionString, "Version %d.%d.%d, %s",
		int(B_TRANSLATION_MAJOR_VERSION(WEBP_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(WEBP_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(WEBP_TRANSLATOR_VERSION)),
		__DATE__);
	BStringView* version = new BStringView("version", versionString);

	BStringView* copyright = new BStringView("copyright",
		B_UTF8_COPYRIGHT "2010 Haiku Inc.");

	BStringView* copyright2 = new BStringView("copyright2",
		"Based on libwebp v0.1");

	BStringView* copyright3 = new BStringView("copyright3",
		B_UTF8_COPYRIGHT "2010 Google Inc.");

	// Build the layout
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 7)
		.Add(title)
		.AddGlue()
		.Add(version)
		.Add(copyright)
		.AddGlue()
		.Add(copyright2)
		.Add(copyright3)
		.AddGlue()
		.SetInsets(5, 5, 5, 5)
	);

	BFont font;
	GetFont(&font);
	SetExplicitPreferredSize(BSize((font.Size() * 233)/12, (font.Size() * 200)/12));
}


ConfigView::~ConfigView()
{
}

