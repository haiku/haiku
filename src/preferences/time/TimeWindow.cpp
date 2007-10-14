/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Julun <host.haiku@gmx.de>
 *
 */

#include "TimeWindow.h"
#include "BaseView.h"
#include "SettingsView.h"
#include "TimeMessages.h"
#include "ZoneView.h"


#include <Application.h>
#include <Message.h>
#include <Screen.h>
#include <TabView.h>


#define WINDOW_RIGHT	470
#define WINDOW_BOTTOM	227


TTimeWindow::TTimeWindow(const BPoint leftTop)
	: BWindow(BRect(leftTop, leftTop + BPoint(WINDOW_RIGHT, WINDOW_BOTTOM)),
		"Time & Date", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect frame = Frame();
	BRect bounds = Bounds();
	BRect screenFrame = BScreen().Frame();
	// Code to make sure that the window doesn't get drawn off screen...
	if (!(screenFrame.right >= frame.right && screenFrame.bottom >= frame.bottom))
		MoveTo((screenFrame.right - bounds.right) * 0.5f, 
			(screenFrame.bottom - bounds.bottom) * 0.5f);
	
	_InitWindow(); 
	SetPulseRate(500000);
}


void 
TTimeWindow::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case H_USER_CHANGE:
			fBaseView->ChangeTime(message);
			break;
		
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool 
TTimeWindow::QuitRequested()
{
	BMessage msg(UPDATE_SETTINGS);
	msg.AddPoint("LeftTop", Frame().LeftTop());
	be_app->PostMessage(&msg);
	
	fBaseView->StopWatchingAll(fTimeSettings);
	fBaseView->StopWatchingAll(fTimeZones);
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return BWindow::QuitRequested();
}


void 
TTimeWindow::_InitWindow()
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
	fBaseView->StartWatchingAll(fTimeSettings);

	fTimeZones = new TZoneView(bounds);
	fBaseView->StartWatchingAll(fTimeZones);

	// add tabs
	BTab *tab = new BTab();
	tabview->AddTab(fTimeSettings, tab);
	tab->SetLabel("Settings");
	
	tab = new BTab();
	tabview->AddTab(fTimeZones, tab);
	tab->SetLabel("Time Zone");
	
	fBaseView->AddChild(tabview);

	float width;
	float height;
	// width/ height from settingsview + all InsetBy etc..
	fTimeSettings->GetPreferredSize(&width, &height);
	ResizeTo(width +10, height + tabview->TabHeight() +25);
}

