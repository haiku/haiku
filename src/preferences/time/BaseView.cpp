/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */


#include "BaseView.h"

#include <DateTime.h>
#include <OS.h>

#include "TimeMessages.h"


TTimeBaseView::TTimeBaseView(const char* name)
	:
	BGroupView(name, B_VERTICAL, 0),
	fMessage(H_TIME_UPDATE)
{
	SetFlags(Flags() | B_PULSE_NEEDED);
}


TTimeBaseView::~TTimeBaseView()
{
}


void
TTimeBaseView::Pulse()
{
	if (IsWatched())
		_SendNotices();
}


void
TTimeBaseView::AttachedToWindow()
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(ViewUIColor());
}


void
TTimeBaseView::ChangeTime(BMessage* message)
{
	bool isTime;
	if (message->FindBool("time", &isTime) != B_OK)
		return;

	BDateTime dateTime = BDateTime::CurrentDateTime(B_LOCAL_TIME);

	if (isTime) {
		BTime time = dateTime.Time();
		int32 hour;
		if (message->FindInt32("hour", &hour) != B_OK)
			hour  = time.Hour();

		int32 minute;
		if (message->FindInt32("minute", &minute) != B_OK)
			minute = time.Minute();

		int32 second;
		if (message->FindInt32("second", &second) != B_OK)
			second = time.Second();

		time.SetTime(hour, minute, second);
		dateTime.SetTime(time);
	} else {
		BDate date = dateTime.Date();
		int32 day;
		if (message->FindInt32("day", &day) != B_OK)
			day = date.Day();

		int32 year;
		if (message->FindInt32("year", &year) != B_OK)
			year = date.Year();

		int32 month;
		if (message->FindInt32("month", &month) != B_OK)
			month = date.Month();

		date.SetDate(year, month, day);
		dateTime.SetDate(date);
	}

	set_real_time_clock(dateTime.Time_t());
}


void
TTimeBaseView::_SendNotices()
{
	fMessage.MakeEmpty();

	BDate date = BDate::CurrentDate(B_LOCAL_TIME);
	fMessage.AddInt32("day", date.Day());
	fMessage.AddInt32("year", date.Year());
	fMessage.AddInt32("month", date.Month());

	BTime time = BTime::CurrentTime(B_LOCAL_TIME);
	fMessage.AddInt32("hour", time.Hour());
	fMessage.AddInt32("minute", time.Minute());
	fMessage.AddInt32("second", time.Second());

	SendNotices(H_TM_CHANGED, &fMessage);
}

