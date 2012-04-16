/*****************************************************************************/
// SGIView
// Adopted by Stephan Aßmus, <stippi@yellowbites.com>
// from TIFFView written by
// Picking the compression method added by Stephan Aßmus, <stippi@yellowbites.com>
//
// SGIView.cpp
//
// This BView based object displays information about the SGITranslator.
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

#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>
#include <StringView.h>
#include <TextView.h>
#include <Window.h>

#include "SGIImage.h"
#include "SGITranslator.h"
#include "SGIView.h"

const char* author = "Stephan Aßmus, <stippi@yellowbites.com>";

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SGIView"

// add_menu_item
void
add_menu_item(BMenu* menu,
			  uint32 compression,
			  const char* label,
			  uint32 currentCompression)
{
	BMessage* message = new BMessage(SGIView::MSG_COMPRESSION_CHANGED);
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
SGIView::SGIView(const char* name, uint32 flags, TranslatorSettings* settings)
	:
	BView(name, flags, new BGroupLayout(B_VERTICAL)),
	fSettings(settings)
{
	BPopUpMenu* menu = new BPopUpMenu("pick compression");

	uint32 currentCompression = 
		fSettings->SetGetInt32(SGI_SETTING_COMPRESSION);
	// create the menu items with the various compression methods
	add_menu_item(menu, SGI_COMP_NONE, B_TRANSLATE("None"), 
		currentCompression);
//	menu->AddSeparatorItem();
	add_menu_item(menu, SGI_COMP_RLE, B_TRANSLATE("RLE"), currentCompression);

// DON'T turn this on, it's so slow that I didn't wait long enough
// the one time I tested this. So I don't know if the code even works.
// Supposedly, this would look for an already written scanline, and
// modify the scanline tables so that the current row is not written
// at all...

//	add_menu_item(menu, SGI_COMP_ARLE, "Agressive RLE", currentCompression);

	fCompressionMF = new BMenuField("compression", 
		B_TRANSLATE("Use compression:"), menu);

	BAlignment labelAlignment(B_ALIGN_LEFT, B_ALIGN_NO_VERTICAL);

	BStringView* titleView = new BStringView("title", 
		B_TRANSLATE("SGI image translator"));
	titleView->SetFont(be_bold_font);
	titleView->SetExplicitAlignment(labelAlignment);

	char detail[100];
	sprintf(detail, B_TRANSLATE("Version %d.%d.%d %s"),
		static_cast<int>(B_TRANSLATION_MAJOR_VERSION(SGI_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_MINOR_VERSION(SGI_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_REVISION_VERSION(
			SGI_TRANSLATOR_VERSION)), __DATE__);
	BStringView* detailView = new BStringView("details", detail);
	detailView->SetExplicitAlignment(labelAlignment);

	BTextView* infoView = new BTextView("info");
	infoView->SetText(BString(B_TRANSLATE("written by:\n"))
			.Append(author)
			.Append(B_TRANSLATE("\n\nbased on GIMP SGI plugin v1.5:\n"))
			.Append(kSGICopyright).String());
	infoView->SetExplicitAlignment(labelAlignment);
	infoView->SetWordWrap(false);
	infoView->MakeEditable(false);
	infoView->MakeResizable(true);
	infoView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	float padding = 5.0f;
	AddChild(BGroupLayoutBuilder(B_VERTICAL, padding)
		.Add(titleView)
		.Add(detailView)
		.Add(fCompressionMF)
		.Add(infoView)
		.AddGlue()
		.SetInsets(padding, padding, padding, padding)
	);

	BFont font;
	GetFont(&font);
 	SetExplicitPreferredSize(
		BSize((font.Size() * 390) / 12, (font.Size() * 180) / 12));

	// TODO: remove this workaround for ticket #4217
	infoView->SetExplicitPreferredSize(
		BSize(infoView->LineWidth(4), infoView->TextHeight(0, 80)));
	infoView->SetExplicitMaxSize(infoView->ExplicitPreferredSize());
	infoView->SetExplicitMinSize(infoView->ExplicitPreferredSize());
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
SGIView::~SGIView()
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
SGIView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_COMPRESSION_CHANGED: {
			int32 value;
			if (message->FindInt32("value", &value) >= B_OK) {
				fSettings->SetGetInt32(SGI_SETTING_COMPRESSION, &value);
				fSettings->SaveSettings();
			}
			fCompressionMF->ResizeToPreferred();
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
SGIView::AllAttached()
{
	fCompressionMF->Menu()->SetTargetForItems(this);
}

