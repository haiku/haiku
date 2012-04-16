////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFView.cpp
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

// Additional authors:	Stephan Aßmus, <superstippi@gmx.de>
//                      Maxime Simon, <maxime.simon@gmail.com>

#include <stdio.h>
#include <stdlib.h>

#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <InterfaceKit.h>
#include <String.h>

#include "Prefs.h"
#include "SavePalette.h"

#include "GIFView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GIFView"


extern int32 translatorVersion;
extern char translatorName[];

// constructor
GIFView::GIFView(const char *name)
	: BView(name, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BStringView *title = new BStringView("Title", translatorName);
	title->SetFont(be_bold_font);

	char version_string[100];
	sprintf(version_string, "v%d.%d.%d %s", (int)(translatorVersion >> 8),
		(int)((translatorVersion >> 4) & 0xf), (int)(translatorVersion & 0xf),
		__DATE__);
	BStringView *version = new BStringView("Version", version_string);

	const char *copyrightString = "©2003 Daniel Switkin, software@switkin.com";
	BStringView *copyright = new BStringView("Copyright", copyrightString);

	// menu fields (Palette & Colors)
	fWebSafeMI = new BMenuItem(B_TRANSLATE("Websafe"),
		new BMessage(GV_WEB_SAFE), 0, 0);
	fBeOSSystemMI = new BMenuItem(B_TRANSLATE("BeOS system"),
		new BMessage(GV_BEOS_SYSTEM), 0, 0);
	fGreyScaleMI = new BMenuItem(B_TRANSLATE("Greyscale"),
		new BMessage(GV_GREYSCALE), 0, 0);
	fOptimalMI = new BMenuItem(B_TRANSLATE("Optimal"),
		new BMessage(GV_OPTIMAL), 0, 0);
	fPaletteM = new BPopUpMenu("PalettePopUpMenu", true, true, B_ITEMS_IN_COLUMN);
	fPaletteM->AddItem(fWebSafeMI);
	fPaletteM->AddItem(fBeOSSystemMI);
	fPaletteM->AddItem(fGreyScaleMI);
	fPaletteM->AddItem(fOptimalMI);

	fColorCountM = new BPopUpMenu("ColorCountPopUpMenu", true, true,
		B_ITEMS_IN_COLUMN);
	int32 count = 2;
	for (int32 i = 0; i < 8; i++) {
		BMessage* message = new BMessage(GV_SET_COLOR_COUNT);
		message->AddInt32("size in bits", i + 1);
		BString label;
		label << count;
		fColorCountMI[i] = new BMenuItem(label.String(), message, 0, 0);
		fColorCountM->AddItem(fColorCountMI[i]);
		count *= 2;
	}
	fColorCount256MI = fColorCountMI[7];

 	fPaletteMF = new BMenuField(B_TRANSLATE("Palette"), fPaletteM);
 	fColorCountMF = new BMenuField(B_TRANSLATE("Colors"), fColorCountM);

 	// check boxes
 	fUseDitheringCB = new BCheckBox(B_TRANSLATE("Use dithering"),
 		new BMessage(GV_USE_DITHERING));

 	fInterlacedCB = new BCheckBox(B_TRANSLATE("Write interlaced images"),
 		new BMessage(GV_INTERLACED));

 	fUseTransparentCB = new BCheckBox(B_TRANSLATE("Write transparent images"),
 		new BMessage(GV_USE_TRANSPARENT));

 	// radio buttons
 	fUseTransparentAutoRB = new BRadioButton(
 		B_TRANSLATE("Automatic (from alpha channel)"),
 		new BMessage(GV_USE_TRANSPARENT_AUTO));

 	fUseTransparentColorRB = new BRadioButton(B_TRANSLATE("Use RGB color"),
 		new BMessage(GV_USE_TRANSPARENT_COLOR));

 	fTransparentRedTC = new BTextControl("", "0", new BMessage(GV_TRANSPARENT_RED));

 	fTransparentGreenTC = new BTextControl("", "0", new BMessage(GV_TRANSPARENT_GREEN));

 	fTransparentBlueTC = new BTextControl("", "0", new BMessage(GV_TRANSPARENT_BLUE));

	BTextView *tr = fTransparentRedTC->TextView();
	BTextView *tg = fTransparentGreenTC->TextView();
	BTextView *tb = fTransparentBlueTC->TextView();

	for (uint32 x = 0; x < 256; x++) {
		if (x < '0' || x > '9') {
			tr->DisallowChar(x);
			tg->DisallowChar(x);
			tb->DisallowChar(x);
		}
	}

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 7)
		.Add(BGridLayoutBuilder(10, 10)
			.Add(title, 0, 0)
			.Add(version, 1, 0)
		)
		.Add(copyright)
		.AddGlue()

		.Add(BGridLayoutBuilder(10, 10)
			.Add(fPaletteMF->CreateLabelLayoutItem(), 0, 0)
        	.Add(fPaletteMF->CreateMenuBarLayoutItem(), 1, 0)

			.Add(fColorCountMF->CreateLabelLayoutItem(), 0, 1)
        	.Add(fColorCountMF->CreateMenuBarLayoutItem(), 1, 1)
		)
		.AddGlue()

		.Add(fUseDitheringCB)
		.Add(fInterlacedCB)
		.Add(fUseTransparentCB)

		.Add(fUseTransparentAutoRB)
		.Add(BGridLayoutBuilder(10, 10)
			.Add(fUseTransparentColorRB, 0, 0)
			.Add(fTransparentRedTC, 1, 0)
			.Add(fTransparentGreenTC, 2, 0)
			.Add(fTransparentBlueTC, 3, 0)
		)
		.AddGlue()
		.SetInsets(5, 5, 5, 5)
	);

	BFont font;
	GetFont(&font);
	SetExplicitPreferredSize(BSize((font.Size() * 400)/12, (font.Size() * 300)/12));

	RestorePrefs();
}


