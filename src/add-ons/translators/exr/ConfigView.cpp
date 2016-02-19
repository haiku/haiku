/*
 * Copyright 2008, Jérôme Duval. All rights reserved.
 * Copyright 2005-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2009, Maxime Simon, maxime.simon@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ConfigView.h"
#include "EXRTranslator.h"

#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <StringView.h>

#include <stdio.h>
#include <string.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


ConfigView::ConfigView(uint32 flags)
	: BView("EXRTranslator Settings", flags)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BStringView *titleView = new BStringView("title",
		B_TRANSLATE("EXR image translator"));
	titleView->SetFont(be_bold_font);

	char version[256];
	sprintf(version, B_TRANSLATE("Version %d.%d.%d, %s"),
		int(B_TRANSLATION_MAJOR_VERSION(EXR_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(EXR_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(EXR_TRANSLATOR_VERSION)),
		__DATE__);
	BStringView *versionView = new BStringView("version", version);

	BStringView *copyrightView = new BStringView("copyright",
		B_UTF8_COPYRIGHT "2008 Haiku Inc.");

	BStringView *copyrightView2 = new BStringView("copyright2",
		B_TRANSLATE("Based on OpenEXR (http://www.openexr.com)"));

	BStringView *copyrightView3 = new BStringView("copyright3",
		B_UTF8_COPYRIGHT "2006, Industrial Light & Magic,");

	BStringView *copyrightView4 = new BStringView("copyright4",
		B_TRANSLATE("a division of Lucasfilm Entertainment Company Ltd"));

	// Build the layout
    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
        .Add(titleView)
		.Add(versionView)
		.Add(copyrightView)
		.AddGlue()
		.Add(copyrightView2)
		.Add(copyrightView3)
		.Add(copyrightView4);

	BFont font;
	GetFont(&font);
	SetExplicitPreferredSize(BSize(font.Size() * 400 / 12,
		font.Size() * 200 / 12));
}
 

ConfigView::~ConfigView()
{
}

