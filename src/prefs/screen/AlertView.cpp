/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "AlertView.h"
#include "Constants.h"

#include <Window.h>
#include <Bitmap.h>
#include <Button.h>
#include <StringView.h>
#include <String.h>

#include <FindDirectory.h>
#include <Resources.h>
#include <File.h>
#include <Path.h>


AlertView::AlertView(BRect frame, char *name)
	: BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED),
	// we will wait 8 seconds until we send a message
	fSeconds(8)
{
	fBitmap = InitIcon();

	BStringView *stringView = new BStringView(BRect(60, 20, 400, 36), NULL,
		"Do you wish to keep these settings?");
	stringView->SetFont(be_bold_font);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	fCountdownView = new BStringView(BRect(60, 37, 400, 50), "countdown",
		B_EMPTY_STRING);
	UpdateCountdownView();
	fCountdownView->ResizeToPreferred();
	AddChild(fCountdownView);

	BButton *button = new BButton(BRect(215, 59, 400, 190), "keep", "Keep",
		new BMessage(BUTTON_KEEP_MSG));
	button->ResizeToPreferred();
	AddChild(button);

	button = new BButton(BRect(130, 59, 400, 199), "revert", "Revert",
		new BMessage(BUTTON_REVERT_MSG));
	button->ResizeToPreferred();
	AddChild(button);

	SetEventMask(B_KEYBOARD_EVENTS);
}


void
AlertView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	if (BButton* button = dynamic_cast<BButton *>(FindView("keep")))
		Window()->SetDefaultButton(button);
}


void
AlertView::Draw(BRect updateRect)
{	
	rgb_color dark = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT);
	SetHighColor(dark);	

	FillRect(BRect(0.0, 0.0, 30.0, 100.0));

	if (fBitmap != NULL) {
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fBitmap, BPoint(18.0, 6.0));
	}
}


void 
AlertView::Pulse()
{
	if (--fSeconds == 0)
		Window()->PostMessage(BUTTON_REVERT_MSG);
	else
		UpdateCountdownView();
}


void
AlertView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1 && bytes[0] == B_ESCAPE)
		Window()->PostMessage(BUTTON_REVERT_MSG);
}


void 
AlertView::UpdateCountdownView()
{
	BString string;
	string << "Settings will revert in " << fSeconds << " seconds.";
	fCountdownView->SetText(string.String());
}


BBitmap*
AlertView::InitIcon()
{
	// This is how BAlert gets to its icon
	BBitmap* icon = NULL;
	BPath path;
	if (find_directory(B_BEOS_SERVERS_DIRECTORY, &path) == B_OK) {
		path.Append("app_server");
		BResources resources;
		BFile file;
		if (file.SetTo(path.Path(), B_READ_ONLY) == B_OK
			&& resources.SetTo(&file) == B_OK) {
			// Load the raw icon data
			size_t size;
			const void* data = resources.LoadResource('ICON', "warn", &size);
			if (data) {
				// Now build the bitmap
				icon = new BBitmap(BRect(0, 0, 31, 31), 0, B_CMAP8);
				icon->SetBits(data, size, 0, B_CMAP8);
			}
		}
	}

	return icon;
}