GIFView::~GIFView()
{
	delete fPrefs;
}


void
GIFView::RestorePrefs()
{
	fPrefs = new Prefs();

	fColorCountMF->SetEnabled(false);
	fUseDitheringCB->SetEnabled(true);

	switch (fPrefs->palettemode) {
		case WEB_SAFE_PALETTE:
			fWebSafeMI->SetMarked(true);
			break;
		case BEOS_SYSTEM_PALETTE:
			fBeOSSystemMI->SetMarked(true);
			break;
		case GREYSCALE_PALETTE:
			fGreyScaleMI->SetMarked(true);
			fUseDitheringCB->SetEnabled(false);
			break;
		case OPTIMAL_PALETTE:
			fOptimalMI->SetMarked(true);
			fColorCountMF->SetEnabled(true);
			break;
		default:
			fPrefs->palettemode = WEB_SAFE_PALETTE;
			fWebSafeMI->SetMarked(true);
			break;
	}

	if (fColorCountMF->IsEnabled() &&
		fPrefs->palette_size_in_bits > 0 &&
		fPrefs->palette_size_in_bits <= 8) {
		// display the stored color count
		fColorCountMI[fPrefs->palette_size_in_bits - 1]->SetMarked(true);
	} else {
		// display 256 colors
		fColorCount256MI->SetMarked(true);
		fPrefs->palette_size_in_bits = 8;
	}

	fInterlacedCB->SetValue(fPrefs->interlaced);

	if (fGreyScaleMI->IsMarked()) fUseDitheringCB->SetValue(false);
	else fUseDitheringCB->SetValue(fPrefs->usedithering);
	fUseTransparentCB->SetValue(fPrefs->usetransparent);
	fUseTransparentAutoRB->SetValue(fPrefs->usetransparentauto);
	fUseTransparentColorRB->SetValue(!fPrefs->usetransparentauto);
	if (fPrefs->usetransparent) {
		fUseTransparentAutoRB->SetEnabled(true);
		fUseTransparentColorRB->SetEnabled(true);
		fTransparentRedTC->SetEnabled(!fPrefs->usetransparentauto);
		fTransparentGreenTC->SetEnabled(!fPrefs->usetransparentauto);
		fTransparentBlueTC->SetEnabled(!fPrefs->usetransparentauto);
	} else {
		fUseTransparentAutoRB->SetEnabled(false);
		fUseTransparentColorRB->SetEnabled(false);
		fTransparentRedTC->SetEnabled(false);
		fTransparentGreenTC->SetEnabled(false);
		fTransparentBlueTC->SetEnabled(false);
	}

	char temp[4];
	sprintf(temp, "%d", fPrefs->transparentred);
	fTransparentRedTC->SetText(temp);
	sprintf(temp, "%d", fPrefs->transparentgreen);
	fTransparentGreenTC->SetText(temp);
	sprintf(temp, "%d", fPrefs->transparentblue);
	fTransparentBlueTC->SetText(temp);
}

