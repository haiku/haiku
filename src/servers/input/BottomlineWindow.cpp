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
	: BWindow(BRect(0,0,350,20), "", 
		(window_look) 25/*B_FLOATING_WINDOW_LOOK*/, 
				B_FLOATING_ALL_WINDOW_FEEL,
				B_NOT_V_RESIZABLE | B_NOT_CLOSABLE |
				B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AVOID_FOCUS |
				B_WILL_ACCEPT_FIRST_CLICK)
{
	BRect textRect = Bounds();
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(1,1);
	fTextView = new BTextView(Bounds(), "", textRect, font, NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
	AddChild(fTextView);

	fTextView->SetText("coucou");

	BRect   screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint pt;
	pt.x = 100;
	pt.y = screenFrame.Height()*2/3 - Bounds().Height()/2;	
	
	MoveTo(pt);
	Show();
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
}

