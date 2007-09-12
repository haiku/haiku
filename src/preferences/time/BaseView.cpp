/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg (inseculous)
 *		Julun <host.haiku@gmx.de>
 */

#include "BaseView.h"
#include "TimeMessages.h"


#include <Message.h>
#include <OS.h>


TTimeBaseView::TTimeBaseView(BRect frame, const char *name)
	: BView(frame, name, B_FOLLOW_ALL_SIDES, B_PULSE_NEEDED),
	  fIsGMT(false),
	  fMessage(H_TIME_UPDATE)
{
}


TTimeBaseView::~TTimeBaseView()
{
}


void
TTimeBaseView::Pulse()
{
	if (IsWatched())
		DispatchMessage();
}


void
TTimeBaseView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
TTimeBaseView::SetGMTime(bool gmt)
{
	fIsGMT = gmt;
}


void
TTimeBaseView::ChangeTime(BMessage *message)
{
	bool isTime;
	if (message->FindBool("time", &isTime) != B_OK)
		return;

	time_t tmp = time(NULL);
	struct tm *tm_struct = localtime(&tmp);
	
	if (isTime) {
		int32 hour = 0;
		if (message->FindInt32("hour", &hour) == B_OK)
			tm_struct->tm_hour = hour;

		int32 minute = 0;
		if (message->FindInt32("minute", &minute) == B_OK)
			tm_struct->tm_min = minute;

		int32 second = 0;
		if (message->FindInt32("second", &second) == B_OK)
			tm_struct->tm_sec = second;

		bool isAM = false;
		if (message->FindBool("isam", &isAM) == B_OK) {
			if (!isAM) 
				tm_struct->tm_hour += 12;
		}		
	} else {
		int32 month = 0;
		if (message->FindInt32("month", &month) == B_OK)
			tm_struct->tm_mon = month;

		int32 day = 0;
		if (message->FindInt32("day", &day) == B_OK)
			tm_struct->tm_mday = day;

		int32 year = 0;
		if (message->FindInt32("year", &year) == B_OK)
			tm_struct->tm_year = year;
	}
	
	tmp = mktime(tm_struct);
	set_real_time_clock(tmp);
}


void
TTimeBaseView::DispatchMessage()
{
	time_t tmp = time(NULL);
	struct tm *tm_struct = localtime(&tmp);

	if (fIsGMT)
		tm_struct = gmtime(&tmp);
	
	fMessage.MakeEmpty();

	fMessage.AddInt32("month", tm_struct->tm_mon);
	fMessage.AddInt32("day", tm_struct->tm_mday);
	fMessage.AddInt32("year", tm_struct->tm_year);

	fMessage.AddInt32("hour", tm_struct->tm_hour);
	fMessage.AddInt32("minute", tm_struct->tm_min);
	fMessage.AddInt32("second", tm_struct->tm_sec);

	SendNotices(H_TM_CHANGED, &fMessage);
}

