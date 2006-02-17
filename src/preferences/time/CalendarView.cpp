/*
 * Copyright 2004-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Berg <mike@agamemnon.homelinux.net>
 */


#include <String.h>
#include <Window.h>

#include "CalendarView.h"
#include "DateUtils.h"
#include "TimeMessages.h"

#include <stdio.h>


#define SEGMENT_CHANGE 'ostd'


// ToDo: read these out of the locale
static const char kDays[7] = { 'S', 'M', 'T', 'W', 'T', 'F', 'S' };


static float
FontHeight(BView* target, bool full)
{
	font_height finfo;		
	target->GetFontHeight(&finfo);
	float h = ceil(finfo.ascent) + ceil(finfo.descent);

	if (full)
		h += ceil(finfo.leading);
	
	return h;
}


//	#pragma mark -


TDay::TDay(BRect frame, int day) 
	: BControl(frame, B_EMPTY_STRING, NULL, NULL, B_FOLLOW_NONE, B_WILL_DRAW)
	, f_day(day)
{
}


TDay::~TDay()
{
}


void
TDay::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
TDay::MouseDown(BPoint where)
{
	if (f_day> 0) {
		// only allow if not currently selected
		if (Value() == B_CONTROL_ON)
			return;
		
		SetValue(B_CONTROL_ON);
		Invoke(); 

		//update display
		Draw(Bounds());	
	}
}


void
TDay::KeyDown(const char *bytes, int32 numbytes)
{
	BControl::KeyDown(bytes, numbytes);
	Parent()->KeyDown(bytes, numbytes);
}


void
TDay::MakeFocus(bool focused)
{
	if (focused != IsFocus()) {
		BView::MakeFocus(focused);
		Draw(Bounds());
		Flush();
	}
}


void
TDay::Stroke3dFrame(BRect frame, rgb_color light, rgb_color dark, bool inset)
{
	rgb_color color1;
	rgb_color color2;
	
	if (inset) {
		color1 = dark;
		color2 = light;
	} else {
		color1 = light;
		color2 = dark;
	}
	
	BeginLineArray(4);
		AddLine(frame.LeftTop(), frame.RightTop(), color1);
		AddLine(frame.LeftTop(), frame.LeftBottom(), color1);
		AddLine(frame.LeftBottom(), frame.RightBottom(), color2);
		AddLine(frame.RightBottom(), frame.RightTop(), color2);
	EndLineArray();
}


void
TDay::Draw(BRect updaterect)
{
	BRect bounds(Bounds());
	
	SetHighColor(ViewColor());
	FillRect(Bounds(), B_SOLID_HIGH);
	
	rgb_color bgcolor;
	rgb_color dark;
	rgb_color light;
	
	BString text;
	
	if (Value() == 1) {
		bgcolor = tint_color(ViewColor(), B_DARKEN_2_TINT);		
	} else {
		bgcolor = tint_color(ViewColor(), B_DARKEN_1_TINT);
	}
	text << f_day;

	dark = tint_color(bgcolor, B_DARKEN_1_TINT);
	light = tint_color(bgcolor, B_LIGHTEN_MAX_TINT);
	
	SetHighColor(bgcolor);
	FillRect(bounds);

	if (f_day> 0) {
		if (!(Value() == 1))
			SetHighColor(0, 0, 0, 255);
		else
			SetHighColor(255, 255, 255, 255);
			
		SetLowColor(bgcolor);
		
		// calc font size;
		float width = be_plain_font->StringWidth(text.String());
		font_height finfo;
		be_plain_font->GetHeight(&finfo);
		float height = finfo.descent +finfo.ascent +finfo.leading;

		BPoint drawpt((bounds.Width()/2.0) -(width/2.0) +1, (bounds.Height()/2.0) +(height/2.0) -2);
		DrawString(text.String(), drawpt);
	}
	
	if (IsFocus()) {
		rgb_color nav_color = keyboard_navigation_color();
		SetHighColor(nav_color);
		StrokeRect(bounds);
		
	} else {
		SetHighColor(bgcolor);
		StrokeRect(bounds);
		if (Value() == 1)
			Stroke3dFrame(bounds, light, dark, true);
	}
}


void
TDay::SetValue(int32 value)
{
	BControl::SetValue(value);
	
	if (Value() == 1) {
		SetFlags(Flags() & ~B_NAVIGABLE);
	} else {
		SetFlags(Flags()|B_NAVIGABLE); 
	}
	Draw(Bounds());
}


void 
TDay::SetTo(BRect frame, int day)
{
	MoveTo(frame.LeftTop());
	f_day = day;
	Draw(Bounds());
}


void
TDay::SetTo(int day, bool selected)
{
	f_day = day;
	SetValue(selected);
	if (Value() == 1) {
		SetFlags(Flags()|B_NAVIGABLE);
	} else {
		SetFlags(Flags() & ~B_NAVIGABLE);
	}
	Draw(Bounds());
}


//	#pragma mark -


TCalendarView::TCalendarView(BRect frame, const char *name, 
		uint32 resizingmode, uint32 flags)
	: BView(frame, name, resizingmode, flags)
	, f_firstday(0)
	, f_month(0)
	, f_day(0)
	,f_year(0)
{
	InitView();
}


TCalendarView::~TCalendarView()
{
}


