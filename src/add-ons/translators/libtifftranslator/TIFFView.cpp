/*****************************************************************************/
// TIFFView
// Written by Michael Wilber, OBOS Translation Kit Team
// Picking the compression method added by Stephan Aßmus, <stippi@yellowbites.com>
//
// TIFFView.cpp
//
// This BView based object displays information about the TIFFTranslator.
//
//
// Copyright (c) 2003 OpenBeOS Project
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

#include <stdio.h>
#include <string.h>

#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include "tiff.h"
#include "tiffvers.h"

#include "TIFFTranslator.h"
#include "TranslatorSettings.h"

#include "TIFFView.h"

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

// ---------------------------------------------------------------
// Constructor
//
// Sets up the view settings
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
TIFFView::TIFFView(const BRect &frame, const char *name,
	uint32 resize, uint32 flags, TranslatorSettings *settings)
	:	BView(frame, name, resize, flags)
{
	fSettings = settings;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BPopUpMenu* menu = new BPopUpMenu("pick compression");

	uint32 currentCompression = fSettings->SetGetInt32(TIFF_SETTING_COMPRESSION);
	// create the menu items with the various compression methods
	add_menu_item(menu, COMPRESSION_NONE, "None", currentCompression);
	menu->AddSeparatorItem();
	add_menu_item(menu, COMPRESSION_PACKBITS, "RLE (Packbits)", currentCompression);
	add_menu_item(menu, COMPRESSION_DEFLATE, "ZIP (Deflate)", currentCompression);
	add_menu_item(menu, COMPRESSION_LZW, "LZW", currentCompression);

// TODO: the disabled compression modes are not configured in libTIFF
//	menu->AddSeparatorItem();
//	add_menu_item(menu, COMPRESSION_JPEG, "JPEG", currentCompression);
// TODO ? - strip encoding is not implemented in libTIFF for this compression
//	add_menu_item(menu, COMPRESSION_JP2000, "JPEG2000", currentCompression);

	fCompressionMF = new BMenuField(BRect(20, 50, 215, 70), "compression",
									"Use Compression:", menu, true);
	AddChild(fCompressionMF);
}

// ---------------------------------------------------------------
// Destructor
//
// Does nothing
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
TIFFView::~TIFFView()
{
	fSettings->Release();
}

// ---------------------------------------------------------------
// MessageReceived
//
// Handles state changes of the Compression menu field
//
// Preconditions:
//
// Parameters: area,	not used
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
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

// ---------------------------------------------------------------
// AllAttached
//
// sets the target for the controls controlling the configuration
//
// Preconditions:
//
// Parameters: area,	not used
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
TIFFView::AllAttached()
{
	fCompressionMF->Menu()->SetTargetForItems(this);
}


// ---------------------------------------------------------------
// Draw
//
// Draws information about the TIFFTranslator to this view.
//
// Preconditions:
//
// Parameters: area,	not used
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
TIFFView::Draw(BRect area)
{
	SetFont(be_bold_font);
	font_height fh;
	GetFontHeight(&fh);
	float xbold, ybold;
	xbold = fh.descent + 1;
	ybold = fh.ascent + fh.descent * 2 + fh.leading;
	
	char title[] = "OpenBeOS TIFF Image Translator";
	DrawString(title, BPoint(xbold, ybold));
	
	SetFont(be_plain_font);
	font_height plainh;
	GetFontHeight(&plainh);
	float yplain;
	yplain = plainh.ascent + plainh.descent * 2 + plainh.leading;
	
	char detail[100];
	sprintf(detail, "Version %d.%d.%d %s",
		static_cast<int>(B_TRANSLATION_MAJOR_VER(TIFF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_MINOR_VER(TIFF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_REVSN_VER(TIFF_TRANSLATOR_VERSION)),
		__DATE__);
	DrawString(detail, BPoint(xbold, yplain + ybold));
	
	int32 lineno = 6;
	DrawString("TIFF Library:", BPoint(xbold, yplain * lineno + ybold));
	lineno += 2;
	
	char libtiff[] = TIFFLIB_VERSION_STR;
	char *tok = strtok(libtiff, "\n");
	while (tok) {
		DrawString(tok, BPoint(xbold, yplain * lineno + ybold));
		lineno++;
		tok = strtok(NULL, "\n");
	}
}
