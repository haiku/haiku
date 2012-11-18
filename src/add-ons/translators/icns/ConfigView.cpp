/*
 * Copyright 2012, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */ 

#include "ConfigView.h"
#include "ICNSTranslator.h"

#include <StringView.h>
#include <SpaceLayoutItem.h>
#include <ControlLook.h>

#include <stdio.h>

extern "C" {
#include <openjpeg.h>
};


ConfigView::ConfigView(TranslatorSettings *settings)
	: BGroupView("ICNSTranslator Settings", B_VERTICAL, 0)
{
	fSettings = settings;
	BAlignment leftAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET);

	BStringView *stringView = new BStringView("title", "Apple icon translator");
	stringView->SetFont(be_bold_font);
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	float spacing = be_control_look->DefaultItemSpacing();
	AddChild(BSpaceLayoutItem::CreateVerticalStrut(spacing));

	char version[256];
	sprintf(version, "Version %d.%d.%d, %s",
		int(B_TRANSLATION_MAJOR_VERSION(ICNS_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(ICNS_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(ICNS_TRANSLATOR_VERSION)),
		__DATE__);
	stringView = new BStringView("version", version);
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	stringView = new BStringView("copyright",
		B_UTF8_COPYRIGHT "2005-2006 Haiku Inc.");
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	stringView = new BStringView("my_copyright",
		B_UTF8_COPYRIGHT "2012 Gerasim Troeglazov <3dEyes@gmail.com>.");
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	AddChild(BSpaceLayoutItem::CreateVerticalStrut(spacing));

	stringView = new BStringView("support_sizes",
		"Valid sizes: 16, 32, 48, 128, 256, 512, 1024");
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	stringView = new BStringView("support_colors",
		"Valid colors: RGB32, RGBA32");
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);	

	AddChild(BSpaceLayoutItem::CreateVerticalStrut(spacing));

	BString copyrightText;
	copyrightText << "libicns v0.8.1\n"
		<< B_UTF8_COPYRIGHT "2001-2012 Mathew Eis <mathew@eisbox.net>\n\n"
		<< "OpenJPEG " << opj_version() << "\n"
		<< B_UTF8_COPYRIGHT "2002-2012, Communications and Remote Sensing "
		<< "Laboratory, Universite catholique de Louvain (UCL), Belgium";
	
	fCopyrightView = new BTextView("CopyrightLibs");
	fCopyrightView->SetExplicitAlignment(leftAlignment);
	fCopyrightView->MakeEditable(false);
	fCopyrightView->MakeResizable(true);
	fCopyrightView->SetWordWrap(true);
	fCopyrightView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fCopyrightView->SetText(copyrightText.String());
	fCopyrightView->SetExplicitMinSize(BSize(300,200));

	BFont font;
	font.SetSize(font.Size() * 0.9);
	fCopyrightView->SetFontAndColor(&font, B_FONT_SIZE, NULL);
	AddChild(fCopyrightView);

	fCopyrightView->SetExplicitAlignment(leftAlignment);
	
	AddChild(BSpaceLayoutItem::CreateGlue());
	GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

	SetExplicitPreferredSize(GroupLayout()->MinSize());		
}


ConfigView::~ConfigView()
{
	fSettings->Release();
}

