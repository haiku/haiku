// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        BottomlineWindow.cpp
//  Author:      Jérôme Duval
//  Description: Input server bottomline window
//  Created :    January 24, 2005
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include "BottomlineWindow.h"
#include "InputServer.h"

BottomlineWindow::BottomlineWindow(const BFont *font)
	: BWindow(BRect(0,0,350,16), "", 
		(window_look) 25/*B_FLOATING_WINDOW_LOOK*/, 
				B_FLOATING_ALL_WINDOW_FEEL,
				B_NOT_V_RESIZABLE | B_NOT_CLOSABLE |
				B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AVOID_FOCUS |
				B_WILL_ACCEPT_FIRST_CLICK)
{
	BRect textRect = Bounds();
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(2,2);
	fTextView = new BTextView(Bounds(), "", textRect, font, NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
	AddChild(fTextView);

	fTextView->SetText("");
	fTextView->MakeFocus(true);

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
BottomlineWindow::HandleInputMethodEvent(BMessage *msg, BList *list)
{
	CALLED();

	PostMessage(msg, fTextView);

	int32 opcode = -1;
	const char* string = NULL;
	
	msg->FindInt32("be:opcode", &opcode);
	msg->FindString("be:string", &string);

	SERIAL_PRINT(("IME : %i, %s\n", opcode, string));
	if (opcode != B_INPUT_METHOD_CHANGED)
		return;

	bool confirmed = false;
	if ((msg->FindBool("be:confirmed", &confirmed) != B_OK)
		|| !confirmed) 
		return;

	SERIAL_PRINT(("IME : confirmed\n"));
			
	if (string != NULL) {
		int32 offset = 0;
		int32 length = strlen(string);
		int32 next_offset = 0;

		while (offset < length) {
			for (++next_offset; (string[next_offset] & 0xC0) == 0x80; ++next_offset)
				;

			BMessage *newmsg = new BMessage(B_KEY_DOWN);
			newmsg->AddInt32("key", 0);
			newmsg->AddInt64("when", system_time());
			newmsg->AddData("bytes", B_STRING_TYPE, string + offset, next_offset - offset);
			list->AddItem(newmsg);
			offset = next_offset;
		}
	}
}

