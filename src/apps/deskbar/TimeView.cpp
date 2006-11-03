/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "TimeView.h"

#ifdef _SHOW_CALENDAR_MENU_ITEM
#	include "CalendarMenuItem.h"
#endif

#include <Debug.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Window.h>
#include <Screen.h>

#include <string.h>


const char *kShortDateFormat = "%m/%d/%y";
const char *kShortEuroDateFormat = "%d/%m/%y";
const char *kLongDateFormat = "%a, %B %d, %Y";
const char *kLongEuroDateFormat = "%a, %d %B, %Y";

static const char * const kMinString = "99:99 AM";


static float
FontHeight(BView *target, bool full)
{
	font_height fontInfo;		
	target->GetFontHeight(&fontInfo);
	float h = fontInfo.ascent + fontInfo.descent;

	if (full)
		h += fontInfo.leading;

	return h;
}


enum {
	kMsgShowClock,
	kMsgChangeClock,
	kMsgHide
};


TTimeView::TTimeView(bool showSeconds, bool milTime, bool fullDate, bool euroDate, bool)
	: 	BView(BRect(-100,-100,-90,-90), "_deskbar_tv_",
	B_FOLLOW_RIGHT | B_FOLLOW_TOP,
	B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS),
	fParent(NULL),
	fShowInterval(true), // ToDo: defaulting this to true until UI is in place
	fShowSeconds(showSeconds),
	fMilTime(milTime),
	fFullDate(fullDate),
	fCanShowFullDate(false),
	fEuroDate(euroDate),
	fOrientation(false)
{
	fShowingDate = false;
	fTime = fLastTime = time(NULL);
	fSeconds = fMinute = fHour = 0;
	fLastTimeStr[0] = 0;
	fLastDateStr[0] = 0;
	fNeedToUpdate = true;
}


#ifdef AS_REPLICANT
TTimeView::TTimeView(BMessage *data)
	: BView(data)
{
	fTime = fLastTime = time(NULL);
	data->FindBool("seconds", &fShowSeconds);
	data->FindBool("miltime", &fMilTime);
	data->FindBool("fulldate", &fFullDate);
	data->FindBool("eurodate", &fEuroDate);
	data->FindBool("interval", &fInterval);
	fShowingDate = false;
}
#endif


TTimeView::~TTimeView()
{
}


#ifdef AS_REPLICANT
BArchivable*
TTimeView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "TTimeView"))
		return NULL;

	return new TTimeView(data);
}


status_t
TTimeView::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);
	data->AddBool("seconds", fShowSeconds);
	data->AddBool("miltime", fMilTime);
	data->AddBool("fulldate", fFullDate);
	data->AddBool("eurodate", fEuroDate);
	data->AddBool("interval", fInterval);
	data->AddInt32("deskbar:private_align", B_ALIGN_RIGHT);

	return B_OK;
}
#endif


void
TTimeView::AttachedToWindow()
{
	fTime = time(NULL);

	SetFont(be_plain_font);
	if (Parent()) {
		fParent = Parent();
		SetViewColor(Parent()->ViewColor());
	} else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fFontHeight = FontHeight(this, true);		
	ResizeToPreferred();
	CalculateTextPlacement();
}


void
TTimeView::GetPreferredSize(float *width, float *height)
{
	*height = fFontHeight;

	GetCurrentTime();
	GetCurrentDate();

	if (ShowingDate())
		*width = 6 + StringWidth(fDateStr);
	else {
		*width = 6 + StringWidth(fTimeStr);
		// Changed this from 10 to 6 so even with interval + seconds, there still
		// is room for two replicants in the default tray.
	}
}


void
TTimeView::ResizeToPreferred()
{
	float width, height;
	float oldWidth = Bounds().Width(), oldHeight = Bounds().Height();

	GetPreferredSize(&width, &height);
	if (height != oldHeight || width != oldWidth) {
		ResizeTo(width, height);
		MoveBy(oldWidth - width, 0);
		fNeedToUpdate = true;
	}
}


void
TTimeView::FrameMoved(BPoint)
{
	Update();
}


