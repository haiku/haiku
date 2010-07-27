/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Philippe Saint-Pierre <stpere@gmail.com>
 *		Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 */

/*
	Exceptions:
		doesn't calc "Time in" time.
*/

#include "ZoneView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <ListItem.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <StorageDefs.h>
#include <String.h>
#include <TimeZone.h>
#include <View.h>
#include <Window.h>

#include <syscalls.h>

#include "TimeMessages.h"
#include "TimeZoneListItem.h"
#include "TZDisplay.h"
#include "TimeWindow.h"


TimeZoneView::TimeZoneView(BRect frame)
	: BView(frame, "timeZoneView", B_FOLLOW_NONE, B_WILL_DRAW
		| B_NAVIGABLE_JUMP), fInitialized(false)
{
	fCurrentZone = NULL;
	fOldZone = NULL;
	// TODO : get default timezone from locale kit
	InitView();
}


bool
TimeZoneView::CheckCanRevert()
{
	return fCurrentZone != fOldZone;
}


void
TimeZoneView::_Revert()
{
	fCurrentZone = fOldZone;
	int32 czone = 0;

	if (fCurrentZone != NULL) {
		czone = fCityList->IndexOf(fCurrentZone);
		fCityList->Select(czone);
	}
		// TODO : else, select ??!

	fCityList->ScrollToSelection();
	fCurrent->SetText(((TimeZoneListItem *)fCityList->ItemAt(czone))->Text());
	SetPreview();
	SetTimeZone();
}


TimeZoneView::~TimeZoneView()
{
	delete fCurrentZone;
	delete fOldZone;
}


void
TimeZoneView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	if (!fInitialized) {
		fInitialized = true;

		fSetZone->SetTarget(this);
		fCityList->SetTarget(this);

		// update displays
		int32 czone = 0;
		if (fCurrentZone != NULL) {
			czone = fCityList->IndexOf(fCurrentZone);
		} else {
			// TODO : else, select ??!
			fCurrentZone = (TimeZoneListItem*)fCityList->ItemAt(0);
		}

		fCityList->Select(czone);

		fCityList->ScrollToSelection();
		fCurrent->SetText(fCurrentZone->Text());
	}
	fCityList->ScrollToSelection();
}


void
TimeZoneView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 change;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
			switch(change) {
				case H_TM_CHANGED:
					UpdateDateTime(message);
					break;

				default:
					BView::MessageReceived(message);
					break;
			}
			break;
		}

		case H_SET_TIME_ZONE:
		{
			SetTimeZone();
			((TTimeWindow*)Window())->SetRevertStatus();
			break;
		}

		case kMsgRevert:
			_Revert();
			break;

		case H_CITY_CHANGED:
			SetPreview();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TimeZoneView::UpdateDateTime(BMessage *message)
{
	int32 hour;
	int32 minute;

	// only need hour and minute
	if (message->FindInt32("hour", &hour) == B_OK
		&& message->FindInt32("minute", &minute) == B_OK) {
		if (fHour != hour || fMinute != minute) {
			fHour = hour;
			fMinute = minute;
			fCurrent->SetTime(hour, minute);

			// do calc to get other zone time
			if (fCityList->CurrentSelection() > -1)
				SetPreview();
		}
	}
}


void
TimeZoneView::InitView()
{
	// left side
	BRect frameLeft(Bounds());
	frameLeft.right = frameLeft.Width() / 2.0;
	frameLeft.InsetBy(10.0f, 10.0f);

	// City Listing
	fCityList = new BOutlineListView(frameLeft, "cityList",
		B_SINGLE_SELECTION_LIST);
	fCityList->SetSelectionMessage(new BMessage(H_CITY_CHANGED));
	fCityList->SetInvocationMessage(new BMessage(H_SET_TIME_ZONE));

	BuildRegionMenu();

	BScrollView *scrollList = new BScrollView("scrollList", fCityList,
		B_FOLLOW_ALL, 0, false, true);
	AddChild(scrollList);

	// right side
	BRect frameRight(Bounds());
	frameRight.left = frameRight.Width() / 2.0;
	frameRight.InsetBy(10.0f, 10.0f);
	frameRight.top = frameLeft.top;

	// Time Displays
	fCurrent = new TTZDisplay(frameRight, "currentTime", "Current time:");
	AddChild(fCurrent);
	fCurrent->ResizeToPreferred();

	frameRight.top = fCurrent->Frame().bottom + 10.0;
	fPreview = new TTZDisplay(frameRight, "previewTime", "Preview time:");
	AddChild(fPreview);
	fPreview->ResizeToPreferred();

	// set button
	fSetZone = new BButton(frameRight, "setTimeZone", "Set time zone",
		new BMessage(H_SET_TIME_ZONE));
	AddChild(fSetZone);
	fSetZone->SetEnabled(false);
	fSetZone->ResizeToPreferred();

	fSetZone->MoveTo(frameRight.right - fSetZone->Bounds().Width(),
		scrollList->Frame().bottom - fSetZone->Bounds().Height());
}


