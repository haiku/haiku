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
#include "PNGView.h"
#include "PNGTranslator.h"

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
PNGView::~PNGView()
{
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
		PNG_TRANSLATOR_VERSION / 100, (PNG_TRANSLATOR_VERSION / 10) % 10,
		PNG_TRANSLATOR_VERSION % 10, __DATE__);
	DrawString(detail, BPoint(xbold, yplain + ybold));
	
	int32 lineno = 4;
	DrawString("PNG Library:", BPoint(xbold, yplain * lineno + ybold));
	lineno += 2;
	
	char libtiff[] = "BEER!";
	char *tok = strtok(libtiff, "\n");
	while (tok) {
		DrawString(tok, BPoint(xbold, yplain * lineno + ybold));
		lineno++;
		tok = strtok(NULL, "\n");
	}
}
