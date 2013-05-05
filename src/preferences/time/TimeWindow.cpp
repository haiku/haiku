/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Julun <host.haiku@gmx.de>
 *		Hamish Morrison <hamish@lavabit.com>
 */

#include "TimeWindow.h"

#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Screen.h>
#include <TabView.h>

#include "BaseView.h"
#include "ClockView.h"
#include "DateTimeView.h"
#include "NetworkTimeView.h"
#include "TimeMessages.h"
#include "TimeSettings.h"
#include "ZoneView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Time"

TTimeWindow::TTimeWindow()
	:
	BWindow(BRect(0, 0, 0, 0), B_TRANSLATE_SYSTEM_NAME("Time"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	_InitWindow();
	_AlignWindow();

	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));
}


TTimeWindow::~TTimeWindow()
{
}


bool
TTimeWindow::QuitRequested()
{
	TimeSettings().SetLeftTop(Frame().LeftTop());

	fBaseView->StopWatchingAll(fTimeZoneView);
	fBaseView->StopWatchingAll(fDateTimeView);

	be_app->PostMessage(B_QUIT_REQUESTED);

	return BWindow::QuitRequested();
}


void
TTimeWindow::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case H_USER_CHANGE:
			fBaseView->ChangeTime(message);
			// To make sure no old time message is in the queue
			_SendTimeChangeFinished();
			_SetRevertStatus();
			break;

		case B_ABOUT_REQUESTED:
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;

		case kMsgRevert:
			fDateTimeView->MessageReceived(message);
			fTimeZoneView->MessageReceived(message);
			fNetworkTimeView->MessageReceived(message);
			fClockView->MessageReceived(message);
			fRevertButton->SetEnabled(false);
			break;

		case kRTCUpdate:
			fDateTimeView->MessageReceived(message);
			fTimeZoneView->MessageReceived(message);
			_SetRevertStatus();
			break;

		case kMsgChange:
			_SetRevertStatus();
			break;

		case kSelectClockTab:
			// focus the clock tab (last one)
			fTabView->Select(fTabView->CountTabs() - 1);
			break;

		case kShowHideTime:
			fClockView->MessageReceived(message);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
TTimeWindow::_InitWindow()
{
	SetPulseRate(500000);

	fDateTimeView = new DateTimeView(B_TRANSLATE("Date and time"));
	fTimeZoneView = new TimeZoneView(B_TRANSLATE("Time zone"));
	fNetworkTimeView = new NetworkTimeView(B_TRANSLATE("Network time"));
	fClockView = new ClockView(B_TRANSLATE("Clock"));

	fBaseView = new TTimeBaseView("baseView");
	fBaseView->StartWatchingAll(fDateTimeView);
	fBaseView->StartWatchingAll(fTimeZoneView);

	fTabView = new BTabView("tabView");
	fTabView->AddTab(fDateTimeView);
	fTabView->AddTab(fTimeZoneView);
	fTabView->AddTab(fNetworkTimeView);
	fTabView->AddTab(fClockView);

	fBaseView->AddChild(fTabView);

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);
	fRevertButton->SetTarget(this);
	fRevertButton->SetExplicitAlignment(
		BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fBaseView)
		.Add(fRevertButton)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
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


void
TTimeWindow::_SendTimeChangeFinished()
{
	BMessenger messenger(fDateTimeView);
	BMessage msg(kChangeTimeFinished);
	messenger.SendMessage(&msg);
}


void
TTimeWindow::_SetRevertStatus()
{
	fRevertButton->SetEnabled(fDateTimeView->CheckCanRevert()
		|| fTimeZoneView->CheckCanRevert()
		|| fNetworkTimeView->CheckCanRevert()
		|| fClockView->CheckCanRevert());
}
