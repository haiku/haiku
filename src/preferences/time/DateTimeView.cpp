/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */

#include "DateTimeView.h"
#include "AnalogClock.h"
#include "CalendarView.h"
#include "DateTimeEdit.h"
#include "TimeMessages.h"


#include <CheckBox.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <RadioButton.h>
#include <String.h>
#include <StringView.h>
#include <Window.h>


#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#include <syscalls.h>
#else
void _kset_tzfilename_(const char *name, size_t length, bool isGMT);
#define _kern_set_tzfilename _kset_tzfilename_
#endif


DateTimeView::DateTimeView(BRect frame)
	: BView(frame, "dateTimeView", B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE_JUMP),
	  fGmtTime(NULL),
	  fUseGmtTime(false),
	  fInitialized(false)
{
	_ReadRTCSettings();
	_InitView();
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

		fGmtTime->SetTarget(this);
		fLocalTime->SetTarget(this);
		fCalendarView->SetTarget(this);		
	}
}


void
DateTimeView::Draw(BRect /*updateRect*/)
{
	rgb_color viewcolor = ViewColor();
	rgb_color dark = tint_color(viewcolor, B_DARKEN_4_TINT);
	rgb_color light = tint_color(viewcolor, B_LIGHTEN_MAX_TINT);

	//draw a separator line
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
DateTimeView::MessageReceived(BMessage *message)
{
	int32 change;
	switch(message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
			switch(change) {
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
		}	break;

		case kRTCUpdate:
			fUseGmtTime = !fUseGmtTime;
			_UpdateGmtSettings();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
DateTimeView::_InitView()
{
	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float textHeight = fontHeight.descent + fontHeight.ascent 
		+ fontHeight.leading + 6.0;	// 6px border

	// left side
	BRect bounds = Bounds();
	bounds.InsetBy(10.0, 10.0);
	bounds.top += textHeight + 10.0;

	fCalendarView = new BCalendarView(bounds, "calendar");
	fCalendarView->SetWeekNumberHeaderVisible(false);
	fCalendarView->ResizeToPreferred();
	fCalendarView->SetSelectionMessage(new BMessage(kDayChanged));
	fCalendarView->SetInvocationMessage(new BMessage(kDayChanged));

	bounds.top -= textHeight + 10.0;
	bounds.bottom = bounds.top + textHeight;
	bounds.right = fCalendarView->Frame().right;

	fDateEdit = new TDateEdit(bounds, "dateEdit", 3);
	AddChild(fDateEdit);
	AddChild(fCalendarView);

	// right side, 2px extra for separator
	bounds.OffsetBy(bounds.Width() + 22.0, 0.0);
	fTimeEdit = new TTimeEdit(bounds, "timeEdit", 4);
	AddChild(fTimeEdit);

	bounds = fCalendarView->Frame();
	bounds.OffsetBy(bounds.Width() + 22.0, 0.0);

	fClock = new TAnalogClock(bounds, "analogClock");
	AddChild(fClock);

	// clock radio buttons
	bounds.top = fClock->Frame().bottom + 10.0;
	BStringView *text = new BStringView(bounds, "clockSetTo", "Clock set to:");
	AddChild(text);
	text->ResizeToPreferred();

	bounds.left += 10.0f;
	bounds.top = text->Frame().bottom;
	fLocalTime = new BRadioButton(bounds, "localTime", "Local Time", 
		new BMessage(kRTCUpdate));
	AddChild(fLocalTime);
	fLocalTime->ResizeToPreferred();

	bounds.left = fLocalTime->Frame().right + 10.0;
	fGmtTime = new BRadioButton(bounds, "greenwichMeanTime", "GMT",
		new BMessage(kRTCUpdate));
	AddChild(fGmtTime);
	fGmtTime->ResizeToPreferred();
	
	if (fUseGmtTime)
		fGmtTime->SetValue(B_CONTROL_ON);
	else
		fLocalTime->SetValue(B_CONTROL_ON);

	ResizeTo(fClock->Frame().right + 10.0, fGmtTime->Frame().bottom + 10.0);
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
	} else
		_UpdateGmtSettings();
}


void
DateTimeView::_WriteRTCSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
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

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("timezone");
	BEntry entry(path.Path(), true);

	if (!entry.Exists())
		return;

	entry.GetPath(&path);

	// take the existing timezone and set it's gmt use
	_kern_set_tzfilename(path.Path(), B_PATH_NAME_LENGTH, fUseGmtTime);
}


void
DateTimeView::_UpdateDateTime(BMessage *message)
{
	int32 day;
	int32 month;
	int32 year;
	if (message->FindInt32("month", &month) == B_OK
		&& message->FindInt32("day", &day) == B_OK
		&& message->FindInt32("year", &year) == B_OK)
	{
		fDateEdit->SetDate(year, month, day);
		fCalendarView->SetDate(year, month, day);
	}

	int32 hour;
	int32 minute;
	int32 second;
	if (message->FindInt32("hour", &hour) == B_OK
		&& message->FindInt32("minute", &minute) == B_OK
		&& message->FindInt32("second", &second) == B_OK)
	{
		fClock->SetTime(hour, minute, second);
		fTimeEdit->SetTime(hour, minute, second);
	}
}
