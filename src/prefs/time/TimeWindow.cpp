/*
 * TimeWindow.cpp
 * Time mccall@@digitalparadise.co.uk
 *	
 */
 
#include <Application.h>
#include <Message.h>
#include <Screen.h>
#include <TabView.h>

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
	//BSlider *slider=NULL;

	MoveTo(dynamic_cast<TimeApplication *>(be_app)->WindowCorner());

	// Code to make sure that the window doesn't get drawn off screen...
	if (!(screen.Frame().right >= Frame().right && screen.Frame().bottom >= Frame().bottom))
		MoveTo((screen.Frame().right-Bounds().right)*.5,(screen.Frame().bottom-Bounds().bottom)*.5);
	
	BuildViews(); 
	SetPulseRate(500000);		//pulse rate. Mattias Sundblad
	Show();

}//TimeWindow::TimeWindow()

void TimeWindow::BuildViews()
{
	BRect viewRect= Bounds();
	
	//the views:
	fTimeSettings = new SettingsView(viewRect);
	fTimeZones = new ZoneView(viewRect);
	
	//The tabs:
	BTabView		*fTabView;
	BTab			*fTab;
	fTabView = new BTabView(viewRect, "tab_view");

	// Settings Tab;
	fTab = new BTab();
	fTabView -> AddTab(fTimeSettings,fTab);
	fTab -> SetLabel("Settings");
	
	//Time zone tab:
	fTab = new BTab();
	fTabView -> AddTab(fTimeZones,fTab);
	fTab -> SetLabel("Time Zone");
	
	AddChild(fTabView);
	
}//TimeWindow::BuildViews()

bool TimeWindow::QuitRequested()
{

	dynamic_cast<TimeApplication *>(be_app)->SetWindowCorner(BPoint(Frame().left,Frame().top));
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return(true);
} //TimeWindow::QuitRequested()

void TimeWindow::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case REGION_CHANGED:
			fTimeZones -> ChangeRegion(message);
		break;
		case SET_TIME_ZONE:
			fTimeZones -> setTimeZone();
		break;
		case TIME_ZONE_CHANGED:
			fTimeZones -> newTimeZone();
		break;	
		default:
			BWindow::MessageReceived(message);
		break;
	}
	
}//TimeWindow::MessageReceived(BMessage *message)

TimeWindow::~TimeWindow()
{

}//TimeWindow::~TimeWindow()


