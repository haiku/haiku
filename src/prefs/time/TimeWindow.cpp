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

#include "BaseView.h"
#include "Time.h"
#include "TimeMessages.h"
#include "TimeView.h"
#include "TimeWindow.h"

#define TIME_WINDOW_RIGHT	361 //332
#define TIME_WINDOW_BOTTOM	227 //208


TTimeWindow::TTimeWindow()
	: BWindow(BRect(0,0,TIME_WINDOW_RIGHT,TIME_WINDOW_BOTTOM), 
		"Time & Date", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
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
{
}


void 
TTimeWindow::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case H_USER_CHANGE:
		{
			bool istime;
			if (message->FindBool("time", &istime) == B_OK)
				f_BaseView->ChangeTime(message);
		}
		break;
		case H_RTC_CHANGE:
		{
			f_BaseView->SetGMTime(f_TimeSettings->GMTime());
		}
		break;
		
		default:
			BWindow::MessageReceived(message);
		break;
	}
}


bool 
TTimeWindow::QuitRequested()
{
	dynamic_cast<TimeApplication *>(be_app)->SetWindowCorner(BPoint(Frame().left,Frame().top));
	
	f_BaseView->StopWatchingAll(f_TimeSettings);
	f_BaseView->StopWatchingAll(f_TimeZones);
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return(true);
	
}


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
	
	f_TimeSettings = new TSettingsView(bounds);
	if (f_BaseView->StartWatchingAll(f_TimeSettings) != B_OK)
		printf("StartWatchingAll(TimeSettings) failed!!!");

	f_TimeZones = new TZoneView(bounds);
	if (f_BaseView->StartWatchingAll(f_TimeZones) != B_OK)
		printf("TimeZones->StartWatchingAll(TimeZone) failed!!!");	
	// add tabs
	BTab *tab;
	tab = new BTab();
	tabview->AddTab(f_TimeSettings, tab);
	tab->SetLabel("Settings");
	
	tab = new BTab();
	tabview->AddTab(f_TimeZones, tab);
	tab->SetLabel("Time Zone");
	
	f_BaseView->AddChild(tabview);
	f_BaseView->Pulse();
	
}
