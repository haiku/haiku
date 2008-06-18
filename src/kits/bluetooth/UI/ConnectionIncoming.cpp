/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <ConnectionIncoming.h>

#define B_PULSES_BY_SECOND(x) (2*x)


ConnectionView::ConnectionView(BRect frame, const char *name, uint32 resizeMask, uint32 flags)
		: BView(BRect(0, 0, 400, 400), "MyViewName", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_PULSE_NEEDED)
{

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

}

ConnectionView::~ConnectionView()
{

}


void ConnectionView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	default:
	
	break;
	}
}


void ConnectionView::Draw(BRect update)
{

}


void ConnectionView::Pulse()
{
	static int a = 0;
	if (a++ == B_PULSES_BY_SECOND(5))
		Window()->QuitRequested();

}



//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
ConnectionIncoming::ConnectionIncoming(RemoteDevice* rDevice)
			 : BWindow(BRect(700, 100, 900, 150), "Incomming Connection", 
                                   B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
                                   B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
_ConnectionView = new ConnectionView(BRect(0, 0, 400, 400),"mViewName", B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW);

AddChild(_ConnectionView);
}

//---------------------------------------------------------------
ConnectionIncoming::~ConnectionIncoming()
{

}

//---------------------------------------------------------------
void ConnectionIncoming::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
	default:
	
	break;
	}
}

//---------------------------------------------------------------
bool ConnectionIncoming::QuitRequested()
{
	Quit();
	return BWindow::QuitRequested();
}

