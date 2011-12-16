/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Philippe Saint-Pierre <stpere@gmail.com>
 *		Hamish Morrison <hamish@lavabit.com>
 */

#include "DateTimeView.h"

#include <time.h>
#include <syscalls.h>

#include <Box.h>
#include <CalendarView.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <DateTime.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <StringView.h>
#include <Window.h>

#include "AnalogClock.h"
#include "DateTimeEdit.h"
#include "TimeMessages.h"
#include "TimeWindow.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Time"


using BPrivate::BCalendarView;
using BPrivate::BDateTime;
using BPrivate::B_LOCAL_TIME;


DateTimeView::DateTimeView(const char* name)
	:
	BGroupView(name, B_HORIZONTAL, 5),
	fInitialized(false),
	fSystemTimeAtStart(system_time())
{
	_InitView();

	// record the current time to enable revert.
	time(&fTimeAtStart);
}


DateTimeView::~DateTimeView()
{
}


void
DateTimeView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	if (!fInitialized) {
		fInitialized = true;

		fCalendarView->SetTarget(this);
	}
}


void
DateTimeView::MessageReceived(BMessage* message)
{
	int32 change;
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
			switch (change) {
				case H_TM_CHANGED:
					_UpdateDateTime(message);
					break;

				default:
					BView::MessageReceived(message);
					break;
			}
			break;

		case kDayChanged:
		{
			BMessage msg(*message);
			msg.what = H_USER_CHANGE;
			msg.AddBool("time", false);
			Window()->PostMessage(&msg);
			break;
		}

		case kMsgRevert:
			_Revert();
			break;

		case kChangeTimeFinished:
			if (fClock->IsChangingTime())
				fTimeEdit->MakeFocus(false);
			fClock->ChangeTimeFinished();
			break;

		case kRTCUpdate:
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


bool
DateTimeView::CheckCanRevert()
{
	// check for changed time
	time_t unchangedNow = fTimeAtStart + _PrefletUptime();
	time_t changedNow;
	time(&changedNow);

	return changedNow != unchangedNow;
}


void
DateTimeView::_Revert()
{
	// Set the clock and calendar as they were at launch time +
	// time elapsed since application launch.

	time_t timeNow = fTimeAtStart + _PrefletUptime();
	struct tm result;
	struct tm* timeInfo;
	timeInfo = localtime_r(&timeNow, &result);

	BDateTime dateTime = BDateTime::CurrentDateTime(B_LOCAL_TIME);
	BTime time = dateTime.Time();
	BDate date = dateTime.Date();
	time.SetTime(timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec % 60);
	date.SetDate(timeInfo->tm_year + 1900, timeInfo->tm_mon + 1,
		timeInfo->tm_mday);
	dateTime.SetTime(time);
	dateTime.SetDate(date);

	set_real_time_clock(dateTime.Time_t());
}


time_t
DateTimeView::_PrefletUptime() const
{
	return (time_t)((system_time() - fSystemTimeAtStart) / 1000000);
}


void
DateTimeView::_InitView()
{
	fCalendarView = new BCalendarView("calendar");
	fCalendarView->SetWeekNumberHeaderVisible(false);
	fCalendarView->SetSelectionMessage(new BMessage(kDayChanged));
	fCalendarView->SetInvocationMessage(new BMessage(kDayChanged));

	fDateEdit = new TDateEdit("dateEdit", 3);
	fTimeEdit = new TTimeEdit("timeEdit", 4);
	fClock = new TAnalogClock("analogClock");

	BTime time(BTime::CurrentTime(B_LOCAL_TIME));
	fClock->SetTime(time.Hour(), time.Minute(), time.Second());

	BBox* divider = new BBox(BRect(0, 0, 1, 1),
		B_EMPTY_STRING, B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER);
	divider->SetExplicitMaxSize(BSize(1, B_SIZE_UNLIMITED));

	const float kInset = be_control_look->DefaultItemSpacing();
	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, kInset / 2)
			.Add(fDateEdit)
			.Add(fCalendarView)
		.End()
		.Add(divider)
		.AddGroup(B_VERTICAL, 0)
			.Add(fTimeEdit)
			.Add(fClock)
		.End()
		.SetInsets(kInset, kInset, kInset, kInset);
}


void
DateTimeView::_UpdateDateTime(BMessage* message)
{
	int32 day;
	int32 month;
	int32 year;
	if (message->FindInt32("month", &month) == B_OK
		&& message->FindInt32("day", &day) == B_OK
		&& message->FindInt32("year", &year) == B_OK) {
		static int32 lastDay;
		static int32 lastMonth;
		static int32 lastYear;
		if (day != lastDay || month != lastMonth || year != lastYear) {
			fDateEdit->SetDate(year, month, day);
			fCalendarView->SetDate(year, month, day);
			lastDay = day;
			lastMonth = month;
			lastYear = year;
		}
	}

	int32 hour;
	int32 minute;
	int32 second;
	if (message->FindInt32("hour", &hour) == B_OK
		&& message->FindInt32("minute", &minute) == B_OK
		&& message->FindInt32("second", &second) == B_OK) {
		fClock->SetTime(hour, minute, second);
		fTimeEdit->SetTime(hour, minute, second);
	}
}

