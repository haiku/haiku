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

#include "GIFView.h"
#include "SavePalette.h"
#include "Prefs.h"
#include <InterfaceKit.h>
#include <stdlib.h>
#include <stdio.h>

extern int32 translatorVersion;
extern char translatorName[];

GIFView::GIFView(BRect rect, const char *name) :
	BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW) {
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	font_height fh;
	be_bold_font->GetHeight(&fh);
	BRect r(10, 15, 10 + be_bold_font->StringWidth(translatorName), 15 + fh.ascent + fh.descent);
	
	BStringView *title = new BStringView(r, "Title", translatorName);
	title->SetFont(be_bold_font);
	AddChild(title);
	
	char version_string[100];
	sprintf(version_string, "v%d.%d.%d %s", (int)(translatorVersion >> 8), (int)((translatorVersion >> 4) & 0xf),
		(int)(translatorVersion & 0xf), __DATE__);
	r.top = r.bottom + 20;
	be_plain_font->GetHeight(&fh);
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(version_string);
	
	BStringView *version = new BStringView(r, "Version", version_string);
	version->SetFont(be_plain_font);
	AddChild(version);
	
	const char *copyright_string = "© 2003 Daniel Switkin, software@switkin.com";
	r.top = r.bottom + 10;
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(copyright_string);
	
	BStringView *copyright = new BStringView(r, "Copyright", copyright_string);
	copyright->SetFont(be_plain_font);
	AddChild(copyright);
	
	websafe = new BMenuItem("websafe", new BMessage(GV_WEB_SAFE), 0, 0);
	beossystem = new BMenuItem("BeOS system", new BMessage(GV_BEOS_SYSTEM), 0, 0);
	greyscale = new BMenuItem("greyscale", new BMessage(GV_GREYSCALE), 0, 0);
	optimal = new BMenuItem("optimal", new BMessage(GV_OPTIMAL), 0, 0);
	palettepopupmenu = new BPopUpMenu("PalettePopUpMenu", true, true, B_ITEMS_IN_COLUMN);
	palettepopupmenu->AddItem(websafe);
	palettepopupmenu->AddItem(beossystem);
	palettepopupmenu->AddItem(greyscale);
	palettepopupmenu->AddItem(optimal);
	
	r.top = r.bottom + 10;
	r.bottom = r.top + 24;
	r.right = r.left + be_plain_font->StringWidth("Palette:") +
		be_plain_font->StringWidth("BeOS system") + 32;
	palettemenufield = new BMenuField(r, "PaletteMenuField", "Palette:",
		palettepopupmenu, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	AddChild(palettemenufield);
	palettemenufield->SetDivider(be_plain_font->StringWidth("Palette:") + 7);
	
	r.top = palettemenufield->Frame().bottom + 5;
	r.bottom = r.top + 10;
	r.right = r.left + be_plain_font->StringWidth("Use dithering") + 20;
	usedithering = new BCheckBox(r, "UseDithering", "Use dithering",
		new BMessage(GV_USE_DITHERING));
	AddChild(usedithering);
	
	r.top = usedithering->Frame().bottom + 2;
	r.bottom = r.top + 10;
	r.right = r.left + be_plain_font->StringWidth("Write interlaced images") + 20;
	interlaced = new BCheckBox(r, "Interlaced", "Write interlaced images",
		new BMessage(GV_INTERLACED));
	AddChild(interlaced);
	
	r.top = interlaced->Frame().bottom + 2;
	r.bottom = r.top + 10;
	r.right = r.left + be_plain_font->StringWidth("Write transparent images") + 20;
	usetransparent = new BCheckBox(r, "UseTransparent", "Write transparent images",
		new BMessage(GV_USE_TRANSPARENT));
	AddChild(usetransparent);
	
	r.top = usetransparent->Frame().bottom + 2;
	r.bottom = r.top + 10;
	r.right = r.left + be_plain_font->StringWidth("Use index:") + 20;
	usetransparentindex = new BRadioButton(r, "UseTransparentIndex", "Use index:",
		new BMessage(GV_USE_TRANSPARENT_INDEX));
	AddChild(usetransparentindex);
	
	r.left = r.right + 1;
	r.right = r.left + 30;
	transparentindex = new BTextControl(r, "TransparentIndex", "", "0",
		new BMessage(GV_TRANSPARENT_INDEX));
	AddChild(transparentindex);
	transparentindex->SetDivider(0);
	
	r.top = usetransparentindex->Frame().bottom + 0;
	r.bottom = r.top + 10;
	r.left = 10;
	r.right = r.left + be_plain_font->StringWidth("Use rgb color:") + 20;
	usetransparentcolor = new BRadioButton(r, "UseTransparentColor", "Use rgb color:",
		new BMessage(GV_USE_TRANSPARENT_COLOR));
	AddChild(usetransparentcolor);
	
	r.left = r.right + 1;
	r.right = r.left + 30;
	transparentred = new BTextControl(r, "TransparentRed", "", "0",
		new BMessage(GV_TRANSPARENT_RED));
	AddChild(transparentred);
	transparentred->SetDivider(0);
	
	r.left = r.right + 5;
	r.right = r.left + 30;
	transparentgreen = new BTextControl(r, "TransparentGreen", "", "0",
		new BMessage(GV_TRANSPARENT_GREEN));
	AddChild(transparentgreen);
	transparentgreen->SetDivider(0);
	
	r.left = r.right + 5;
	r.right = r.left + 30;
	transparentblue = new BTextControl(r, "TransparentBlue", "", "0",
		new BMessage(GV_TRANSPARENT_BLUE));
	AddChild(transparentblue);
	transparentblue->SetDivider(0);
	
	BTextView *ti = transparentindex->TextView();
	BTextView *tr = transparentred->TextView();
	BTextView *tg = transparentgreen->TextView();
	BTextView *tb = transparentblue->TextView();
	
	for (uint32 x = 0; x < 256; x++) {
		if (x < '0' || x > '9') {
			ti->DisallowChar(x);
			tr->DisallowChar(x);
			tg->DisallowChar(x);
			tb->DisallowChar(x);
		}
	}
	
	RestorePrefs();
}

void GIFView::RestorePrefs() {
	prefs = new Prefs();
	
	switch (prefs->palettemode) {
		case WEB_SAFE_PALETTE:
			websafe->SetMarked(true);
			break;
		case BEOS_SYSTEM_PALETTE:
			beossystem->SetMarked(true);
			break;
		case GREYSCALE_PALETTE:
			greyscale->SetMarked(true);
			usedithering->SetEnabled(false);
			break;
		case OPTIMAL_PALETTE:
			optimal->SetMarked(true);
			break;
		default:
			prefs->palettemode = WEB_SAFE_PALETTE;
			websafe->SetMarked(true);
			break;
	}
	
	interlaced->SetValue(prefs->interlaced);
	
	if (greyscale->IsMarked()) usedithering->SetValue(false);
	else usedithering->SetValue(prefs->usedithering);
	usetransparent->SetValue(prefs->usetransparent);
	usetransparentindex->SetValue(prefs->usetransparentindex);
	usetransparentcolor->SetValue(!prefs->usetransparentindex);
	if (prefs->usetransparent) {
		usetransparentindex->SetEnabled(true);
		usetransparentcolor->SetEnabled(true);
		transparentindex->SetEnabled(prefs->usetransparentindex);
		transparentred->SetEnabled(!prefs->usetransparentindex);
		transparentgreen->SetEnabled(!prefs->usetransparentindex);
		transparentblue->SetEnabled(!prefs->usetransparentindex);
	} else {
		usetransparentindex->SetEnabled(false);
		usetransparentcolor->SetEnabled(false);
		transparentindex->SetEnabled(false);
		transparentred->SetEnabled(false);
		transparentgreen->SetEnabled(false);
		transparentblue->SetEnabled(false);
	}
	
	char temp[4];
	sprintf(temp, "%d", prefs->transparentindex);
	transparentindex->SetText(temp);
	sprintf(temp, "%d", prefs->transparentred);
	transparentred->SetText(temp);
	sprintf(temp, "%d", prefs->transparentgreen);
	transparentgreen->SetText(temp);
	sprintf(temp, "%d", prefs->transparentblue);
	transparentblue->SetText(temp);
}

void GIFView::AllAttached() {
	BView::AllAttached();
	BMessenger msgr(this);
	interlaced->SetTarget(msgr);
	usedithering->SetTarget(msgr);
	usetransparent->SetTarget(msgr);
	usetransparentindex->SetTarget(msgr);
	transparentindex->SetTarget(msgr);
	usetransparentcolor->SetTarget(msgr);
	transparentred->SetTarget(msgr);
	transparentgreen->SetTarget(msgr);
	transparentblue->SetTarget(msgr);
	websafe->SetTarget(msgr);
	beossystem->SetTarget(msgr);
	greyscale->SetTarget(msgr);
	optimal->SetTarget(msgr);
}

void GIFView::MessageReceived(BMessage *message) {
	switch (message->what) {
		case GV_WEB_SAFE:
			prefs->palettemode = WEB_SAFE_PALETTE;
			usedithering->SetEnabled(true);
			break;
		case GV_BEOS_SYSTEM:
			prefs->palettemode = BEOS_SYSTEM_PALETTE;
			usedithering->SetEnabled(true);
			break;
		case GV_GREYSCALE:
			prefs->palettemode = GREYSCALE_PALETTE;
			usedithering->SetEnabled(false);
			usedithering->SetValue(false);
			prefs->usedithering = false;
			break;
		case GV_OPTIMAL:
			prefs->palettemode = OPTIMAL_PALETTE;
			usedithering->SetEnabled(true);
			break;
		case GV_INTERLACED:
			prefs->interlaced = interlaced->Value();
			break;
		case GV_USE_DITHERING:
			prefs->usedithering = usedithering->Value();
			break;
		case GV_USE_TRANSPARENT:
			prefs->usetransparent = usetransparent->Value();
			if (prefs->usetransparent) {
				usetransparentindex->SetEnabled(true);
				usetransparentcolor->SetEnabled(true);
				transparentindex->SetEnabled(usetransparentindex->Value());
				transparentred->SetEnabled(usetransparentcolor->Value());
				transparentgreen->SetEnabled(usetransparentcolor->Value());
				transparentblue->SetEnabled(usetransparentcolor->Value());
			} else {
				usetransparentindex->SetEnabled(false);
				usetransparentcolor->SetEnabled(false);
				transparentindex->SetEnabled(false);
				transparentred->SetEnabled(false);
				transparentgreen->SetEnabled(false);
				transparentblue->SetEnabled(false);
			}
			break;
		case GV_USE_TRANSPARENT_INDEX:
			prefs->usetransparentindex = true;
			transparentindex->SetEnabled(true);
			transparentred->SetEnabled(false);
			transparentgreen->SetEnabled(false);
			transparentblue->SetEnabled(false);
			break;
		case GV_TRANSPARENT_INDEX:
			prefs->transparentindex = CheckInput(transparentindex);
			break;
		case GV_USE_TRANSPARENT_COLOR:
			prefs->usetransparentindex = false;
			transparentindex->SetEnabled(false);
			transparentred->SetEnabled(true);
			transparentgreen->SetEnabled(true);
			transparentblue->SetEnabled(true);
			break;
		case GV_TRANSPARENT_RED:
			prefs->transparentred = CheckInput(transparentred);
			break;
		case GV_TRANSPARENT_GREEN:
			prefs->transparentgreen = CheckInput(transparentgreen);
			break;
		case GV_TRANSPARENT_BLUE:
			prefs->transparentblue = CheckInput(transparentblue);
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
	prefs->Save();
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

GIFView::~GIFView() {
	delete prefs;
}

