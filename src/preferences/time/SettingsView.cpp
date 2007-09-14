/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		probably Mike Berg <mike@agamemnon.homelinux.net>
 *		and/or Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Julun <host.haiku@gmx.de>
 */

#include "SettingsView.h"
#include "AnalogClock.h"
#include "CalendarView.h"
#include "DateTimeEdit.h"
#include "TimeMessages.h"


#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <RadioButton.h>
#include <String.h>
#include <StringView.h>


TSettingsView::TSettingsView(BRect frame)
	: BView(frame,"Settings", B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP),
	  fGmtTime(NULL)
{	
	InitView();
}
	

TSettingsView::~TSettingsView()
{
}


void
TSettingsView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
TSettingsView::MessageReceived(BMessage *message)
{
	int32 change;
	switch(message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
			switch(change)
			{
				case H_TM_CHANGED:
					UpdateDateTime(message);
				break;
				
				default:
					BView::MessageReceived(message);
				break;
			}
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
	*width = 361.0f;
	*height = 227.0f;

	if (fGmtTime) {
		// we are initialized
		*width = Bounds().Width();
		*height = fGmtTime->Frame().bottom;
	}
}


void
TSettingsView::InitView()
{
	ReadRTCSettings();
	
	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float textHeight = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	// left side
	BRect frameLeft(Bounds());
	frameLeft.right = frameLeft.Width() / 2;
	frameLeft.InsetBy(10.0f, 10.0f);
	frameLeft.bottom = frameLeft.top + textHeight +6;

	fDateEdit = new TDateEdit(frameLeft, "date_edit", 3);
	AddChild(fDateEdit);

	frameLeft.top = fDateEdit->Frame().bottom + 10;
	frameLeft.bottom = Bounds().bottom - 10;

	fCalendar = new TCalendarView(frameLeft, "calendar", B_FOLLOW_NONE, B_WILL_DRAW);
	AddChild(fCalendar);

	// right side
	BRect frameRight(Bounds());
	frameRight.left = frameRight.Width() / 2;
	frameRight.InsetBy(10.0f, 10.0f);
	frameRight.bottom = frameRight.top + textHeight +6;

	fTimeEdit = new TTimeEdit(frameRight, "time_edit", 4);
	AddChild(fTimeEdit);

	frameRight.top = fTimeEdit->Frame().bottom + 10;
	frameRight.bottom = Bounds().bottom - 10;

	float left = fTimeEdit->Frame().left;
	float tmp = MIN(frameRight.Width(), frameRight.Height());
	frameRight.left = left + (fTimeEdit->Bounds().Width() - tmp) /2;	
	frameRight.bottom = frameRight.top + tmp;
	frameRight.right = frameRight.left + tmp;
	
	fClock = new TAnalogClock(frameRight, "analog clock", B_FOLLOW_NONE, B_WILL_DRAW);
	AddChild(fClock);
	
	// clock radio buttons
	frameRight.left = left;
	frameRight.top = fClock->Frame().bottom + 10;
	BStringView *text = new BStringView(frameRight, "clockis", "Clock set to:");
	AddChild(text);
	text->ResizeToPreferred();

	frameRight.left += 10.0f;	
	frameRight.top = text->Frame().bottom + 5;

	fLocalTime = new BRadioButton(frameRight, "local", "Local time",
		new BMessage(H_RTC_CHANGE));
	AddChild(fLocalTime);
	fLocalTime->ResizeToPreferred();

	frameRight.left = fLocalTime->Frame().right +10.0f;

	fGmtTime = new BRadioButton(frameRight, "gmt", "GMT", new BMessage(H_RTC_CHANGE));
	AddChild(fGmtTime);
	fGmtTime->ResizeToPreferred();

	if (fIsLocalTime)
		fLocalTime->SetValue(B_CONTROL_ON);
	else
		fGmtTime->SetValue(B_CONTROL_ON);
}


void
TSettingsView::Draw(BRect /*updateRect*/)
{
	//draw a separator line
	BRect bounds(Bounds());
	
	rgb_color viewcolor = ViewColor();
	rgb_color dark = tint_color(viewcolor, B_DARKEN_4_TINT);
	rgb_color light = tint_color(viewcolor, B_LIGHTEN_MAX_TINT);

	BPoint start(bounds.Width() / 2.0f +2.0f, bounds.top +1.0f);
	BPoint end(bounds.Width() / 2.0 +2.0f, bounds.bottom -1.0f);
	
	BeginLineArray(2);
		AddLine(start, end, dark);
		start.x++;
		end.x++;
		AddLine(start, end, light);
	EndLineArray();
	
	fTimeEdit->Draw(bounds);
	fDateEdit->Draw(bounds);
}


bool
TSettingsView::GMTime()
{
	return fGmtTime->Value() == B_CONTROL_ON;
}


void
TSettingsView::UpdateDateTime(BMessage *message)
{
	int32 day;	
	int32 month;
	int32 year;
	if (message->FindInt32("month", &month) == B_OK
		&& message->FindInt32("day", &day) == B_OK
		&& message->FindInt32("year", &year) == B_OK)
	{
		fDateEdit->SetDate(year, month, day);
		fCalendar->SetTo(year, month, day);
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


void
TSettingsView::ReadRTCSettings()
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
			if (text.Compare("local\n", 5) == 0)
				fIsLocalTime = true;
			else
				fIsLocalTime = false;
		}
	} else {
		// create set to local
		fIsLocalTime = true;
		file.SetTo(&entry, B_CREATE_FILE | B_READ_WRITE);
		file.Write("local\n", 6);
	}
}

