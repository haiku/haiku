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
#include "DateTimeView.h"
#include "TimeMessages.h"
#include "TimeSettings.h"
#include "ZoneView.h"


#include <Application.h>
#include <Message.h>
#include <Screen.h>
#include <TabView.h>


TTimeWindow::TTimeWindow(BRect rect)
	: BWindow(rect, "Time", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	_InitWindow();
	_AlignWindow();

	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));
}


TTimeWindow::~TTimeWindow()
{
}


void 
TTimeWindow::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case H_USER_CHANGE:
			fBaseView->ChangeTime(message);
			break;
		
		case B_ABOUT_REQUESTED:
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool 
TTimeWindow::QuitRequested()
{
	TimeSettings().SetLeftTop(Frame().LeftTop());

	fBaseView->StopWatchingAll(fTimeZones);
	fBaseView->StopWatchingAll(fDateTimeView);
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return BWindow::QuitRequested();
}


void 
TTimeWindow::_InitWindow()
{
	SetPulseRate(500000);

	BRect bounds(Bounds());
	
	fBaseView = new TTimeBaseView(bounds, "background view");
	AddChild(fBaseView);
	
	bounds.top = 9;
	BTabView *tabview = new BTabView(bounds, "tab_view");
	
	bounds = tabview->Bounds();
	bounds.InsetBy(4, 6);
	bounds.bottom -= tabview->TabHeight();
	
	fDateTimeView = new DateTimeView(bounds);
	fBaseView->StartWatchingAll(fDateTimeView);

	BTab *tab = new BTab();
	tabview->AddTab(fDateTimeView, tab);
	tab->SetLabel("Date & Time");

	fTimeZones = new TZoneView(bounds);
	fBaseView->StartWatchingAll(fTimeZones);

	tab = new BTab();
	tabview->AddTab(fTimeZones, tab);
	tab->SetLabel("Time Zone");

	fBaseView->AddChild(tabview);

	float width;
	float height;
	fDateTimeView->GetPreferredSize(&width, &height);

	// width/ height from DateTimeView + all InsetBy etc..
	ResizeTo(width +10, height + tabview->TabHeight() +25);
}


void
TTimeWindow::_AlignWindow()
{
	BPoint pt = TimeSettings().LeftTop();
	MoveTo(pt);

	BRect frame = Frame();
	BRect screen = BScreen().Frame();
	if (!frame.Intersects(screen.InsetByCopy(50.0, 50.0))) {
		BRect bounds(Bounds());
		BPoint leftTop((screen.Width() - bounds.Width()) / 2.0,
			(screen.Height() - bounds.Height()) / 2.0);

		MoveTo(leftTop);
	}
}
