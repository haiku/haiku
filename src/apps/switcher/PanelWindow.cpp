/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "PanelWindow.h"

#include <stdio.h>

#include <GroupLayout.h>
#include <MessageRunner.h>
#include <Screen.h>

#include "ApplicationsView.h"
#include "GroupListView.h"
#include "Switcher.h"
#include "WindowsView.h"


static const uint32 kMsgShow = 'SHOW';
static const uint32 kMsgHide = 'HIDE';

static const int32 kMinShowState = 0;
static const int32 kMaxShowState = 4;
static const bigtime_t kMoveDelay = 15000;
	// 25 ms


PanelWindow::PanelWindow(uint32 location, uint32 which, team_id team)
	:
	BWindow(BRect(-16000, -16000, -15900, -15900), "panel",
		B_BORDERED_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_AVOID_FOCUS,
		B_ALL_WORKSPACES),
	fLocation(location),
	fShowState(kMinShowState)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	BView* child = _ViewFor(location, which, team);
	if (child != NULL)
		AddChild(child);

	Run();
	PostMessage(kMsgShow);
}


PanelWindow::~PanelWindow()
{
	BMessage message(kMsgLocationFree);
	message.AddInt32("location", fLocation);
	be_app->PostMessage(&message);
}


void
PanelWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgShow:
		case kMsgHide:
			_UpdateShowState(message->what);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


BView*
PanelWindow::_ViewFor(uint32 location, uint32 which, team_id team) const
{
	switch (which) {
		case kShowApplications:
			return new ApplicationsView(location);
		case kShowApplicationWindows:
			return new WindowsView(team, location);

		default:
			return NULL;
	}
}


void
PanelWindow::_UpdateShowState(uint32 how)
{
	if ((how == kMsgShow && fShowState >= kMaxShowState)
		|| (how == kMsgHide && fShowState <= kMinShowState))
		return;

	fShowState += how == kMsgShow ? 1 : -1;

	// Compute start and end position depending on the location
	// TODO: multi-screen support
	BScreen screen;
	BRect screenFrame = screen.Frame();
	BPoint from;
	BPoint to;

	switch (fLocation) {
		case kLeftEdge:
		case kRightEdge:
			from.y = screenFrame.top
				+ (screenFrame.Height() - Bounds().Height()) / 2.f;
			to.y = from.y;
			break;
		case kTopEdge:
		case kBottomEdge:
			from.x = screenFrame.left
				+ (screenFrame.Width() - Bounds().Width()) / 2.f;
			to.x = from.x;
			break;
	}

	switch (fLocation) {
		case kLeftEdge:
			from.x = screenFrame.left - Bounds().Width();
			to.x = screenFrame.left;
			break;
		case kRightEdge:
			from.x = screenFrame.right;
			to.x = screenFrame.right - Bounds().Width();
			break;
		case kTopEdge:
			from.y = screenFrame.top - Bounds().Height();
			to.y = screenFrame.top;
			break;
		case kBottomEdge:
			from.y = screenFrame.bottom;
			to.y = screenFrame.bottom - Bounds().Height();
			break;
	}

	MoveTo(from.x + _Factor() * (to.x - from.x),
		from.y + _Factor() * (to.y - from.y));

	if (kMsgShow && IsHidden())
		Show();
	else if (fShowState == 0 && kMsgHide && !IsHidden())
		Quit();

	if ((how == kMsgShow && fShowState < kMaxShowState)
		|| (how == kMsgHide && fShowState > kMinShowState)) {
		BMessage move(how);
		BMessageRunner::StartSending(this, &move, kMoveDelay, 1);
	} else if (how == kMsgShow) {
		// Hide the window once the mouse left its frame
		BMessage hide(kMsgHideWhenMouseMovedOut);
		hide.AddMessenger("target", this);
		hide.AddInt32("what", kMsgHide);

		// The window might not span over the whole screen, but one dimension
		// should be ignored for the cursor movements
		BRect frame = Frame();
		switch (fLocation) {
			case kLeftEdge:
			case kRightEdge:
				frame.top = screenFrame.top;
				frame.bottom = screenFrame.bottom;
				break;
			case kTopEdge:
			case kBottomEdge:
				frame.left = screenFrame.left;
				frame.right = screenFrame.right;
				break;
		}
		hide.AddRect("frame", frame);
		be_app->PostMessage(&hide);
	}
}


float
PanelWindow::_Factor()
{
	float factor = 1.f * fShowState / kMaxShowState;
	return 1 - (factor - 1) * (factor - 1);
}
