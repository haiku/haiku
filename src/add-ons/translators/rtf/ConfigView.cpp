/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ConfigView.h"
#include "RTFTranslator.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <StringView.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


ConfigView::ConfigView(const BRect &frame, uint32 resize, uint32 flags)
	: BView(frame, B_TRANSLATE("RTF-Translator Settings"), resize, flags)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BStringView *titleView = new BStringView("title",
		B_TRANSLATE("Rich Text Format (RTF) translator"));
	titleView->SetFont(be_bold_font);


	char version[256];
	snprintf(version, sizeof(version), B_TRANSLATE("Version %d.%d.%d, %s"),
		static_cast<int>(B_TRANSLATION_MAJOR_VERSION(RTF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_MINOR_VERSION(RTF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_REVISION_VERSION(
			RTF_TRANSLATOR_VERSION)), __DATE__);
	BStringView *versionView = new BStringView("version", version);
	BStringView *copyrightView = new BStringView(
		"Copyright", B_UTF8_COPYRIGHT "2004-2006 Haiku Inc.");
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleView)
		.Add(versionView)
		.Add(copyrightView)
		.AddGlue();
}


ConfigView::~ConfigView()
{
}