void
TTimeView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgFullDate:
			ShowFullDate(!ShowingFullDate());
			break;

		case kMsgShowSeconds:
			ShowSeconds(!ShowingSeconds());
			break;

		case kMsgMilTime:
			ShowMilTime(!ShowingMilTime());
			break;

		case kMsgEuroDate:
			ShowEuroDate(!ShowingEuroDate());
			break;

		case kMsgChangeClock:
			// launch the time prefs app
			be_roster->Launch("application/x-vnd.Be-TIME");
			break;

		case 'time':
			Window()->PostMessage(message, Parent());
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
TTimeView::GetCurrentTime()
{
	char tmp[64];
	tm time = *localtime(&fTime);

	if (fMilTime) {
		strftime(tmp, 64, fShowSeconds ? "%H:%M:%S" : "%H:%M", &time);
	} else {
		if (fShowInterval)
			strftime(tmp, 64, fShowSeconds ? "%I:%M:%S %p" : "%I:%M %p", &time);
		else
			strftime(tmp, 64, fShowSeconds ? "%I:%M:%S" : "%I:%M", &time);
	}

	//	remove leading 0 from time when hour is less than 10
	const char *str = tmp;
	if (str[0] == '0')
		str++;

	strcpy(fTimeStr, str);

	fSeconds = time.tm_sec;
	fMinute = time.tm_min;
	fHour = time.tm_hour;
}


void
TTimeView::GetCurrentDate()
{
	char tmp[64];
	tm time = *localtime(&fTime);

	if (fFullDate && CanShowFullDate())
		strftime(tmp, 64, fEuroDate ? kLongEuroDateFormat : kLongDateFormat, &time);
	else
		strftime(tmp, 64, fEuroDate ? kShortEuroDateFormat : kShortDateFormat, &time);

	//	remove leading 0 from date when month is less than 10 (MM/DD/YY)
	//  or remove leading 0 from date when day is less than 10 (DD/MM/YY)
	const char* str = tmp;
	if (str[0] == '0')
		str++;

	strcpy(fDateStr, str);
}


void
TTimeView::Draw(BRect /*updateRect*/)
{
	PushState();

	SetHighColor(ViewColor());
	SetLowColor(ViewColor());
	FillRect(Bounds());
	SetHighColor(0, 0, 0, 255);

	if (fShowingDate)
		DrawString(fDateStr, fDateLocation);
	else
		DrawString(fTimeStr, fTimeLocation);

	PopState();
}


void
TTimeView::MouseDown(BPoint point)
{
	uint32 buttons;

	Window()->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);
	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		ShowClockOptions(ConvertToScreen(point));
		return;
	}

	//	flip to/from showing date or time
	fShowingDate = !fShowingDate;
	if (fShowingDate)
		fLastTime = time(NULL);

	// invalidate last time/date strings and call the pulse
	// method directly to change the display instantly
	fLastDateStr[0] = '\0';
	fLastTimeStr[0] = '\0';
	Pulse();

#ifdef _SHOW_CALENDAR_MENU_ITEM
	// see if the user holds down the button long enough to show him the calendar

	bigtime_t startTime = system_time();

	// use the doubleClickSpeed as a treshold
	bigtime_t doubleClickSpeed;
	get_click_speed(&doubleClickSpeed);

	while (buttons) {
		BPoint where;
		GetMouse(&where, &buttons, false);

		if ((system_time() - startTime) > doubleClickSpeed) {
			BPopUpMenu *menu = new BPopUpMenu("", false, false);
			menu->SetFont(be_plain_font);

			menu->AddItem(new CalendarMenuItem());
			menu->ResizeToPreferred();

			point = where;
			BScreen screen;
			where.y = Bounds().bottom + 4;

			// make sure the menu is visible at doesn't hide the date
			ConvertToScreen(&where);
			if (where.y + menu->Bounds().Height() > screen.Frame().bottom)
				where.y -= menu->Bounds().Height() + 2 * Bounds().Height();

			ConvertToScreen(&point);
			menu->Go(where, true, true, BRect(point.x - 4, point.y - 4,
				point.x + 4, point.y + 4), true);
			return;
		}

		snooze(15000);
	}
