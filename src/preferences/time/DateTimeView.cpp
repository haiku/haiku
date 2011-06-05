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
#include <RadioButton.h>
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
	fGmtTime(NULL),
	fUseGmtTime(false),
	fInitialized(false),
	fSystemTimeAtStart(system_time())
{
	_ReadRTCSettings();
	_InitView();

	// record the current time to enable revert.
	time(&fTimeAtStart);
}


DateTimeView::~DateTimeView()
{
	_WriteRTCSettings();
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
DateTimeView::Draw(BRect /*updateRect*/)
{
	rgb_color viewcolor = ViewColor();
	rgb_color dark = tint_color(viewcolor, B_DARKEN_4_TINT);
	rgb_color light = tint_color(viewcolor, B_LIGHTEN_MAX_TINT);

	// draw a separator line
	BRect bounds(Bounds());
	BPoint start(bounds.Width() / 2.0f, bounds.top + 5.0f);
	BPoint end(bounds.Width() / 2.0, bounds.bottom - 5.0f);

	BeginLineArray(2);
		AddLine(start, end, dark);
		start.x++;
		end.x++;
		AddLine(start, end, light);
	EndLineArray();

	fTimeEdit->Draw(bounds);
	fDateEdit->Draw(bounds);
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

		case kRTCUpdate:
			fUseGmtTime = fGmtTime->Value() == B_CONTROL_ON;
			_UpdateGmtSettings();
			break;

		case kMsgRevert:
			_Revert();
			break;

		case kChangeTimeFinished:
			if (fClock->IsChangingTime())
				fTimeEdit->MakeFocus(false);
			fClock->ChangeTimeFinished();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


bool
DateTimeView::CheckCanRevert()
{
	// check GMT vs Local setting
	bool enable = fUseGmtTime != fOldUseGmtTime;

	// check for changed time
	time_t unchangedNow = fTimeAtStart + _PrefletUptime();
	time_t changedNow;
	time(&changedNow);

	return enable || (changedNow != unchangedNow);
}


void
DateTimeView::_Revert()
{
	// Set the clock and calendar as they were at launch time +
	// time elapsed since application launch.

	fUseGmtTime = fOldUseGmtTime;
	_UpdateGmtSettings();

	if (fUseGmtTime)
		fGmtTime->SetValue(B_CONTROL_ON);
	else
		fLocalTime->SetValue(B_CONTROL_ON);

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

	BStringView* text = new BStringView("clockSetTo",
		B_TRANSLATE("Clock set to:"));
	fLocalTime = new BRadioButton("localTime",
		B_TRANSLATE("Local time"), new BMessage(kRTCUpdate));
	fGmtTime = new BRadioButton("greenwichMeanTime",
		B_TRANSLATE("GMT"), new BMessage(kRTCUpdate));

	if (fUseGmtTime)
		fGmtTime->SetValue(B_CONTROL_ON);
	else
		fLocalTime->SetValue(B_CONTROL_ON);
	fOldUseGmtTime = fUseGmtTime;

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
			.Add(text)
			.AddGroup(B_HORIZONTAL, kInset)
				.Add(fLocalTime)
				.Add(fGmtTime)
			.End()
		.End()
		.SetInsets(kInset, kInset, kInset, kInset);
}


void
DateTimeView::_ReadRTCSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("RTC_time_settings");

	BEntry entry(path.Path());
	if (entry.Exists()) {
		BFile file(&entry, B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			char buffer[6];
			file.Read(buffer, 6);
			if (strncmp(buffer, "gmt", 3) == 0)
				fUseGmtTime = true;
		}
	}
}


void
DateTimeView::_WriteRTCSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return;

	path.Append("RTC_time_settings");

	BFile file(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() == B_OK) {
		if (fUseGmtTime)
			file.Write("gmt", 3);
		else
			file.Write("local", 5);
	}
}


void
DateTimeView::_UpdateGmtSettings()
{
	_WriteRTCSettings();

	_kern_set_real_time_clock_is_gmt(fUseGmtTime);
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
		fDateEdit->SetDate(year, month, day);
		fCalendarView->SetDate(year, month, day);
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
