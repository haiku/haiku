/*****************************************************************************/
// SlideShowConfigView
// Written by Michael Wilber
//
// SlideShowConfigView.cpp
//
// This BView based object displays the SlideShowSaver settings options
//
//
// Copyright (C) Haiku
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
#include <FilePanel.h>
#include "SlideShowConfigView.h"

BMessage *
delay_msg(int32 option)
{
	BMessage *pMsg = new BMessage(CHANGE_DELAY);
	pMsg->AddInt32("delay", option);
	return pMsg;
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
SlideShowConfigView::SlideShowConfigView(const BRect &frame, const char *name,
	uint32 resize, uint32 flags, LiveSettings *settings)
	:	BView(frame, name, resize, flags)
{
	fSettings = settings;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BMessage *pMsg;
	int32 val;

	// Show Caption checkbox
	pMsg = new BMessage(CHANGE_CAPTION);
	fShowCaption = new BCheckBox(BRect(10, 45, 180, 62),
		"Show caption", "Show caption", pMsg);
	val = (fSettings->SetGetBool(SAVER_SETTING_CAPTION)) ? 1 : 0;
	fShowCaption->SetValue(val);
	fShowCaption->SetViewColor(ViewColor());
	AddChild(fShowCaption);

	// Change Border checkbox
	pMsg = new BMessage(CHANGE_BORDER);
	fShowBorder = new BCheckBox(BRect(10, 70, 180, 87),
		"Show border", "Show border", pMsg);
	val = (fSettings->SetGetBool(SAVER_SETTING_BORDER)) ? 1 : 0;
	fShowBorder->SetValue(val);
	fShowBorder->SetViewColor(ViewColor());
	AddChild(fShowBorder);

	// Delay Menu
	// setup PNG interlace options menu
	int32 currentDelay = fSettings->SetGetInt32(SAVER_SETTING_DELAY) / 1000;
	fDelayMenu = new BPopUpMenu("Delay menu");
	struct DelayItem {
		const char *name;
		int32 delay;
	};
	DelayItem items[] = {
		{"No delay",	0},
		{"1 second",	1},
		{"2 seconds",	2},
		{"3 seconds",	3},
		{"4 seconds",	4},
		{"5 seconds",	5},
		{"6 seconds",	6},
		{"7 seconds",	7},
		{"8 seconds",	8},
		{"9 seconds",	9},
		{"10 seconds",	10},
		{"15 seconds",	15},
		{"20 seconds",	20},
		{"30 seconds",	30},
		{"1 minute",	1 * 60},
		{"2 minutes",	2 * 60},
		{"5 minutes",	5 * 60},
		{"10 minutes",	10 * 60},
		{"15 minutes",	15 * 60}
	};
	for (uint32 i = 0; i < sizeof(items) / sizeof(DelayItem); i++) {
		BMenuItem *menuItem =
			new BMenuItem(items[i].name, delay_msg(items[i].delay));
		fDelayMenu->AddItem(menuItem);
		if (items[i].delay == currentDelay)
			menuItem->SetMarked(true);
	}
	fDelayMenuField = new BMenuField(BRect(10, 100, 180, 120),
		"Delay Menu Field", "Delay:", fDelayMenu);
	fDelayMenuField->SetViewColor(ViewColor());
	fDelayMenuField->SetDivider(40);
	AddChild(fDelayMenuField);

	// Choose Image Folder button
	pMsg = new BMessage(CHOOSE_DIRECTORY);
	fChooseFolder = new BButton(BRect(50, 160, 180, 180),
		"Choose Folder", "Choose image folder" B_UTF8_ELLIPSIS, pMsg);
	AddChild(fChooseFolder);

	// Setup choose folder file panel
	pMsg = new BMessage(CHANGE_DIRECTORY);
	fFilePanel = new BFilePanel(B_OPEN_PANEL, NULL, (const entry_ref *) NULL,
		B_DIRECTORY_NODE, false, pMsg, NULL, true, true);
	fFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
	delete pMsg;
}

// ---------------------------------------------------------------
// Destructor
//
// Releases the translator settings
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
SlideShowConfigView::~SlideShowConfigView()
{
	fSettings->Release();

	delete fFilePanel;
	fFilePanel = NULL;
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
SlideShowConfigView::AllAttached()
{
	BMessenger msgr(this);
	fShowCaption->SetTarget(msgr);
	fShowBorder->SetTarget(msgr);
	fChooseFolder->SetTarget(msgr);
	fFilePanel->SetTarget(msgr);

	// Set target for menu items
	for (int32 i = 0; i < fDelayMenu->CountItems(); i++) {
		BMenuItem *item = fDelayMenu->ItemAt(i);
		if (item)
			item->SetTarget(msgr);
	}
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
SlideShowConfigView::MessageReceived(BMessage *message)
{
	bool bNewVal;
	switch (message->what) {
		case CHANGE_CAPTION:
			if (fShowCaption->Value())
				bNewVal = true;
			else
				bNewVal = false;
			fSettings->SetGetBool(SAVER_SETTING_CAPTION, &bNewVal);
			fSettings->SaveSettings();
			break;

		case CHANGE_BORDER:
			if (fShowBorder->Value())
				bNewVal = true;
			else
				bNewVal = false;
			fSettings->SetGetBool(SAVER_SETTING_BORDER, &bNewVal);
			fSettings->SaveSettings();
			break;

		case CHOOSE_DIRECTORY:
		{
			BString strDirectory;
			fSettings->GetString(SAVER_SETTING_DIRECTORY, strDirectory);
			BEntry entry(strDirectory.String());
			if (entry.InitCheck() != B_OK)
				return;
			entry_ref ref;
			if (entry.GetRef(&ref) != B_OK)
				return;
			fFilePanel->SetPanelDirectory(&ref);

			fFilePanel->Show();
			break;
		}

		case CHANGE_DIRECTORY:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				return;
			BEntry entry(&ref, true);
			if (entry.InitCheck() != B_OK)
				return;
			BPath path(&entry);
			if (path.InitCheck() != B_OK)
				return;
			BString strDirectory = path.Path();

			fSettings->SetString(SAVER_SETTING_DIRECTORY, strDirectory);
			fSettings->SaveSettings();

			Invalidate();
			break;
		}

		case CHANGE_DELAY:
		{
			int32 newVal;
			if (message->FindInt32("delay", &newVal) == B_OK) {
				newVal *= 1000;
				fSettings->SetGetInt32(SAVER_SETTING_DELAY, &newVal);
				fSettings->SaveSettings();
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}

// ---------------------------------------------------------------
// Draw
//
// Draws information about the SlideShowConfigTranslator to this view.
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
SlideShowConfigView::Draw(BRect area)
{
	SetFont(be_bold_font);
	font_height fh;
	GetFontHeight(&fh);
	float xbold, ybold;
	xbold = fh.descent + 1;
	ybold = fh.ascent + fh.descent * 2 + fh.leading;

	char title[] = "SlideShow Screen Saver";
	DrawString(title, BPoint(xbold, ybold));

	SetFont(be_plain_font);
	font_height plainh;
	GetFontHeight(&plainh);
	float yplain;
	yplain = plainh.ascent + plainh.descent * 2 + plainh.leading;

	char writtenby[] = "Written by Michael Wilber";
	DrawString(writtenby, BPoint(xbold, yplain * 1 + ybold));

	// Draw current folder
	BString strFolder;
	fSettings->GetString(SAVER_SETTING_DIRECTORY, strFolder);
	strFolder.Prepend("Image folder: ");
	DrawString(strFolder.String(), BPoint(10, yplain * 9 + ybold));
}