#endif
}


void
TTimeView::Pulse()
{
	time_t curTime = time(NULL);
	tm	ct = *localtime(&curTime);
	fTime = curTime;

	GetCurrentTime();
	GetCurrentDate();
	if ((!fShowingDate && strcmp(fTimeStr, fLastTimeStr) != 0)
		|| 	(fShowingDate && strcmp(fDateStr, fLastDateStr) != 0)) {
		// Update bounds when the size of the strings has changed
		// For dates, Update() could be called two times in a row,
		// but that should only happen very rarely
		if ((!fShowingDate && fLastTimeStr[1] != fTimeStr[1]
				&& (fLastTimeStr[1] == ':' || fTimeStr[1] == ':'))
			|| (fShowingDate && strlen(fDateStr) != strlen(fLastDateStr))
			|| !fLastTimeStr[0])
			Update();

		strcpy(fLastTimeStr, fTimeStr);
		strcpy(fLastDateStr, fDateStr);
		fNeedToUpdate = true;
	}

	if (fShowingDate && (fLastTime + 5 <= time(NULL))) {
		fShowingDate = false;
		Update();	// Needs to happen since size can change here
	}

	if (fNeedToUpdate) {
		fSeconds = ct.tm_sec;
		fMinute = ct.tm_min;
		fHour = ct.tm_hour;
		fInterval = ct.tm_hour >= 12;

		Draw(Bounds());
		fNeedToUpdate = false;
	}	
}


void
TTimeView::ShowSeconds(bool on)
{
	fShowSeconds = on;
	Update();
}


void
TTimeView::ShowMilTime(bool on)
{
	fMilTime = on;
	Update();
}


void
TTimeView::ShowDate(bool on)
{
	fShowingDate = on;
	Update();
}


void
TTimeView::ShowFullDate(bool on)
{
	fFullDate = on;
	Update();
}


void
TTimeView::ShowEuroDate(bool on)
{
	fEuroDate = on;
	Update();
}


void
TTimeView::AllowFullDate(bool allow)
{
	fCanShowFullDate = allow;

	if (allow != ShowingFullDate())
		Update();
}


void
TTimeView::Update()
{
	GetCurrentTime();
	GetCurrentDate();

	ResizeToPreferred();
	CalculateTextPlacement();

	if (fParent) {
		BMessage reformat('Trfm');
		fParent->MessageReceived(&reformat);
			// time string format realign
		fParent->Invalidate();
	}
}


void
TTimeView::SetOrientation(bool o)
{
	fOrientation = o;
	CalculateTextPlacement();
	Invalidate();
}


void
TTimeView::CalculateTextPlacement()
{
	BRect bounds(Bounds());

	if (fOrientation) {		// vertical mode
		fDateLocation.x = bounds.Width()/2 - StringWidth(fDateStr)/2;
		fTimeLocation.x = bounds.Width()/2 - StringWidth(fTimeStr)/2;
	} else {
		fTimeLocation.x = bounds.Width() - StringWidth(fTimeStr) - 5;
		fDateLocation.x = bounds.Width() - StringWidth(fDateStr) - 5;
	}
	//	center vertically
	fDateLocation.y = fTimeLocation.y = bounds.Height()/2 + fFontHeight/2;
}

		
void
TTimeView::ShowClockOptions(BPoint point)
{
	BPopUpMenu *menu = new BPopUpMenu("", false, false);
	menu->SetFont(be_plain_font);
	BMenuItem *item;

	item = new BMenuItem("Change Time" B_UTF8_ELLIPSIS, new BMessage(kMsgChangeClock));
	menu->AddItem(item);

	item = new BMenuItem("Hide Time", new BMessage('time'));
	menu->AddItem(item);

	menu->SetTargetForItems(this);
	// Changed to accept screen coord system point;
	// not constrained to this view now
	menu->Go(point, true, true, BRect(point.x - 4, point.y - 4,
		point.x + 4, point.y +4), true);
}
