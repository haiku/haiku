// WorkWindow.cpp

#include <Application.h>
#include <Window.h>
#include <Button.h>
#include "WorkView.h"
#include "WorkWindow.h"

WorkWindow::WorkWindow(BRect rect)
	: BWindow(rect, "Work Window", B_TITLED_WINDOW, B_WILL_DRAW)
{
	SetPulseRate(1000000); // pulse once every 2 seconds
	
	WorkView *pView;
	BRect viewrect(Bounds());
	pView = new WorkView(viewrect);
	
	AddChild(pView);
}

void
WorkWindow::MessageReceived(BMessage *pMsg)
{
	switch (pMsg->what) {
		default:
			BWindow::MessageReceived(pMsg);
			break;
	}
}