// AllAttached
void
GIFView::AllAttached()
{
	BView::AllAttached();
	BMessenger msgr(this);
	fInterlacedCB->SetTarget(msgr);
	fUseDitheringCB->SetTarget(msgr);
	fUseTransparentCB->SetTarget(msgr);
	fUseTransparentAutoRB->SetTarget(msgr);
	fUseTransparentColorRB->SetTarget(msgr);
	fTransparentRedTC->SetTarget(msgr);
	fTransparentGreenTC->SetTarget(msgr);
	fTransparentBlueTC->SetTarget(msgr);
	fPaletteM->SetTargetForItems(msgr);
	fColorCountM->SetTargetForItems(msgr);
}

// MessageReceived
void
GIFView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case GV_WEB_SAFE:
			fPrefs->palettemode = WEB_SAFE_PALETTE;
			fUseDitheringCB->SetEnabled(true);
			fColorCountMF->SetEnabled(false);
			fColorCount256MI->SetMarked(true);
			break;
		case GV_BEOS_SYSTEM:
			fPrefs->palettemode = BEOS_SYSTEM_PALETTE;
			fUseDitheringCB->SetEnabled(true);
			fColorCountMF->SetEnabled(false);
			fColorCount256MI->SetMarked(true);
			break;
		case GV_GREYSCALE:
			fPrefs->palettemode = GREYSCALE_PALETTE;
			fUseDitheringCB->SetEnabled(false);
			fUseDitheringCB->SetValue(false);
			fColorCountMF->SetEnabled(false);
			fColorCount256MI->SetMarked(true);
			fPrefs->usedithering = false;
			break;
		case GV_OPTIMAL:
			fPrefs->palettemode = OPTIMAL_PALETTE;
			fUseDitheringCB->SetEnabled(true);
			fColorCountMF->SetEnabled(true);
			fColorCountMI[fPrefs->palette_size_in_bits - 1]->SetMarked(true);
			break;
		case GV_SET_COLOR_COUNT:
			if (fColorCountMF->IsEnabled()) {
				int32 sizeInBits;
				if (message->FindInt32("size in bits", &sizeInBits) >= B_OK) {
					if (sizeInBits > 0 && sizeInBits <= 8)
						fPrefs->palette_size_in_bits = sizeInBits;
				}
			}
			break;
		case GV_INTERLACED:
			fPrefs->interlaced = fInterlacedCB->Value();
			break;
		case GV_USE_DITHERING:
			fPrefs->usedithering = fUseDitheringCB->Value();
			break;
		case GV_USE_TRANSPARENT:
			fPrefs->usetransparent = fUseTransparentCB->Value();
			if (fPrefs->usetransparent) {
				fUseTransparentAutoRB->SetEnabled(true);
				fUseTransparentColorRB->SetEnabled(true);
				fTransparentRedTC->SetEnabled(fUseTransparentColorRB->Value());
				fTransparentGreenTC->SetEnabled(fUseTransparentColorRB->Value());
				fTransparentBlueTC->SetEnabled(fUseTransparentColorRB->Value());
			} else {
				fUseTransparentAutoRB->SetEnabled(false);
				fUseTransparentColorRB->SetEnabled(false);
				fTransparentRedTC->SetEnabled(false);
				fTransparentGreenTC->SetEnabled(false);
				fTransparentBlueTC->SetEnabled(false);
			}
			break;
		case GV_USE_TRANSPARENT_AUTO:
			fPrefs->usetransparentauto = true;
			fTransparentRedTC->SetEnabled(false);
			fTransparentGreenTC->SetEnabled(false);
			fTransparentBlueTC->SetEnabled(false);
			break;
		case GV_USE_TRANSPARENT_COLOR:
			fPrefs->usetransparentauto = false;
			fTransparentRedTC->SetEnabled(true);
			fTransparentGreenTC->SetEnabled(true);
			fTransparentBlueTC->SetEnabled(true);
			break;
		case GV_TRANSPARENT_RED:
			fPrefs->transparentred = CheckInput(fTransparentRedTC);
			break;
		case GV_TRANSPARENT_GREEN:
			fPrefs->transparentgreen = CheckInput(fTransparentGreenTC);
			break;
		case GV_TRANSPARENT_BLUE:
			fPrefs->transparentblue = CheckInput(fTransparentBlueTC);
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
	fPrefs->Save();
}

int GIFView::CheckInput(BTextControl *control) {
	int value = atoi(control->Text());
	if (value < 0 || value > 255) {
		value = (value < 0) ? 0 : 255;
		char temp[4];
		sprintf(temp, "%d", value);
		control->SetText(temp);
	}
	return value;
}

