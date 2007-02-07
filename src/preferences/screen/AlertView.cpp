/*
 * Copyright 2001-2006, Haiku.
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
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fBitmap = InitIcon();

	BRect rect(60, 8, 400, 36);
	BStringView *stringView = new BStringView(rect, NULL,
		"Do you wish to keep these settings?");
	stringView->SetFont(be_bold_font);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect = stringView->Frame();
	rect.OffsetBy(0, rect.Height());
	fCountdownView = new BStringView(rect, "countdown", NULL);
	UpdateCountdownView();
	fCountdownView->ResizeToPreferred();
	AddChild(fCountdownView);

	BButton* keepButton = new BButton(rect, "keep", "Keep",
		new BMessage(BUTTON_KEEP_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	keepButton->ResizeToPreferred();
	AddChild(keepButton);

	BButton* button = new BButton(rect, "undo", "Undo",
		new BMessage(BUTTON_UNDO_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	AddChild(button);

	// we're resizing ourselves to the right size
	// (but we're not implementing GetPreferredSize(), bad style!)
	float width = stringView->Frame().right;
	if (fCountdownView->Frame().right > width)
		width = fCountdownView->Frame().right;
	if (width < Bounds().Width())
		width = Bounds().Width();

	float height = fCountdownView->Frame().bottom + 24 + button->Bounds().Height();
	ResizeTo(width, height);

	keepButton->MoveTo(Bounds().Width() - 8 - keepButton->Bounds().Width(),
		Bounds().Height() - 8 - keepButton->Bounds().Height());
	button->MoveTo(keepButton->Frame().left - button->Bounds().Width() - 8,
		keepButton->Frame().top);

	keepButton->MakeDefault(true);
}


void
AlertView::AttachedToWindow()
{
	// the view displays a decrementing counter (until the user must take action)
	Window()->SetPulseRate(1000000);
		// every second

	SetEventMask(B_KEYBOARD_EVENTS);
}


void
AlertView::Draw(BRect updateRect)
{
	rgb_color dark = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT);
	SetHighColor(dark);

	FillRect(BRect(0.0, 0.0, 30.0, Bounds().bottom));

	if (fBitmap != NULL) {
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fBitmap, BPoint(18.0, 6.0));
		SetDrawingMode(B_OP_COPY);
	}
}


void
AlertView::Pulse()
{
	if (--fSeconds == 0)
		Window()->PostMessage(BUTTON_UNDO_MSG);
	else
		UpdateCountdownView();
}


void
AlertView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1 && bytes[0] == B_ESCAPE)
		Window()->PostMessage(BUTTON_UNDO_MSG);
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

