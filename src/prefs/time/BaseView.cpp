/*
	BaseView.cpp
		by Mike Berg (inseculous)
*/

#include <Alert.h>
#include <OS.h>
#include <stdio.h>

#include "BaseView.h"
#include "TimeMessages.h"


TTimeBaseView::TTimeBaseView(BRect frame, const char *name)
	: BView(frame, name, B_FOLLOW_ALL_SIDES, B_PULSE_NEEDED)
{
	InitView();
}


TTimeBaseView::~TTimeBaseView()
{
}



void
TTimeBaseView::InitView()
{
	f_message = new BMessage(H_TIME_UPDATE);
}


void
TTimeBaseView::Pulse()
{
	if (IsWatched())
		DispatchMessage();
}


void
TTimeBaseView::SetGMTime(bool gmt)
{
	f_gmtime = gmt;
}


void
TTimeBaseView::DispatchMessage()
{
	time_t current;
	struct tm *ltime;

	current = time(0);

	if (f_gmtime)
		ltime = gmtime(&current);
	else
		ltime = localtime(&current);
	
	int32 month = ltime->tm_mon;
	int32 day = ltime->tm_mday;
	int32 year = ltime->tm_year;
	int32 hour = ltime->tm_hour;
	int32 minute = ltime->tm_min;
	int32 second = ltime->tm_sec;
	
	f_message->MakeEmpty();
	f_message->AddInt32("month", month);
	f_message->AddInt32("day", day);
	f_message->AddInt32("year", year);
	f_message->AddInt32("hour", hour);
	f_message->AddInt32("minute", minute);
	f_message->AddInt32("second", second);

	SendNotices(H_TM_CHANGED, f_message);
}


void
TTimeBaseView::ChangeTime(BMessage *message)
{
	bool istime;
	if (!(message->FindBool("time", &istime) == B_OK))
		return;

	time_t atime;
	struct tm *_tm;
	
	
	atime = time(0);
	_tm = localtime(&atime);
	
	int32 hour = 0;
	int32 minute = 0;
	int32 second = 0;
	int32 month = 0;
	int32 day = 0;
	int32 year = 0;
	bool isam = false;
	if (istime) {
		if (message->FindInt32("hour", &hour) == B_OK)
			_tm->tm_hour = hour;
		if (message->FindInt32("minute", &minute) == B_OK)
			_tm->tm_min = minute;
		if (message->FindInt32("second", &second) == B_OK)
			_tm->tm_sec = second;
		if (message->FindBool("isam", &isam) == B_OK) {
			if (!isam) 
				_tm->tm_hour += 12;
		}		
	} else {
		if (message->FindInt32("month", &month) == B_OK)
			_tm->tm_mon = month;
		if (message->FindInt32("day", &day) == B_OK)
			_tm->tm_mday = day;
		if (message->FindInt32("year", &year) == B_OK)
			_tm->tm_year = year;
	}
	
	time_t atime2 = mktime(_tm);
	set_real_time_clock(atime2);
	DispatchMessage();
}

