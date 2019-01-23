/*
 * Copyright 2007 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *		Ryan Leavengood, leavengood@gmail.com
 */


#include "MessageWin.h"

#include <Box.h>
#include <Message.h>
#include <TextView.h>
#include <View.h>

/* frame will be the frame of the parent window as*/
MessageWin::MessageWin(BRect parentFrame, const char *title,
	window_look look, window_feel feel, uint32 flags, uint32 workspace)
	: BWindow(parentFrame ,title ,look ,feel, flags, workspace)
{
	fBox = new BBox(Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	fBox->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fBox->SetLowColor(fBox->ViewColor());

	// Rects for the text view
	BRect outside(fBox->Bounds());
	outside.InsetBy(10, 10);
	BRect insider(outside);
	insider.OffsetTo(B_ORIGIN);

	fText = new BTextView(outside, "message", insider, B_FOLLOW_NONE, B_WILL_DRAW);
	fText->MakeEditable(false);
	fText->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fText->SetLowColor(fText->ViewColor());

	fBox->AddChild(fText);
 	AddChild(fBox);

 	/* Relocate the window to the center of what its being given */
  	ResizeTo(parentFrame.Width(), floor(parentFrame.Height() / 3));
 	MoveBy(0, floor(parentFrame.Height() / 2 - (parentFrame.Height()/3) / 2 ));

}


void MessageWin::SetText(const char* str)
{
	Lock();
	fText->SetText(str);
	fText->Flush();
	Unlock();
}


void MessageWin::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
		default:
			BWindow::MessageReceived(message);
			break;
	}	
}


bool MessageWin::QuitRequested()
{
	return BWindow::QuitRequested();
}

