/*****************************************************************************/
// STXTView
// Written by Michael Wilber, OBOS Translation Kit Team
//
// STXTView.cpp
//
// This BView based object displays information about the STXTTranslator.
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
#include "STXTView.h"
#include "STXTTranslator.h"

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
STXTView::STXTView(const BRect &frame, const char *name,
	uint32 resize, uint32 flags, TranslatorSettings *settings)
	:	BView(frame, name, resize, flags)
{
	fSettings = settings;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
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
STXTView::~STXTView()
{
	fSettings->Release();
}

// ---------------------------------------------------------------
// Draw
//
// Draws information about the STXTTranslator to this view.
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
STXTView::Draw(BRect area)
{
	SetFont(be_bold_font);
	font_height fh;
	GetFontHeight(&fh);
	float xbold, ybold;
	xbold = fh.descent + 1;
	ybold = fh.ascent + fh.descent * 2 + fh.leading;
	
	char title[] = "OpenBeOS StyledEdit Files Translator";
	DrawString(title, BPoint(xbold, ybold));
	
	SetFont(be_plain_font);
	font_height plainh;
	GetFontHeight(&plainh);
	float yplain;
	yplain = plainh.ascent + plainh.descent * 2 + plainh.leading;
	
	char detail[100];
	sprintf(detail, "Version %d.%d.%d %s",
		static_cast<int>(B_TRANSLATION_MAJOR_VER(STXT_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_MINOR_VER(STXT_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_REVSN_VER(STXT_TRANSLATOR_VERSION)),
		__DATE__);
	DrawString(detail, BPoint(xbold, yplain + ybold));
/*	char copyright[] = "© 2002 OpenBeOS Project";
	DrawString(copyright, BPoint(xbold, yplain * 2 + ybold));
*/	
	char writtenby[] = "Written by the OBOS Translation Kit Team";
	DrawString(writtenby, BPoint(xbold, yplain * 7 + ybold));
}
