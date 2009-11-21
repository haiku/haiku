/*
 * Copyright 2004-2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */


#include "BottomlineWindow.h"
#include "WindowPrivate.h"

#include <String.h>
#include <TextView.h>


BottomlineWindow::BottomlineWindow()
	: BWindow(BRect(0, 0, 350, 16), "", 
		kLeftTitledWindowLook, 
		B_FLOATING_ALL_WINDOW_FEEL,
		B_NOT_V_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
			| B_AVOID_FOCUS | B_WILL_ACCEPT_FIRST_CLICK)
{
	BRect textRect = Bounds();
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(2,2);
	fTextView = new BTextView(Bounds(), "", textRect, be_plain_font,
		NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
	AddChild(fTextView);

	fTextView->SetText("");

	BRect   screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint pt;
	pt.x = 100;
	pt.y = screenFrame.Height()*2/3 - Bounds().Height()/2;	
	
	MoveTo(pt);
	Show();

	SERIAL_PRINT(("BottomlineWindow created\n"));
}


BottomlineWindow::~BottomlineWindow()
{


}


void
BottomlineWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


bool
BottomlineWindow::QuitRequested()
{
	return true;
}


void 
BottomlineWindow::HandleInputMethodEvent(BMessage* event, EventList& newEvents)
{
	CALLED();

	PostMessage(event, fTextView);

	const char* string;
	bool confirmed;
	int32 opcode;
	if (event->FindInt32("be:opcode", &opcode) != B_OK
		|| opcode != B_INPUT_METHOD_CHANGED
		|| event->FindBool("be:confirmed", &confirmed) != B_OK
		|| !confirmed
		|| event->FindString("be:string", &string) != B_OK) 
		return;

	SERIAL_PRINT(("IME : %i, %s\n", opcode, string));
	SERIAL_PRINT(("IME : confirmed\n"));

	int32 length = strlen(string);
	int32 offset = 0;
	int32 nextOffset = 0;

	while (offset < length) {
		// this is supposed to go to the next UTF-8 character
		for (++nextOffset; (string[nextOffset] & 0xC0) == 0x80; ++nextOffset)
			;

		BMessage *newEvent = new BMessage(B_KEY_DOWN);
		if (newEvent != NULL) {
			newEvent->AddInt32("key", 0);
			newEvent->AddInt64("when", system_time());
			BString bytes(string + offset, nextOffset - offset);
			newEvent->AddString("bytes", bytes);
			newEvent->AddInt32("raw_char", 0xa);
			newEvents.AddItem(newEvent);
		}

		offset = nextOffset;
	}
}

