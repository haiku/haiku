/*
 * TimeWindow.cpp
 * Time mccall@digitalparadise.co.uk
 *
 */
 
#include <Application.h>
#include <Message.h>
#include <Screen.h>

#include "TimeMessages.h"
#include "TimeWindow.h"
#include "TimeView.h"
#include "Time.h"

#define TIME_WINDOW_RIGHT	332
#define TIME_WINDOW_BOTTTOM	208

TimeWindow::TimeWindow()
				: BWindow(BRect(0,0,TIME_WINDOW_RIGHT,TIME_WINDOW_BOTTTOM), "Time & Date", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	BScreen screen;
	BSlider *slider=NULL;

	MoveTo(dynamic_cast<TimeApplication *>(be_app)->WindowCorner());

	// Code to make sure that the window doesn't get drawn off screen...
	if (!(screen.Frame().right >= Frame().right && screen.Frame().bottom >= Frame().bottom))
		MoveTo((screen.Frame().right-Bounds().right)*.5,(screen.Frame().bottom-Bounds().bottom)*.5);
	
	BuildView();
	AddChild(fView);

	Show();

}

void
TimeWindow::BuildView()
{
	fView = new TimeView(Bounds());	
}

bool
TimeWindow::QuitRequested()
{

	dynamic_cast<TimeApplication *>(be_app)->SetWindowCorner(BPoint(Frame().left,Frame().top));
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return(true);
}

void
TimeWindow::MessageReceived(BMessage *message)
{
			
	switch(message->what) {
		default:
			BWindow::MessageReceived(message);
			break;
	}
	
}

TimeWindow::~TimeWindow()
{

}