void
TimeZoneView::BuildRegionMenu()
{
	// Get a list of countries
	// For each country, get all the timezones and AddItemUnder them
	//	(only if there are multiple ones ?)
	// Unfold the current country and highlight the selected TZ

	BMessage countryList;
	be_locale_roster->GetAvailableCountries(&countryList);

	BString countryCode;
	for (int i = 0; countryList.FindString("countries", i, &countryCode)
			== B_OK; i++) {
		BCountry country("", countryCode);
		BString fullName;
		country.Name(fullName);

		// Now list the timezones for this country
		BList tzList;

		TimeZoneListItem* countryItem;
		switch (country.GetTimeZones(tzList))
		{
			case 0:
				// No TZ info for this country. Sorry guys!
				break;
			case 1:
				// Only one Timezone, no need to add it to the list
				countryItem = new TimeZoneListItem(fullName, &country,
					(BTimeZone*)tzList.ItemAt(0));
				fCityList->AddItem(countryItem);
				break;
			default:
				countryItem = new TimeZoneListItem(fullName, &country, NULL);
				fCityList->AddItem(countryItem);

				BTimeZone* timeZone;
				for (int j = 0; timeZone = (BTimeZone*)tzList.ItemAt(j); j++) {
					BString readableName;
					timeZone->GetName(readableName);
					BStringItem* tzItem = new TimeZoneListItem(readableName,
						NULL, timeZone);
					fCityList->AddUnder(tzItem, countryItem);
				}
				break;
		}
	}

	fCurrentZone = fOldZone = (TimeZoneListItem*)fCityList->ItemAt(0);
		// TODO get the actual setting from locale kit
}


void
TimeZoneView::SetPreview()
{
	int32 selection = fCityList->CurrentSelection();
	if (selection >= 0) {
		TimeZoneListItem *item = (TimeZoneListItem *)fCityList->ItemAt(
			selection);

		// set timezone to selection
		char buffer[50];
		item->Code(buffer);
		SetTimeZone(buffer);

		// calc preview time
		time_t current = time(NULL);
		struct tm localTime;
		localtime_r(&current, &localTime);

		// update prview
		fPreview->SetText(item->Text());
		fPreview->SetTime(localTime.tm_hour, localTime.tm_min);

		fCurrentZone->Code(buffer);
		SetTimeZone(buffer);

		fSetZone->SetEnabled((strcmp(fCurrent->Text(), item->Text()) != 0));
	}
}


void
TimeZoneView::SetCurrent(const char *text)
{
	char buffer[50];
	fCurrentZone->Code(buffer);
	SetTimeZone(buffer);

	time_t current = time(NULL);
	struct tm localTime;
	localtime_r(&current, &localTime);

	fCurrent->SetText(text);
	fCurrent->SetTime(localTime.tm_hour, localTime.tm_min);
}


void
TimeZoneView::SetTimeZone()
{
	/*	set time based on supplied timezone. How to do this?
		1) replace symlink "timezone" in B_USER_SETTINGS_DIR with a link to the
			new timezone
		2) set TZ environment var
		3) call settz()
		4) call set_timezone from OS.h passing path to timezone file
	*/

	int32 selection = fCityList->CurrentSelection();
	if (selection < 0)
		return;

	char timeZoneCode[50];
	((TimeZoneListItem *)fCityList->ItemAt(selection))->Code(timeZoneCode);

	// update environment
	SetTimeZone(timeZoneCode);

	// update display
	time_t current = time(NULL);
	struct tm localTime;
	localtime_r(&current, &localTime);

	set_timezone(timeZoneCode);
	// disable button
	fSetZone->SetEnabled(false);

	time_t newTime = mktime(&localTime);
	localtime_r(&newTime, &localTime);
	stime(&newTime);

	fHour = localTime.tm_hour;
	fMinute = localTime.tm_min;
	fCurrentZone = (TimeZoneListItem*)(fCityList->ItemAt(selection));
	SetCurrent(((TimeZoneListItem *)fCityList->ItemAt(selection))->Text());
}


void
TimeZoneView::SetTimeZone(const char *zone)
{
	putenv(BString("TZ=").Append(zone).String());
	tzset();

	be_locale_roster->SetDefaultTimeZone(zone);
}

