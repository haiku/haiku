/*****************************************************************************/
// TGAView
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TGAView.cpp
//
// This BView based object displays information about the TGATranslator.
//
//
// Copyright (c) 2002 OpenBeOS Project
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
#include "TGAView.h"
#include "TGATranslator.h"

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
TGAView::TGAView(const BRect &frame, const char *name,
	uint32 resize, uint32 flags, TGATranslatorSettings *psettings)
	:	BView(frame, name, resize, flags)
{
	fpsettings = psettings;
	
	SetViewColor(220,220,220,0);
	
	BMessage *pmsg;
	int32 val;
	
	pmsg = new BMessage(CHANGE_IGNORE_ALPHA);
	fpchkIgnoreAlpha = new BCheckBox(BRect(10, 45, 180, 62),
		"Ignore TGA alpha channel",
		"Ignore TGA alpha channel", pmsg);
	val = (psettings->SetGetIgnoreAlpha()) ? 1 : 0;
	fpchkIgnoreAlpha->SetValue(val);
	fpchkIgnoreAlpha->SetViewColor(ViewColor());
	AddChild(fpchkIgnoreAlpha);
	
	pmsg = new BMessage(CHANGE_RLE);
	fpchkRLE = new BCheckBox(BRect(10, 67, 180, 84),
		"Save with RLE Compression",
		"Save with RLE Compression", pmsg);
	val = (psettings->SetGetRLE()) ? 1 : 0;
	fpchkRLE->SetValue(val);
	fpchkRLE->SetViewColor(ViewColor());
	AddChild(fpchkRLE);
}

// ---------------------------------------------------------------
// Destructor
//
// Releases the TGATranslator settings
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
TGAView::~TGAView()
{
	fpsettings->Release();
}

// ---------------------------------------------------------------
// AllAttached
//
//
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
TGAView::AllAttached()
{
	BMessenger msgr(this);
	fpchkIgnoreAlpha->SetTarget(msgr);
	fpchkRLE->SetTarget(msgr);
}

// ---------------------------------------------------------------
// MessageReceived
//
// Handles state changes of the RLE setting checkbox
//
// Preconditions:
//
// Parameters: message	the actual BMessage that was received
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
TGAView::MessageReceived(BMessage *message)
{
	bool bnewval;
	switch (message->what) {
		case CHANGE_IGNORE_ALPHA:
			if (fpchkIgnoreAlpha->Value())
				bnewval = true;
			else
				bnewval = false;
			fpsettings->SetGetIgnoreAlpha(&bnewval);
			fpsettings->SaveSettings();
			break;
		
		case CHANGE_RLE:
			if (fpchkRLE->Value())
				bnewval = true;
			else
				bnewval = false;
			fpsettings->SetGetRLE(&bnewval);
			fpsettings->SaveSettings();
			break;
		
		default:
			BView::MessageReceived(message);
			break;
	}
}

// ---------------------------------------------------------------
// Draw
//
// Draws information about the TGATranslator to this view.
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
TGAView::Draw(BRect area)
{
	SetFont(be_bold_font);
	font_height fh;
	GetFontHeight(&fh);
	float xbold, ybold;
	xbold = fh.descent + 1;
	ybold = fh.ascent + fh.descent * 2 + fh.leading;
	
	char title[] = "OpenBeOS TGA Image Translator";
	DrawString(title, BPoint(xbold, ybold));
	
	SetFont(be_plain_font);
	font_height plainh;
	GetFontHeight(&plainh);
	float yplain;
	yplain = plainh.ascent + plainh.descent * 2 + plainh.leading;
	
	char detail[100];
	sprintf(detail, "Version %d.%d.%d %s",
		static_cast<int>(TGA_TRANSLATOR_VERSION >> 8),
		static_cast<int>((TGA_TRANSLATOR_VERSION >> 4) & 0xf),
		static_cast<int>(TGA_TRANSLATOR_VERSION & 0xf), __DATE__);
	DrawString(detail, BPoint(xbold, yplain + ybold));
/*	char copyright[] = "© 2002 OpenBeOS Project";
	DrawString(copyright, BPoint(xbold, yplain * 2 + ybold));
*/	
	char writtenby[] = "Written by the OBOS Translation Kit Team";
	DrawString(writtenby, BPoint(xbold, yplain * 7 + ybold));
}
