/*
 * TTimeWindow.cpp
 * Time mccall@@digitalparadise.co.uk
 *	
 */
 
#include <Application.h>
#include <Message.h>
#include <Screen.h>
#include <TabView.h>

#include <stdio.h>

#include "TimeMessages.h"
#include "TimeWindow.h"
#include "TimeView.h"
#include "Time.h"
#include "BaseView.h"

#define TIME_WINDOW_RIGHT	361 //332
#define TIME_WINDOW_BOTTOM	227 //208

TTimeWindow::TTimeWindow()
	: BWindow(BRect(0,0,TIME_WINDOW_RIGHT,TIME_WINDOW_BOTTOM), "Time & Date", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	BScreen screen;

	MoveTo(dynamic_cast<TimeApplication *>(be_app)->WindowCorner());

	// Code to make sure that the window doesn't get drawn off screen...
	if (!(screen.Frame().right >= Frame().right && screen.Frame().bottom >= Frame().bottom))
		MoveTo((screen.Frame().right-Bounds().right)*.5,(screen.Frame().bottom-Bounds().bottom)*.5);
	
	InitWindow(); 
	SetPulseRate(500000);
}

TTimeWindow::~TTimeWindow()
{ /* empty */ }

void 
TTimeWindow::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case REGION_CHANGED:
			fTimeZones->ChangeRegion(message);
		break;
		case SET_TIME_ZONE:
			fTimeZones->SetTimeZone();
		break;
		case TIME_ZONE_CHANGED:
			fTimeZones->NewTimeZone();
		break;	
		default:
			BWindow::MessageReceived(message);
		break;
	}
	
}//TTimeWindow::MessageReceived(BMessage *message)

bool 
TTimeWindow::QuitRequested()
{
	dynamic_cast<TimeApplication *>(be_app)->SetWindowCorner(BPoint(Frame().left,Frame().top));
	
	f_BaseView->StopWatchingAll(fTimeSettings);
	f_BaseView->StopWatchingAll(fTimeZones);
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return(true);
	
} //TTimeWindow::QuitRequested()

void 
TTimeWindow::InitWindow()
{
	BRect bounds(Bounds());
	
	f_BaseView = new TTimeBaseView(bounds, "background view");
	f_BaseView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(f_BaseView);
	
	bounds.top = 9;
	BTabView *tabview = new BTabView(bounds, "tab_view");
	
	bounds = tabview->Bounds();
	bounds.InsetBy(4, 6);
	bounds.bottom -= tabview->TabHeight();
	
	fTimeSettings = new TSettingsView(bounds);
	if (f_BaseView->StartWatchingAll(fTimeSettings) != B_OK)
		printf("StartWatchingAll(TimeSettings) failed!!!");

	fTimeZones = new TZoneView(bounds);
	if (f_BaseView->StartWatchingAll(fTimeZones) != B_OK)
		printf("TimeZones->StartWatchingAll(TimeZone) failed!!!");
	
	
	// add tabs
	BTab *tab;
	tab = new BTab();
	tabview->AddTab(fTimeSettings, tab);
	tab->SetLabel("Settings");
	
	tab = new BTab();
	tabview->AddTab(fTimeZones, tab);
	tab->SetLabel("Time Zone");
	
	f_BaseView->AddChild(tabview);
	f_BaseView->Pulse();
	
}//TTimeWindow::BuildViews()
