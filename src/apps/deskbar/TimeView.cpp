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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/


#include "TimeView.h"

#include <string.h>

#include <Catalog.h>
#include <Debug.h>
#include <Locale.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <Window.h>

#include "CalendarMenuWindow.h"


static const char*  const kMinString = "99:99 AM";
static const float kHMargin = 2.0;


enum {
	kShowClock,
	kChangeClock,
	kHide,
	kShowCalendar
};


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "TimeView"

TTimeView::TTimeView(float maxWidth, float height, bool showSeconds,
	bool)
	:
	BView(BRect(-100, -100, -90, -90), "_deskbar_tv_",
	B_FOLLOW_RIGHT | B_FOLLOW_TOP,
	B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS),
	fParent(NULL),
	fShowInterval(true), // ToDo: defaulting this to true until UI is in place
	fShowSeconds(showSeconds),
	fMaxWidth(maxWidth),
	fHeight(height),
	fOrientation(true)
{
	fTime = fLastTime = time(NULL);
	fSeconds = fMinute = fHour = 0;
	fTimeStr[0] = 0;
	fDateStr[0] = 0;
	fLastTimeStr[0] = 0;
	fLastDateStr[0] = 0;
	fNeedToUpdate = true;

	fLocale = *BLocale::Default();
}


#ifdef AS_REPLICANT
TTimeView::TTimeView(BMessage* data)
	: BView(data)
{
	fTime = fLastTime = time(NULL);
	data->FindBool("seconds", &fShowSeconds);

	fLocale = *BLocale::Default();
}
#endif


TTimeView::~TTimeView()
{
}


#ifdef AS_REPLICANT
BArchivable*
TTimeView::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, "TTimeView"))
		return NULL;

	return new TTimeView(data);
}


status_t
TTimeView::Archive(BMessage* data, bool deep) const
{
	BView::Archive(data, deep);
	data->AddBool("seconds", fShowSeconds);
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

	ResizeToPreferred();
	CalculateTextPlacement();
}


void
TTimeView::GetPreferredSize(float* width, float* height)
{
	*height = fHeight;

	GetCurrentTime();

	// TODO: SetOrientation never gets called, fix that when in vertical mode,
	// we want to limit the width so that it can't overlap the bevels in the
	// parent view.
	*width = fOrientation ?
		min_c(fMaxWidth - kHMargin, kHMargin + StringWidth(fTimeStr))
		: kHMargin + StringWidth(fTimeStr);
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
		case kShowSeconds:
			ShowSeconds(!ShowingSeconds());
			break;

		case B_LOCALE_CHANGED:
			Update();
			break;

		case kChangeClock:
			// launch the time prefs app
			be_roster->Launch("application/x-vnd.Haiku-Time");
			break;

		case 'time':
			Window()->PostMessage(message, Parent());
			break;

		case kShowCalendar:
		{
			BRect bounds(Bounds());
			BPoint center(bounds.LeftTop());
			center += BPoint(bounds.Width() / 2, bounds.Height() / 2);
			ShowCalendar(center);
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TTimeView::ShowCalendar(BPoint where)
{
	if (fCalendarWindow.IsValid()) {
		// If the calendar is already shown, just activate it
		BMessage activate(B_SET_PROPERTY);
		activate.AddSpecifier("Active");
		activate.AddBool("data", true);

		if (fCalendarWindow.SendMessage(&activate) == B_OK)
			return;
	}

	where.y = Bounds().bottom + 4.0;
	ConvertToScreen(&where);

	if (where.y >= BScreen().Frame().bottom)
		where.y -= (Bounds().Height() + 4.0);

	CalendarMenuWindow* window = new CalendarMenuWindow(where);
	fCalendarWindow = BMessenger(window);

	window->Show();
}


void
TTimeView::GetCurrentTime()
{
	fLocale.FormatTime(fTimeStr, 64, fTime,
		fShowSeconds ? B_MEDIUM_TIME_FORMAT : B_SHORT_TIME_FORMAT);
}


void
TTimeView::GetCurrentDate()
{
	char tmp[64];

	fLocale.FormatDate(tmp, 64, fTime, B_FULL_DATE_FORMAT);

	// remove leading 0 from date when month is less than 10 (MM/DD/YY)
	// or remove leading 0 from date when day is less than 10 (DD/MM/YY)
	const char* str = tmp;
	if (str[0] == '0')
		str++;

	strlcpy(fDateStr, str, sizeof(fDateStr));
}


void
TTimeView::Draw(BRect /*updateRect*/)
{
	PushState();

	SetHighColor(ViewColor());
	SetLowColor(ViewColor());
	FillRect(Bounds());
	SetHighColor(0, 0, 0, 255);

	DrawString(fTimeStr, fTimeLocation);

	PopState();
}


void
TTimeView::MouseDown(BPoint point)
{
	uint32 buttons;

	Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);
	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		ShowClockOptions(ConvertToScreen(point));
		return;
	} else if (buttons == B_PRIMARY_MOUSE_BUTTON)
		ShowCalendar(point);

	// invalidate last time/date strings and call the pulse
	// method directly to change the display instantly
	fLastDateStr[0] = '\0';
	fLastTimeStr[0] = '\0';
	Pulse();
}


