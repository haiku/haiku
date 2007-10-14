/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */

#include "SettingsView.h"
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


#include <stdlib.h>


#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#include <syscalls.h>
#else
void _kset_tzfilename_(const char *name, size_t length, bool isGMT);
#define _kern_set_tzfilename _kset_tzfilename_
#endif


TSettingsView::TSettingsView(BRect frame)
	: BView(frame,"Settings", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP),
	  fGmtTime(NULL),
	  fInitialized(false)
{
	_ReadRTCSettings();
}


TSettingsView::~TSettingsView()
{
	_WriteRTCSettings();
}


void
TSettingsView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	if (!fInitialized) {
		_InitView();
		fInitialized = true;
	}
}


void
TSettingsView::Draw(BRect /*updateRect*/)
{
	rgb_color viewcolor = ViewColor();
	rgb_color dark = tint_color(viewcolor, B_DARKEN_4_TINT);
	rgb_color light = tint_color(viewcolor, B_LIGHTEN_MAX_TINT);

	//draw a separator line
	BRect bounds(Bounds());
	BPoint start(bounds.Width() / 2.0f + 2.0f, bounds.top + 2.0f);
	BPoint end(bounds.Width() / 2.0 + 2.0f, bounds.bottom - 2.0f);

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
TSettingsView::MessageReceived(BMessage *message)
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
			_UpdateGmtSettings();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TSettingsView::GetPreferredSize(float *width, float *height)
{
	// hardcode in TimeWindow ...
	*width = 470.0f;
	*height = 227.0f;

	if (fInitialized) {
		// we are initialized
		*width = Bounds().Width();
		*height = fGmtTime->Frame().bottom;
	}
}


void
TSettingsView::_InitView()
{
	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float textHeight = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	// left side
	BRect frameLeft(Bounds());
	frameLeft.right = frameLeft.Width() / 2.0;
	frameLeft.InsetBy(10.0f, 10.0f);
	frameLeft.bottom = frameLeft.top + textHeight + 6.0;

	fDateEdit = new TDateEdit(frameLeft, "date_edit", 3);
	AddChild(fDateEdit);

	frameLeft.top = fDateEdit->Frame().bottom + 10;
	frameLeft.bottom = Bounds().bottom - 10;

	fCalendarView = new BCalendarView(frameLeft, "calendar");
	fCalendarView->SetWeekNumberHeaderVisible(false);
	AddChild(fCalendarView);
	fCalendarView->SetSelectionMessage(new BMessage(kDayChanged));
	fCalendarView->SetInvocationMessage(new BMessage(kDayChanged));
	fCalendarView->SetTarget(this);

	// right side	
	BRect frameRight(Bounds());
	frameRight.left = frameRight.Width() / 2.0;
	frameRight.InsetBy(10.0f, 10.0f);
	frameRight.bottom = frameRight.top + textHeight + 6.0;

	fTimeEdit = new TTimeEdit(frameRight, "time_edit", 4);
	AddChild(fTimeEdit);

	frameRight.top = fTimeEdit->Frame().bottom + 10.0;
	frameRight.bottom = Bounds().bottom - 10.0;

	float left = fTimeEdit->Frame().left;
	float tmp = MIN(frameRight.Width(), frameRight.Height());
	frameRight.left = left + (fTimeEdit->Bounds().Width() - tmp) / 2.0;
	frameRight.bottom = frameRight.top + tmp;
	frameRight.right = frameRight.left + tmp;

	fClock = new TAnalogClock(frameRight, "analog clock", B_FOLLOW_NONE, B_WILL_DRAW);
	AddChild(fClock);

	// clock radio buttons
	frameRight.left = left;
	frameRight.top = fClock->Frame().bottom + 10.0;
	BStringView *text = new BStringView(frameRight, "clockis", "Clock set to:");
	AddChild(text);
	text->ResizeToPreferred();

	frameRight.left += 10.0f;
	frameRight.top = text->Frame().bottom + 5.0;

	fLocalTime = new BRadioButton(frameRight, "local", "Local time",
		new BMessage(kRTCUpdate));
	AddChild(fLocalTime);
	fLocalTime->ResizeToPreferred();
	fLocalTime->SetTarget(this);

	frameRight.left = fLocalTime->Frame().right +10.0f;

	fGmtTime = new BRadioButton(frameRight, "gmt", "GMT", new BMessage(kRTCUpdate));
	AddChild(fGmtTime);
	fGmtTime->ResizeToPreferred();
	fGmtTime->SetTarget(this);
	
	if (fIsLocalTime)
		fLocalTime->SetValue(B_CONTROL_ON);
	else
		fGmtTime->SetValue(B_CONTROL_ON);
}


void
TSettingsView::_ReadRTCSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("RTC_time_settings");

	BFile file;
	BEntry entry(path.Path());
	if (entry.Exists()) {
		file.SetTo(&entry, B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			char localTime[6];
			file.Read(localTime, 6);
			BString	text(localTime);
			if (text.Compare("local", 4) == 0)
				fIsLocalTime = true;
			else
				fIsLocalTime = false;
		}
	} else {
		// create set to local
		fIsLocalTime = true;
		file.SetTo(&entry, B_CREATE_FILE | B_READ_WRITE);
		file.Write("local", 5);
	}
}


void
TSettingsView::_WriteRTCSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("RTC_time_settings");

	BFile file(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() == B_OK) {
		if (fLocalTime->Value() == B_CONTROL_ON)
			file.Write("local", 5);
		else
			file.Write("gmt", 3);
	}
}


void
TSettingsView::_UpdateGmtSettings()
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
	_kern_set_tzfilename(path.Path(), B_PATH_NAME_LENGTH
		, fGmtTime->Value() == B_CONTROL_ON);
}


void
TSettingsView::_UpdateDateTime(BMessage *message)
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
		fTimeEdit->SetTime(hour, minute, second);
		fClock->SetTime(hour, minute, second);
	}
}
