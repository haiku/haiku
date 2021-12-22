/*****************************************************************************/
// TIFFView
// Written by Michael Wilber, Haiku Translation Kit Team
// Picking the compression method added by Stephan Aßmus, <stippi@yellowbites.com>
// Use of Layout API added by Maxime Simon, maxime.simon@gmail.com, 2009.
//
// TIFFView.cpp
//
// This BView based object displays information about the TIFFTranslator.
//
//
// Copyright (c) 2003 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/


#include "TIFFView.h"

#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include "tiff.h"
#include "tiffvers.h"

#include "TIFFTranslator.h"
#include "TranslatorSettings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TIFFView"

// add_menu_item
void
add_menu_item(BMenu* menu,
			  uint32 compression,
			  const char* label,
			  uint32 currentCompression)
{
	// COMPRESSION_NONE
	BMessage* message = new BMessage(TIFFView::MSG_COMPRESSION_CHANGED);
	message->AddInt32("value", compression);
	BMenuItem* item = new BMenuItem(label, message);
	item->SetMarked(currentCompression == compression);
	menu->AddItem(item);
}


TIFFView::TIFFView(const char* name, uint32 flags,
	TranslatorSettings* settings)
	:
	BView(name, flags)
{
	fSettings = settings;

	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(ViewUIColor());

	fTitle = new BStringView("title", B_TRANSLATE("TIFF image translator"));
	fTitle->SetFont(be_bold_font);

	char detail[100];
	sprintf(detail, B_TRANSLATE("Version %d.%d.%d, %s"),
		static_cast<int>(B_TRANSLATION_MAJOR_VERSION(TIFF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_MINOR_VERSION(TIFF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_REVISION_VERSION(
			TIFF_TRANSLATOR_VERSION)), __DATE__);
	fDetail = new BStringView("detail", detail);

	int16 i = 1;
	fLibTIFF[0] = new BStringView(NULL, B_TRANSLATE("TIFF Library:"));
	char libtiff[] = TIFFLIB_VERSION_STR;
	char* tok = strtok(libtiff, "\n");
	while (i < 5 && tok) {
		fLibTIFF[i] = new BStringView(NULL, tok);
		tok = strtok(NULL, "\n");
		i++;
	}

	BPopUpMenu* menu = new BPopUpMenu("pick compression");

	uint32 currentCompression = fSettings->SetGetInt32(TIFF_SETTING_COMPRESSION);
	// create the menu items with the various compression methods
	add_menu_item(menu, COMPRESSION_NONE, B_TRANSLATE("None"),
		currentCompression);
	menu->AddSeparatorItem();
	add_menu_item(menu, COMPRESSION_PACKBITS, B_TRANSLATE("RLE (Packbits)"),
		currentCompression);
	add_menu_item(menu, COMPRESSION_DEFLATE, B_TRANSLATE("ZIP (Deflate)"),
		currentCompression);
	add_menu_item(menu, COMPRESSION_LZW, B_TRANSLATE("LZW"),
		currentCompression);

	// TODO: the disabled compression modes are not configured in libTIFF
	//	menu->AddSeparatorItem();
	//	add_menu_item(menu, COMPRESSION_JPEG, "JPEG", currentCompression);
	// TODO ? - strip encoding is not implemented in libTIFF for this compression
	//	add_menu_item(menu, COMPRESSION_JP2000, "JPEG2000", currentCompression);

	fCompressionMF = new BMenuField(B_TRANSLATE("Use compression:"), menu);

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fTitle)
		.Add(fDetail)
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.Add(fCompressionMF)
			.AddGlue()
			.End()
		.AddGlue()
		.Add(fLibTIFF[0])
		.Add(fLibTIFF[1])
		.Add(fLibTIFF[2])
		.Add(fLibTIFF[3]);
			// Theses 4 adding above work because we know there are 4 strings
			// but it's fragile: one string less in the library version and the
			// application breaks

	BFont font;
	GetFont(&font);
	SetExplicitPreferredSize(
		BSize((font.Size() * 350)/12, (font.Size() * 200)/12));
}


TIFFView::~TIFFView()
{
	fSettings->Release();
}


void
TIFFView::AllAttached()
{
	fCompressionMF->Menu()->SetTargetForItems(this);
}


void
TIFFView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_COMPRESSION_CHANGED: {
			int32 value;
			if (message->FindInt32("value", &value) >= B_OK) {
				fSettings->SetGetInt32(TIFF_SETTING_COMPRESSION, &value);
				fSettings->SaveSettings();
			}
			break;
		}
		default:
			BView::MessageReceived(message);
	}
}
