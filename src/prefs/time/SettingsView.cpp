/*
	SettingsView.cpp
*/
#include <RadioButton.h>
#include <StringView.h>
#include <View.h>
#include <Box.h>
#include <String.h>
#include <RadioButton.h>
#include <AppDefs.h>
#include <Handler.h>
#include <stdio.h>
#include <Path.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>

#include "SettingsView.h"
#include "AnalogClock.h"
#include "CalendarView.h"
#include "TimeMessages.h"

TSettingsView::TSettingsView(BRect frame):
	BView(frame,"Settings", B_FOLLOW_ALL, 
			B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE_JUMP|B_PULSE_NEEDED)
{
	InitView();
} //SettingsView::SettingsView()
	
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
	switch(message->what)
	{
		case B_OBSERVER_NOTICE_CHANGE:
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
			switch (change)
			{
				case OB_TIME_UPDATE:
				{
					UpdateDateTime(message);
					break;
				}
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
TSettingsView::InitView()
{
	ReadFiles();
	
	font_height finfo;
	be_plain_font->GetHeight(&finfo);
	float text_height = finfo.descent +finfo.ascent +finfo.leading;

	BRect bounds(Bounds());
	bounds.right = (bounds.Width()/2.0) -5;

	BRect frame(bounds);
	
	// left side
	frame.InsetBy(6, 6);
	frame.bottom = frame.top +text_height +6;
	f_dateedit = new TDateEdit(frame, "date_edit");
	
	frame.top = f_dateedit->Frame().bottom;
	frame.bottom = bounds.bottom;
	frame.InsetBy(10, 8);
	
	f_calendar = new TCalendarView(frame, "calendar", B_FOLLOW_NONE, B_WILL_DRAW);
	
	AddChild(f_dateedit);
	AddChild(f_calendar);
	
	// right side
	bounds.OffsetBy(bounds.Width() +9, 0);
	f_temp = bounds;
	frame = bounds;
	frame.InsetBy(26, 6);
	frame.bottom = frame.top +text_height +6;
	frame.right += 4;
	f_timeedit = new TTimeEdit(frame, "time_edit");
	
	frame.top = f_timeedit->Frame().bottom;
	frame.bottom = bounds.bottom -(text_height *3);
	frame.InsetBy(10, 10);
	f_clock = new TAnalogClock(frame, "analog clock", 
			B_FOLLOW_NONE, B_WILL_DRAW);
	
	// clock radio buttons
	frame = bounds.InsetByCopy(6, 10);
	frame.top = f_clock->Frame().bottom +12;
	frame.bottom = frame.top +text_height +2;
	BString label = "Clock set to:";
	float width = be_plain_font->StringWidth(label.String());
	frame.right = frame.left +width;
	BStringView *text = new BStringView(frame, "clockis", "Clock set to:");
	
	frame.OffsetBy(frame.Width() +9, -1);
	frame.right = bounds.right-2;
	
	f_local = new BRadioButton(frame, "local", "Local time", NULL);
	AddChild(f_local);
	
	frame.OffsetBy(0, text_height +4);
	f_gmt = new BRadioButton(frame, "gmt", "GMT", NULL);
	AddChild(f_gmt);
	
	AddChild(text);	
	AddChild(f_timeedit);
	AddChild(f_clock);
	
	f_clock->SetViewColor(200, 20, 200);
	if (f_islocal)
		f_local->SetValue(1);
	else
		f_gmt->SetValue(1);
		
}//SettingsView::buildView()

void
TSettingsView::Draw(BRect frame)
{
	//draw a separator line
	BRect bounds(Bounds());
	
	rgb_color viewcolor = ViewColor();
	rgb_color dark = tint_color(viewcolor, B_DARKEN_4_TINT);
	rgb_color light = tint_color(viewcolor, B_LIGHTEN_MAX_TINT);

	BPoint start(bounds.Width()/2.0 +2.0, bounds.top +1);
	BPoint end(bounds.Width()/2.0 +2.0, bounds.bottom -1);
	
	BeginLineArray(2);
		AddLine(start, end, dark);
		start.x++;
		end.x++;
		AddLine(start, end, light);
	EndLineArray();
}//SettingsView::Draw(BRect frame)

void
TSettingsView::UpdateDateTime(BMessage *message)
{
	struct tm *ltime;
	message->FindPointer("_tm_", (void **)&ltime);
	f_timeedit->Update(ltime);
	f_clock->Update(ltime);

	f_dateedit->Update(ltime);
	f_calendar->Update(ltime);
}

void
TSettingsView::ReadFiles()
{
	// do all file reading here

	// read RTC_time_settings
	
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
	{
		return; // NO USER SETTINGS DIRECTORY!!!
	}
	path.Append("RTC_time_settings");
	
	BEntry entry(path.Path());
	BFile file;
	
	if (entry.Exists())
	{
		//read file
		file.SetTo(&entry, B_READ_ONLY);
		status_t err = file.InitCheck();
		if (err == B_OK)
		{
			char buff[6];
			file.Read(buff, 6);
			BString	text;
			text << buff;
			if (text.Compare("local\n", 5) == 0)
				f_islocal = true;
			else
				f_islocal = false;
			printf("f_islocal >> (BS)%s, (BF)%s\n", text.String(), buff);
		}
	}
	else
	{
		// create set to local
		f_islocal = true;
		file.SetTo(&entry, B_CREATE_FILE|B_READ_WRITE);
		file.Write("local\n", 6);
	}
	
	// done reading RTC_time_settings
}
