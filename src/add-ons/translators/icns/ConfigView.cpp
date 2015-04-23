/*
 * Copyright 2012, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */ 

#include "ConfigView.h"
#include "ICNSTranslator.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <StringView.h>
#include <ControlLook.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ICNSConfig"


ConfigView::ConfigView(TranslatorSettings *settings)
	: BGroupView("ICNSTranslator Settings", B_VERTICAL, 0)
{
	fSettings = settings;

	BStringView *titleView = new BStringView("title", B_TRANSLATE("Apple icon translator"));
	titleView->SetFont(be_bold_font);

	char version[256];
	sprintf(version, B_TRANSLATE("Version %d.%d.%d, %s"),
		int(B_TRANSLATION_MAJOR_VERSION(ICNS_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(ICNS_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(ICNS_TRANSLATOR_VERSION)),
		__DATE__);

	BStringView *versionView = new BStringView("version", version);


	BStringView *copyrightView = new BStringView("copyright",
		B_UTF8_COPYRIGHT "2005-2006 Haiku Inc.");

	BStringView *copyright2View = new BStringView("my_copyright",
		B_UTF8_COPYRIGHT "2012 Gerasim Troeglazov <3dEyes@gmail.com>.");

	BStringView *infoView = new BStringView("support_sizes",
		"Valid sizes: 16, 32, 48, 128, 256, 512, 1024");

	BStringView *info2View  = new BStringView("support_colors",
		"Valid colors: RGB32, RGBA32");
	
	BStringView *copyright3View  = new BStringView("copyright3",
		"libicns v0.8.1\n");

	BStringView *copyright4View  = new BStringView("copyright4",
		"2001-2012 Mathew Eis <mathew@eisbox.net>");

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleView)
		.Add(versionView)
		.Add(copyrightView)
		.Add(copyright2View)
		.AddGlue()
		.Add(infoView)
		.Add(info2View)
		.AddGlue()
		.Add(copyright3View)
		.Add(copyright4View);

	SetExplicitPreferredSize(GroupLayout()->MinSize());		
}


ConfigView::~ConfigView()
{
	fSettings->Release();
}

