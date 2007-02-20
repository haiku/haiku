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
#include "SettingsView.h"
#include "Time.h"
#include "TimeMessages.h"
#include "TimeWindow.h"
#include "TimeSettings.h"
#include "ZoneView.h"

#define TIME_WINDOW_RIGHT	361 //332
#define TIME_WINDOW_BOTTOM	227 //208


TTimeWindow::TTimeWindow()
	: BWindow(BRect(0, 0, TIME_WINDOW_RIGHT, TIME_WINDOW_BOTTOM), 
		"Time & Date", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	MoveTo(dynamic_cast<TimeApplication *>(be_app)->WindowCorner());

	BScreen screen;
	// Code to make sure that the window doesn't get drawn off screen...
	if (!(screen.Frame().right >= Frame().right && screen.Frame().bottom >= Frame().bottom))
		MoveTo((screen.Frame().right-Bounds().right)*.5,(screen.Frame().bottom-Bounds().bottom)*.5);
	
	InitWindow(); 
	SetPulseRate(500000);
}


void 
TTimeWindow::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case H_USER_CHANGE:
		{
			bool istime;
			if (message->FindBool("time", &istime) == B_OK)
				fBaseView->ChangeTime(message);
			break;
		}
		
		case H_RTC_CHANGE:
			fBaseView->SetGMTime(fTimeSettings->GMTime());
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
	
	fBaseView->StopWatchingAll(fTimeSettings);
	fBaseView->StopWatchingAll(fTimeZones);
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return BWindow::QuitRequested();
	
}


void 
TTimeWindow::InitWindow()
{
	BRect bounds(Bounds());
	
	fBaseView = new TTimeBaseView(bounds, "background view");
	AddChild(fBaseView);
	
	bounds.top = 9;
	BTabView *tabview = new BTabView(bounds, "tab_view");
	
	bounds = tabview->Bounds();
	bounds.InsetBy(4, 6);
	bounds.bottom -= tabview->TabHeight();
	
	fTimeSettings = new TSettingsView(bounds);
	if (fBaseView->StartWatchingAll(fTimeSettings) != B_OK)
		printf("StartWatchingAll(TimeSettings) failed!!!\n");

	fTimeZones = new TZoneView(bounds);
	if (fBaseView->StartWatchingAll(fTimeZones) != B_OK)
		printf("TimeZones->StartWatchingAll(TimeZone) failed!!!\n");
	
	// add tabs
	BTab *tab = new BTab();
	tabview->AddTab(fTimeSettings, tab);
	tab->SetLabel("Settings");
	
	tab = new BTab();
	tabview->AddTab(fTimeZones, tab);
	tab->SetLabel("Time Zone");
	
	fBaseView->AddChild(tabview);
}
