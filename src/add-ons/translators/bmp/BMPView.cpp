/*
 * Copyright 2002-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*! A view with information about the BMPTranslator. */


#include "BMPView.h"
#include "BMPTranslator.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <StringView.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BMPView"


BMPView::BMPView(const BRect &frame, const char *name, uint32 resizeMode,
		uint32 flags, TranslatorSettings *settings)
	: BView(frame, name, resizeMode, flags)
{
	fSettings = settings;
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BStringView *titleView = new BStringView("title",
		B_TRANSLATE("BMP image translator"));
	titleView->SetFont(be_bold_font);

	char version[256];
	snprintf(version, sizeof(version), B_TRANSLATE("Version %d.%d.%d, %s"),
		int(B_TRANSLATION_MAJOR_VERSION(BMP_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(BMP_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(BMP_TRANSLATOR_VERSION)),
		__DATE__);
	BStringView *versionView = new BStringView("version", version);
	BStringView *copyrightView = new BStringView("Copyright", B_UTF8_COPYRIGHT "2002-2010 Haiku Inc.");

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleView)
		.Add(versionView)
		.Add(copyrightView)
		.AddGlue();

	BFont font;
	GetFont(&font);
	SetExplicitPreferredSize(BSize((font.Size() * 300)/12,
		(font.Size() * 100)/12));
}


BMPView::~BMPView()
{
	fSettings->Release();
}

