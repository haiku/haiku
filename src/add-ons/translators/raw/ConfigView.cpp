/*
 * Copyright 2005-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2009, Maxime Simon, maxime.simon@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ConfigView.h"
#include "RAWTranslator.h"

#include <StringView.h>
#include <CheckBox.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>

#include <stdio.h>
#include <string.h>


ConfigView::ConfigView(uint32 flags)
	: BView("RAWTranslator Settings", flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BStringView *fTitle = new BStringView("title", "RAW Images");
	fTitle->SetFont(be_bold_font);

	char version[256];
	sprintf(version, "Version %d.%d.%d, %s",
		int(B_TRANSLATION_MAJOR_VERSION(RAW_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(RAW_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(RAW_TRANSLATOR_VERSION)),
		__DATE__);
	BStringView *fVersion = new BStringView("version", version);

	BStringView *fCopyright = new BStringView("copyright",
		B_UTF8_COPYRIGHT "2007-2009 Haiku Inc.");

	BStringView *fCopyright2 = new BStringView("copyright2",
		"Based on Dave Coffin's dcraw 8.63");

	BStringView *fCopyright3 = new BStringView("copyright3",
		B_UTF8_COPYRIGHT "1997-2007 Dave Coffin");

	// Build the layout
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 7)
		.Add(fTitle)
		.AddGlue()
		.Add(fVersion)
		.Add(fCopyright)
		.AddGlue()
		.Add(fCopyright2)
		.Add(fCopyright3)
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