void
TTimeView::Pulse()
{
	time_t curTime = time(NULL);
	tm* ct = localtime(&curTime);
	if (ct == NULL)
		return;

	fTime = curTime;

	GetCurrentTime();
	GetCurrentDate();
	if (strcmp(fTimeStr, fLastTimeStr) != 0) {
		// Update bounds when the size of the strings has changed
		// For dates, Update() could be called two times in a row,
		// but that should only happen very rarely
		if ((fLastTimeStr[1] != fTimeStr[1]
			&& (fLastTimeStr[1] == ':' || fTimeStr[1] == ':'))
			|| !fLastTimeStr[0])
			Update();

		strlcpy(fLastTimeStr, fTimeStr, sizeof(fLastTimeStr));
		fNeedToUpdate = true;
	}

	// Update the tooltip if the date has changed
	if (strcmp(fDateStr, fLastDateStr) != 0) {
		strlcpy(fLastDateStr, fDateStr, sizeof(fLastDateStr));
		SetToolTip(fDateStr);
	}

	if (fNeedToUpdate) {
		fSeconds = ct->tm_sec;
		fMinute = ct->tm_min;
		fHour = ct->tm_hour;

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
TTimeView::Update()
{
	fLocale = *BLocale::Default();
	GetCurrentTime();
	GetCurrentDate();

	SetToolTip(fDateStr);

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

	fDateLocation.x = 0.0;
	fTimeLocation.x = 0.0;

	BFont font;
	GetFont(&font);
	const char* stringArray[1];
	stringArray[0] = fTimeStr;
	BRect rectArray[1];
	escapement_delta delta = { 0.0, 0.0 };
	font.GetBoundingBoxesForStrings(stringArray, 1, B_SCREEN_METRIC, &delta,
		rectArray);

	fTimeLocation.y = fDateLocation.y = ceilf((bounds.Height()
		- rectArray[0].Height() + 1.0) / 2.0 - rectArray[0].top);
}


void
TTimeView::ShowClockOptions(BPoint point)
{
	BPopUpMenu* menu = new BPopUpMenu("", false, false);
	menu->SetFont(be_plain_font);
	BMenuItem* item;

	item = new BMenuItem(B_TRANSLATE("Change time" B_UTF8_ELLIPSIS),
		new BMessage(kChangeClock));
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Hide time"), new BMessage('time'));
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Show calendar" B_UTF8_ELLIPSIS),
		new BMessage(kShowCalendar));
	menu->AddItem(item);

	menu->SetTargetForItems(this);
	// Changed to accept screen coord system point;
	// not constrained to this view now
	menu->Go(point, true, true, BRect(point.x - 4, point.y - 4,
		point.x + 4, point.y +4), true);
}