void
TCalendarView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
TCalendarView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case H_DAY_CHANGED:
		{
			TDay *day;
			message->FindPointer("source", (void **)&day);
			if (f_cday != NULL)
				f_cday->SetValue(B_CONTROL_OFF);
			f_cday = day;
			
			DispatchMessage();
		}
		break;
		
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TCalendarView::KeyDown(const char *bytes, int32 numbytes)
{
	if (f_focused == NULL)
		f_focused = f_cday;
		
	switch(bytes[0]) {
		case B_LEFT_ARROW: {
			TDay *newfocus = dynamic_cast<TDay *>(f_focused->PreviousSibling());
			if (newfocus == NULL || newfocus->Day() == 0)
				break;
			f_focused->MakeFocus(false);
			f_focused = newfocus;
			f_focused->MakeFocus(true);
		}
		break;
		
		case B_RIGHT_ARROW: {
			TDay *newfocus = dynamic_cast<TDay *>(f_focused->NextSibling());
			if (newfocus == NULL || newfocus->Day() == 0)
				break;
			f_focused->MakeFocus(false);
			f_focused = newfocus;
			f_focused->MakeFocus(true);
		}
		break;
		
		case B_UP_ARROW: {
			int32 idx = IndexOf(f_focused) -7;
			if (idx> f_daybound.x -1 ) {
				f_focused->MakeFocus(false);
				f_focused = dynamic_cast<TDay  *>(ChildAt(idx));
				f_focused->MakeFocus(true);
			}
		}
		break;
		
		case B_DOWN_ARROW: {
			int32 idx = IndexOf(f_focused) +7;
			if (idx < f_daybound.y +1) {
				f_focused->MakeFocus(false);
				f_focused = dynamic_cast<TDay *>(ChildAt(idx));
				f_focused->MakeFocus(true);
			}
		}
		break;
		
		default:
		break;
	}
}


int32
TCalendarView::IndexOf(BView *child) const
{
	BView *test;
	for (int32 index = 0; index < CountChildren(); index++) {
		test = ChildAt(index);
		if (test == child)
			return index;
	}

	return -1;
}


void
TCalendarView::Draw(BRect updaterect)
{	
	BRect bounds(0.0, 0.0, f_dayrect.Width(), f_dayrect.Height());

	float width = 0;	
	float x = bounds.Width()/2.0;
	BPoint drawpt;
	BString day;
	SetLowColor(ViewColor());
	SetHighColor(0, 0, 0);
	
	float offset = FontHeight(this, true);
	
	for (int i = 0; i < 7; i++) {
		day = BString(&kDays[i], 1);
		width = be_plain_font->StringWidth(day.String());
		drawpt.x = bounds.left + (x - width / 2.0 + 2);
		drawpt.y = bounds.bottom - offset / 2.0;
		DrawString(day.String(), drawpt);	
		bounds.OffsetBy(bounds.Width() + 1, 0);
	}
}


void
TCalendarView::InitView()
{
	font_height finfo;
	be_plain_font->GetHeight(&finfo);
	float text_height = finfo.ascent +finfo.descent +finfo.leading;
	
	BRect bounds(Bounds());
	float width = (bounds.Width() -7)/7;
	float height = ((bounds.Height() -(text_height +4)) -6)/6;
	f_dayrect.Set(0.0, text_height +6, width, text_height +6 +height);

	BRect dayrect(f_dayrect);
	TDay *day;
	for (int row = 0; row < 6; row++) {
		for (int col = 0; col < 7; col++) {
			day = new TDay(dayrect.InsetByCopy(1, 1), 0);
			AddChild(day);
			dayrect.OffsetBy(width +1, 0);
		}
		dayrect.OffsetBy(0, height +1);
		dayrect.OffsetTo(0, dayrect.top);
	}
	
	f_cday = NULL;
}

void
TCalendarView::InitDates()
{
	TDay *day;
	
	int32 label = 0;
	int idx = 0;
	f_firstday = getFirstDay(f_month, f_year);
	int daycnt = getDaysInMonth(f_month, f_year);
	bool cday = false;
	for (int row = 0; row < 6; row++) {
		for (int col = 0; col < 7; col++) {
			day = dynamic_cast<TDay *>(ChildAt((row *7) +col));
			
			if (idx < f_firstday || idx > daycnt +f_firstday +1)
				label = 0;
			else
				label = idx - (f_firstday - 1);
			
			idx++;
			cday = label == f_day;
			day->SetTo(label, cday);

			if (label> 0) {
				BMessage *message = new BMessage(H_DAY_CHANGED);
				message->AddInt32("Day", label);
				
				day->SetTarget(this);
				day->SetMessage(message);
				if (label == 1)
					f_daybound.x = row *7 +col;
				if (label == daycnt)
					f_daybound.y = row *7 +col;
			}
			
			if (cday) {
				f_cday = day;
			}
			cday = false;		
		}
	}
	if (f_focused == NULL)
		f_focused = f_cday;
}


void
TCalendarView::SetTo(int32 year, int32 month, int32 day)
{
	if (f_month != month||f_year != year) {
		f_month = month;
		f_day = day;
		f_year = year;
		InitDates();
	} else if (f_day != day) {
		f_day = day;
		int idx = ((f_day/7)*7) + (f_day%7 +f_firstday - 1);
		TDay *day = dynamic_cast<TDay *>(ChildAt(idx));
		f_cday->SetValue(B_CONTROL_OFF);
		f_cday = day;
		day->SetValue(B_CONTROL_ON);		
	}
}


void
TCalendarView::DispatchMessage()
{
	// send message to update timedate
	BMessage *msg = new BMessage(H_USER_CHANGE);
	msg->AddBool("time", false);
	msg->AddInt32("month", f_month);
	if (f_cday != NULL)
		msg->AddInt32("day", f_cday->Day());
	msg->AddInt32("year", f_year);
		
	Window()->PostMessage(msg);
}
