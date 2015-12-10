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
#include <LayoutBuilder.h>
#include <StringView.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "STXTView"


STXTView::STXTView(const BRect &frame, const char *name, uint32 resizeMode,
		uint32 flags, TranslatorSettings *settings)
	: BView(frame, name, resizeMode, flags)
{
	fSettings = settings;
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BStringView *titleView = new BStringView("title",
		B_TRANSLATE("StyledEdit file translator"));
	titleView->SetFont(be_bold_font);

	char version[256];
	snprintf(version, sizeof(version), "Version %d.%d.%d, %s",
		int(B_TRANSLATION_MAJOR_VERSION(STXT_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(STXT_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(STXT_TRANSLATOR_VERSION)),
		__DATE__);
	BStringView *versionView  =  new BStringView("version", version);
	BStringView *copyrightView  = new BStringView("Copyright",
		B_UTF8_COPYRIGHT "2002-2006 Haiku Inc.");

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleView)
		.Add(versionView)
		.Add(copyrightView)
		.AddGlue();
}


STXTView::~STXTView()
{
	fSettings->Release();
}
