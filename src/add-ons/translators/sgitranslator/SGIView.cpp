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

#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Window.h>

#include "SGIImage.h"
#include "SGITranslator.h"
#include "SGITranslatorSettings.h"

#include "SGIView.h"

const char* author = "Stephan Aßmus, <stippi@yellowbites.com>";

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
SGIView::SGIView(const BRect &frame, const char *name,
	uint32 resize, uint32 flags, SGITranslatorSettings* settings)
	:	BView(frame, name, resize, flags),
		fSettings(settings)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BPopUpMenu* menu = new BPopUpMenu("pick compression");

	uint32 currentCompression = fSettings->SetGetCompression();
	// create the menu items with the various compression methods
	add_menu_item(menu, SGI_COMP_NONE, "None", currentCompression);
//	menu->AddSeparatorItem();
	add_menu_item(menu, SGI_COMP_RLE, "RLE", currentCompression);

// DON'T turn this on, it's so slow that I didn't wait long enough
// the one time I tested this. So I don't know if the code even works.
// Supposedly, this would look for an already written scanline, and
// modify the scanline tables so that the current row is not written
// at all...

//	add_menu_item(menu, SGI_COMP_ARLE, "Agressive RLE", currentCompression);

	BRect menuFrame = Bounds();
	menuFrame.bottom = menuFrame.top + menu->Bounds().Height();
	fCompressionMF = new BMenuField(menuFrame, "compression",
									"Use Compression:", menu, true/*,
									B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP*/);
	if (fCompressionMF->MenuBar())
		fCompressionMF->MenuBar()->ResizeToPreferred();
	fCompressionMF->ResizeToPreferred();

	// figure out where the text ends
	font_height fh;
	be_bold_font->GetHeight(&fh);
	float xbold, ybold;
	xbold = fh.descent + 1;
	ybold = fh.ascent + fh.descent * 2 + fh.leading;
	
	font_height plainh;
	be_plain_font->GetHeight(&plainh);
	float yplain;
	yplain = plainh.ascent + plainh.descent * 2 + plainh.leading;
	
	// position the menu field below all the text we draw in Draw()
	BPoint textOffset(0.0, yplain * 2 + ybold);
	fCompressionMF->MoveTo(textOffset);

	AddChild(fCompressionMF);

	ResizeToPreferred();
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
			uint32 value;
			if (message->FindInt32("value", (int32*)&value) >= B_OK) {
				fSettings->SetGetCompression(&value);
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
SGIView::AllAttached()
{
	fCompressionMF->Menu()->SetTargetForItems(this);
}

// ---------------------------------------------------------------
// AttachedToWindow
//
// hack to make the window recognize our size
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
SGIView::AttachedToWindow()
{
	// Hack for DataTranslations which doesn't resize visible area to requested by view
	// which makes some parts of bigger than usual translationviews out of visible area
	// so if it was loaded to DataTranslations resize window if needed
	BWindow *window = Window();
	if (!strcmp(window->Name(), "DataTranslations")) {
		BView *view = Parent();
		if (view) {
			BRect frame = view->Frame();
			float x, y;
			GetPreferredSize(&x, &y);
			if (frame.Width() < x || (frame.Height() - 48) < y) {
				x -= frame.Width();
				y -= frame.Height() - 48;
				if (x < 0) x = 0;
				if (y < 0) y = 0;

				// DataTranslations has main view called "Background"
				// change it's resizing mode so it will always resize with window
				// also make sure view will be redrawed after resize
				view = window->FindView("Background");
				if (view) {
					view->SetResizingMode(B_FOLLOW_ALL);
					view->SetFlags(B_FULL_UPDATE_ON_RESIZE);
				}

				// The same with "Info..." button, except redrawing, which isn't needed
				view = window->FindView("Info…");
				if (view)
					view->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

				window->ResizeBy( x, y);
			}
		}
	}
}

// ---------------------------------------------------------------
// Draw
//
// Draws information about the SGITranslator to this view.
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
SGIView::Draw(BRect area)
{
	SetFont(be_bold_font);
	font_height fh;
	GetFontHeight(&fh);
	float xbold, ybold;
	xbold = fh.descent + 1;
	ybold = fh.ascent + fh.descent * 2 + fh.leading;
	
	const char* text = "OpenBeOS SGI Image Translator";
	DrawString(text, BPoint(xbold, ybold));

	SetFont(be_plain_font);
	font_height plainh;
	GetFontHeight(&plainh);
	float yplain;
	yplain = plainh.ascent + plainh.descent * 2 + plainh.leading;
	
	char detail[100];
	sprintf(detail, "Version %d.%d.%d %s",
		SGI_TRANSLATOR_VERSION / 100, (SGI_TRANSLATOR_VERSION / 10) % 10,
		SGI_TRANSLATOR_VERSION % 10, __DATE__);
	DrawString(detail, BPoint(xbold, yplain + ybold));

	BPoint offset = fCompressionMF->Frame().LeftBottom();
	offset.x += xbold;
	offset.y += 2 * ybold;

	text = "written by:";
	DrawString(text, offset);
	offset.y += ybold;

	DrawString(author, offset);
	offset.y += 2 * ybold;

	text = "based on GIMP SGI plugin v1.5:";
	DrawString(text, offset);
	offset.y += ybold;
	
	DrawString(kSGICopyright, offset);
}

// ---------------------------------------------------------------
// Draw
//
// calculated the preferred size of this view
//
// Preconditions:
//
// Parameters: width and height
//
// Postconditions:
//
// Returns: in width and height, the preferred size...
// ---------------------------------------------------------------
void
SGIView::GetPreferredSize(float* width, float* height)
{
	*width = fCompressionMF->Bounds().Width();
	// look at the two biggest strings
	float width1 = StringWidth(kSGICopyright) + 15.0;
	if (*width < width1)
		*width = width1;
	float width2 = be_plain_font->StringWidth(author) + 15.0;
	if (*width < width2)
		*width = width2;

	font_height fh;
	be_bold_font->GetHeight(&fh);
	float ybold = fh.ascent + fh.descent * 2 + fh.leading;

	*height = fCompressionMF->Bounds().bottom + 7 * ybold;
}
