#include <Alert.h>
#include <stdio.h>

#include "BaseView.h"

TTimeBaseView::TTimeBaseView(BRect frame, const char *name)
	: BView(frame, name, B_FOLLOW_ALL_SIDES, B_PULSE_NEEDED)
{	
	f_message = new BMessage(OB_TIME_UPDATE);
}

TTimeBaseView::~TTimeBaseView()
{	}

void
TTimeBaseView::Pulse()
{
	time_t current;
	struct tm *ltime;

	current = time(0);
	ltime = localtime(&current);
	
	f_message->MakeEmpty();
	f_message->AddPointer("_tm_", ltime);
	if (IsWatched())
	{
		SendNotices(OB_TIME_UPDATE, f_message);
	}
}
