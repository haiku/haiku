/*****************************************************************************/
// TIFFView
// Written by Michael Wilber, OBOS Translation Kit Team
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
#include "TIFFView.h"
#include "TIFFTranslator.h"
#include "tiffvers.h"

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
	uint32 resize, uint32 flags)
	:	BView(frame, name, resize, flags)
{
	SetViewColor(220,220,220,0);
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
		static_cast<int>(TIFF_TRANSLATOR_VERSION >> 8),
		static_cast<int>((TIFF_TRANSLATOR_VERSION >> 4) & 0xf),
		static_cast<int>(TIFF_TRANSLATOR_VERSION & 0xf), __DATE__);
	DrawString(detail, BPoint(xbold, yplain + ybold));
	
	int32 lineno = 4;
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
