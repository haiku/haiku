/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "CaptureWindow.h"

#include <stdio.h>

#include <Roster.h>
#include <Screen.h>

#include "Switcher.h"


static const bigtime_t kUpdateDelay = 50000;
	// 50 ms


class CaptureView : public BView {
public:
								CaptureView();
	virtual						~CaptureView();

			void				AddMoveOutNotification(BMessage* message);

	virtual	void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* dragMessage);
	virtual void				KeyDown(const char* bytes, int32 numBytes);

private:
			void				_UpdateLast(const BPoint& point);
			void				_Notify(uint32 location, team_id team);
			void				_SendMovedOutNotification();

	static	team_id				_CurrentTeam();

private:
			uint32				fModifierMask;
			BPoint				fLastPoint;
			team_id				fLastTeam;
			bigtime_t			fLastEvent;

			uint32				fMovedOutWhat;
			BMessenger			fMovedOutMessenger;
			BRect				fMovedOutFrame;
};


CaptureView::CaptureView()
	:
	BView("main", 0),
	fModifierMask(B_CONTROL_KEY),
	fMovedOutWhat(0)
{
	SetEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS, B_NO_POINTER_HISTORY);
	_UpdateLast(BPoint(-1, -1));
}


CaptureView::~CaptureView()
{
}


void
CaptureView::AddMoveOutNotification(BMessage* message)
{
	if (fMovedOutWhat != 0)
		_SendMovedOutNotification();

	if (message->FindMessenger("target", &fMovedOutMessenger) == B_OK
		&& message->FindRect("frame", &fMovedOutFrame) == B_OK)
		fMovedOutWhat = (uint32)message->FindInt32("what");
}


void
CaptureView::MouseMoved(BPoint point, uint32 transit,
	const BMessage* dragMessage)
{
	ConvertToScreen(&point);

	if (fMovedOutWhat != 0 && !fMovedOutFrame.Contains(point))
		_SendMovedOutNotification();

	uint32 modifiers = ::modifiers();
	if ((modifiers & fModifierMask) == 0) {
		_UpdateLast(point);
		return;
	}

	// TODO: we will need to iterate over all existing screens to find the
	// right one!
	BScreen screen;
	BRect screenFrame = screen.Frame();

	uint32 location = kNowhere;
	if (point.x <= screenFrame.left && fLastPoint.x > screenFrame.left)
		location = kLeftEdge;
	else if (point.x >= screenFrame.right && fLastPoint.x < screenFrame.right)
		location = kRightEdge;
	else if (point.y <= screenFrame.top && fLastPoint.y > screenFrame.top)
		location = kTopEdge;
	else if (point.y >= screenFrame.bottom && fLastPoint.y < screenFrame.bottom)
		location = kBottomEdge;

	if (location != kNowhere)
		_Notify(location, fLastTeam);

	_UpdateLast(point);
}


void
CaptureView::KeyDown(const char* bytes, int32 numBytes)
{
	if ((::modifiers() & (B_CONTROL_KEY | B_SHIFT_KEY | B_OPTION_KEY
			| B_COMMAND_KEY)) != (B_COMMAND_KEY | B_CONTROL_KEY))
		return;

	uint32 location = kNowhere;

	switch (bytes[0]) {
		case '1':
			location = kLeftEdge;
			break;
		case '2':
			location = kRightEdge;
			break;
		case '3':
			location = kTopEdge;
			break;
		case '4':
			location = kBottomEdge;
			break;
	}

	if (location != kNowhere)
		_Notify(location, _CurrentTeam());
}


void
CaptureView::_UpdateLast(const BPoint& point)
{
	fLastPoint = point;

	bigtime_t now = system_time();

	// We update the currently active application only, if the mouse did
	// not move over it for a certain time - this is necessary only for
	// focus follow mouse.
	if (now > fLastEvent + kUpdateDelay)
		fLastTeam = _CurrentTeam();

	fLastEvent = now;
}


void
CaptureView::_Notify(uint32 location, team_id team)
{
	if (location == kNowhere)
		return;

	BMessage message(kMsgLocationTrigger);
	message.AddInt32("location", location);
	message.AddInt32("team", team);

	be_app->PostMessage(&message);
}


void
CaptureView::_SendMovedOutNotification()
{
	BMessage movedOut(fMovedOutWhat);
	fMovedOutMessenger.SendMessage(&movedOut);
	fMovedOutWhat = 0;
}


/*static*/ team_id
CaptureView::_CurrentTeam()
{
	app_info appInfo;
	status_t status = be_roster->GetActiveAppInfo(&appInfo);
	if (status == B_OK)
		return appInfo.team;

	return status;
}


// #pragma mark -


CaptureWindow::CaptureWindow()
	:
	BWindow(BRect(0, 0, 100, 100), "mouse capture", B_NO_BORDER_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS, B_ALL_WORKSPACES)
{
	fCaptureView = new CaptureView();
	AddChild(fCaptureView);
}


CaptureWindow::~CaptureWindow()
{
}


void
CaptureWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgHideWhenMouseMovedOut:
			fCaptureView->AddMoveOutNotification(message);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
