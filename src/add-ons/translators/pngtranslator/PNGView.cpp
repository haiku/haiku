/*****************************************************************************/
// PNGView
// Written by Michael Wilber, OBOS Translation Kit Team
//
// PNGView.cpp
//
// This BView based object displays information about the PNGTranslator.
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
#include <png.h>
#include <Alert.h>
#include "PNGView.h"
#include "PNGTranslator.h"

BMessage *
interlace_msg(int option)
{
	BMessage *pmsg = new BMessage(M_PNG_SET_INTERLACE);
	pmsg->AddInt32(PNG_SETTING_INTERLACE, option);
	return pmsg;
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
PNGView::PNGView(const BRect &frame, const char *name,
	uint32 resize, uint32 flags, TranslatorSettings *settings)
	:	BView(frame, name, resize, flags)
{
	fSettings = settings;
	
	SetViewColor(220,220,220,0);
	
	// setup PNG interlace options menu
	fpmnuInterlace = new BPopUpMenu("Interlace Option");
	BMenuItem *pitmNone, *pitmAdam7;
	pitmNone = new BMenuItem("None", interlace_msg(PNG_INTERLACE_NONE));
	pitmAdam7 = new BMenuItem("Adam7", interlace_msg(PNG_INTERLACE_ADAM7));
	fpmnuInterlace->AddItem(pitmNone);
	fpmnuInterlace->AddItem(pitmAdam7);
	
	fpfldInterlace = new BMenuField(BRect(20, 50, 200, 70),
		"PNG Interlace Menu", "Interlacing Type", fpmnuInterlace);
	fpfldInterlace->SetViewColor(ViewColor());
	AddChild(fpfldInterlace);
	
	// set Interlace option to show the current configuration
	if (fSettings->SetGetInt32(PNG_SETTING_INTERLACE) == PNG_INTERLACE_NONE)
		pitmNone->SetMarked(true);
	else
		pitmAdam7->SetMarked(true);
}

// ---------------------------------------------------------------
// Destructor
//
// Releases the PNGTranslator settings
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
PNGView::~PNGView()
{
	fSettings->Release();
}

void
PNGView::AllAttached()
{
	// Tell all controls that this view is the target for 
	// messages from controls on this view
	BView::AllAttached();
	BMessenger msgr(this);
	
	// set target for interlace options menu items
	for (int32 i = 0; i < fpmnuInterlace->CountItems(); i++) {
		BMenuItem *pitm = fpmnuInterlace->ItemAt(i);
		if (pitm)
			pitm->SetTarget(msgr);
	}
}

// ---------------------------------------------------------------
// Draw
//
// Draws information about the PNGTranslator to this view.
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
PNGView::Draw(BRect area)
{
	SetFont(be_bold_font);
	font_height fh;
	GetFontHeight(&fh);
	float xbold, ybold;
	xbold = fh.descent + 1;
	ybold = fh.ascent + fh.descent * 2 + fh.leading;
	
	char title[] = "OpenBeOS PNG Image Translator";
	DrawString(title, BPoint(xbold, ybold));
	
	SetFont(be_plain_font);
	font_height plainh;
	GetFontHeight(&plainh);
	float yplain;
	yplain = plainh.ascent + plainh.descent * 2 + plainh.leading;
	
	char detail[100];
	sprintf(detail, "Version %d.%d.%d %s",
		static_cast<int>(B_TRANSLATION_MAJOR_VER(PNG_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_MINOR_VER(PNG_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_REVSN_VER(PNG_TRANSLATOR_VERSION)),
		__DATE__);
	DrawString(detail, BPoint(xbold, yplain + ybold));
	
	int32 lineno = 6;
	DrawString("PNG Library:", BPoint(xbold, yplain * lineno + ybold));
	lineno += 1;
	
	const int32 kinfolen = 512;
	char libinfo[kinfolen] = { 0 };
	strncpy(libinfo, png_get_copyright(NULL), kinfolen - 1);
	char *tok = strtok(libinfo, "\n");
	while (tok) {
		DrawString(tok, BPoint(xbold, yplain * lineno + ybold));
		lineno++;
		tok = strtok(NULL, "\n");
	}
}

void
PNGView::MessageReceived(BMessage *pmsg)
{
	if (pmsg->what == M_PNG_SET_INTERLACE) {
		// change setting for interlace option
		int32 option;
		if (pmsg->FindInt32(PNG_SETTING_INTERLACE, &option) == B_OK) {
			fSettings->SetGetInt32(PNG_SETTING_INTERLACE, &option);
			fSettings->SaveSettings();
		}
	} else
		BView::MessageReceived(pmsg);
}

