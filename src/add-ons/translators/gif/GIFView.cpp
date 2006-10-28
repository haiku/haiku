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

#include <stdio.h>
#include <stdlib.h>

#include <InterfaceKit.h>
#include <String.h>

#include "Prefs.h"
#include "SavePalette.h"

#include "GIFView.h"

extern int32 translatorVersion;
extern char translatorName[];

// constructor
GIFView::GIFView(BRect rect, const char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	font_height fh;
	be_bold_font->GetHeight(&fh);
	BRect r(10, 15, 10 + be_bold_font->StringWidth(translatorName),
		15 + fh.ascent + fh.descent);

	BStringView *title = new BStringView(r, "Title", translatorName);
	title->SetFont(be_bold_font);
	AddChild(title);

	char version_string[100];
	sprintf(version_string, "v%d.%d.%d %s", (int)(translatorVersion >> 8), (int)((translatorVersion >> 4) & 0xf),
		(int)(translatorVersion & 0xf), __DATE__);
	be_plain_font->GetHeight(&fh);
	r.bottom = r.top + fh.ascent + fh.descent;
	r.left = r.right + 2.0 * r.Height();
	r.right = r.left + be_plain_font->StringWidth(version_string);

	BStringView *version = new BStringView(r, "Version", version_string);
	version->SetFont(be_plain_font);
	AddChild(version);

	const char *copyrightString = "©2003 Daniel Switkin, software@switkin.com";
	r.top = r.bottom + 5;
	r.left = 10;
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = rect.right - 10;

	BStringView *copyright = new BStringView(r, "Copyright", copyrightString);
	copyright->ResizeToPreferred();
	AddChild(copyright);

	// menu fields (Palette & Colors)
	fWebSafeMI = new BMenuItem("Websafe", new BMessage(GV_WEB_SAFE), 0, 0);
	fBeOSSystemMI = new BMenuItem("BeOS System", new BMessage(GV_BEOS_SYSTEM), 0, 0);
	fGreyScaleMI = new BMenuItem("Greyscale", new BMessage(GV_GREYSCALE), 0, 0);
	fOptimalMI = new BMenuItem("Optimal", new BMessage(GV_OPTIMAL), 0, 0);
	fPaletteM = new BPopUpMenu("PalettePopUpMenu", true, true, B_ITEMS_IN_COLUMN);
	fPaletteM->AddItem(fWebSafeMI);
	fPaletteM->AddItem(fBeOSSystemMI);
	fPaletteM->AddItem(fGreyScaleMI);
	fPaletteM->AddItem(fOptimalMI);

	fColorCountM = new BPopUpMenu("ColorCountPopUpMenu", true, true, B_ITEMS_IN_COLUMN);
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
	
	r.top = r.bottom + 14;
	r.bottom = r.top + 24;
	fPaletteMF = new BMenuField(r, "PaletteMenuField", "Palette",
		fPaletteM, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	AddChild(fPaletteMF);

	r.top = r.bottom + 5;
	r.bottom = r.top + 24;
	fColorCountMF = new BMenuField(r, "ColorCountMenuField", "Colors",
		fColorCountM, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	AddChild(fColorCountMF);

	// align menu fields
	float maxLabelWidth = ceilf(max_c(be_plain_font->StringWidth("Colors"),
									  be_plain_font->StringWidth("Palette")));
	fPaletteMF->SetDivider(maxLabelWidth + 7);
	fColorCountMF->SetDivider(maxLabelWidth + 7);

	// check boxen
	r.top = fColorCountMF->Frame().bottom + 5;
	r.bottom = r.top + 10;
	fUseDitheringCB = new BCheckBox(r, "UseDithering", "Use dithering",
		new BMessage(GV_USE_DITHERING));
	AddChild(fUseDitheringCB);

	r.top = fUseDitheringCB->Frame().bottom + 2;
	r.bottom = r.top + 10;
	fInterlacedCB = new BCheckBox(r, "Interlaced", "Write interlaced images",
		new BMessage(GV_INTERLACED));
	AddChild(fInterlacedCB);
	
	r.top = fInterlacedCB->Frame().bottom + 2;
	r.bottom = r.top + 10;
	fUseTransparentCB = new BCheckBox(r, "UseTransparent", "Write transparent images",
		new BMessage(GV_USE_TRANSPARENT));
	AddChild(fUseTransparentCB);

	// radio buttons
	r.top = fUseTransparentCB->Frame().bottom + 2;
	r.bottom = r.top + 10;
	r.right = r.left + be_plain_font->StringWidth("Automatic (from alpha channel)") + 20;
	fUseTransparentAutoRB = new BRadioButton(r, "UseTransparentAuto", "Automatic (from alpha channel)",
		new BMessage(GV_USE_TRANSPARENT_AUTO));
	AddChild(fUseTransparentAutoRB);
	
	r.top = fUseTransparentAutoRB->Frame().bottom + 0;
	r.bottom = r.top + 10;
	r.left = 10;
	r.right = r.left + be_plain_font->StringWidth("Use RGB color") + 20;
	fUseTransparentColorRB = new BRadioButton(r, "UseTransparentColor", "Use RGB color",
		new BMessage(GV_USE_TRANSPARENT_COLOR));
	AddChild(fUseTransparentColorRB);
	
	r.left = r.right + 1;
	r.right = r.left + 30;
	fTransparentRedTC = new BTextControl(r, "TransparentRed", "", "0",
		new BMessage(GV_TRANSPARENT_RED));
	AddChild(fTransparentRedTC);
	fTransparentRedTC->SetDivider(0);
	
	r.left = r.right + 5;
	r.right = r.left + 30;
	fTransparentGreenTC = new BTextControl(r, "TransparentGreen", "", "0",
		new BMessage(GV_TRANSPARENT_GREEN));
	AddChild(fTransparentGreenTC);
	fTransparentGreenTC->SetDivider(0);
	
	r.left = r.right + 5;
	r.right = r.left + 30;
	fTransparentBlueTC = new BTextControl(r, "TransparentBlue", "", "0",
		new BMessage(GV_TRANSPARENT_BLUE));
	AddChild(fTransparentBlueTC);
	fTransparentBlueTC->SetDivider(0);

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

	RestorePrefs();

	if (copyright->Frame().right + 10 > Bounds().Width())
		ResizeTo(copyright->Frame().right + 10, Bounds().Height());
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

