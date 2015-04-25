/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ConfigView.h"
#include "ICOTranslator.h"

#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <StringView.h>

#include <stdio.h>
#include <string.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


ConfigView::ConfigView()
	:
	BGroupView(B_TRANSLATE("ICOTranslator Settings"), B_VERTICAL, 0)
{
	BStringView* titleView = new BStringView("title",
		B_TRANSLATE("Windows icon translator"));
	titleView->SetFont(be_bold_font);

	char version[256];
	sprintf(version, B_TRANSLATE("Version %d.%d.%d, %s"),
		int(B_TRANSLATION_MAJOR_VERSION(ICO_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(ICO_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(ICO_TRANSLATOR_VERSION)),
		__DATE__);

	BStringView* versionView = new BStringView("version", version);

	BStringView *copyrightView = new BStringView("copyright",
		B_UTF8_COPYRIGHT "2005-2006 Haiku Inc.");

	BCheckBox *colorCheckBox = new BCheckBox("color", 
		B_TRANSLATE("Write 32 bit images on true color input"), NULL);

	BCheckBox *sizeCheckBox = new BCheckBox("size",
		B_TRANSLATE("Enforce valid icon sizes"), NULL);
	sizeCheckBox->SetValue(1);

	BStringView* infoView = new BStringView("valid1",
		B_TRANSLATE("Valid icon sizes are 16, 32, or 48"));

	BStringView* info2View = new BStringView("valid2",
		B_TRANSLATE("pixels in either direction."));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleView)
		.Add(versionView)
		.Add(copyrightView)
		.AddGlue()
		.Add(colorCheckBox)
		.Add(sizeCheckBox)
		.Add(infoView)
		.Add(info2View)
		.AddGlue();

	SetExplicitPreferredSize(GroupLayout()->MinSize());
}


ConfigView::~ConfigView()
{
}

