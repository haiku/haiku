/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <ConnectionIncoming.h>


#define B_PULSES_BY_SECOND(x) (2*x)


namespace Bluetooth {


ConnectionView::ConnectionView(BRect frame, const char *name)
	:
	BView(BRect(0, 0, 400, 400), "MyViewName", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_PULSE_NEEDED)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


ConnectionView::~ConnectionView()
{
}


void ConnectionView::MessageReceived(BMessage *message)
{
}


void ConnectionView::Draw(BRect update)
{
}


void ConnectionView::Pulse()
{
	static int a = 0;
	
	if (a++ == B_PULSES_BY_SECOND(5)) {
		// BUG: for some reason the window is not being removed...
		Window()->PostMessage(B_QUIT_REQUESTED);
		Window()->Quit();
	}
}


//#pragma mark -


ConnectionIncoming::ConnectionIncoming(RemoteDevice* rDevice)
	:
	BWindow(BRect(700, 100, 900, 150), "Connection completed",
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	_ConnectionView = new ConnectionView(BRect(0, 0, 400, 400),"mViewName");

	AddChild(_ConnectionView);
}


ConnectionIncoming::~ConnectionIncoming()
{
}


void ConnectionIncoming::MessageReceived(BMessage *message)
{
}


bool ConnectionIncoming::QuitRequested()
{
	return BWindow::QuitRequested();
}


} /* end namespace Bluetooth */
