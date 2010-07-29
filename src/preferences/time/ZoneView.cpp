/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Philippe Saint-Pierre <stpere@gmail.com>
 *		Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */

/*
	Exceptions:
		doesn't calc "Time in" time.
*/

#include "ZoneView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Button.h>
#include <Collator.h>
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


static BCollator sCollator;
	// used to sort the timezone list


TimeZoneView::TimeZoneView(BRect frame)
	:
	BView(frame, "timeZoneView", B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE_JUMP),
	fCurrentZone(NULL),
	fOldZone(NULL),
	fInitialized(false)
{
	_InitView();
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
	fCurrent->SetText(((TimeZoneListItem*)fCityList->ItemAt(czone))->Text());
	_SetPreview();
	_SetTimeZone();
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
TimeZoneView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 change;
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
		}

		case H_SET_TIME_ZONE:
		{
			_SetTimeZone();
			((TTimeWindow*)Window())->SetRevertStatus();
			break;
		}

		case kMsgRevert:
			_Revert();
			break;

		case H_CITY_CHANGED:
			_SetPreview();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TimeZoneView::_UpdateDateTime(BMessage* message)
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
				_SetPreview();
		}
	}
}


void
TimeZoneView::_InitView()
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

	_BuildRegionMenu();

	BScrollView* scrollList = new BScrollView("scrollList", fCityList,
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
TimeZoneView::_BuildRegionMenu()
{
	BTimeZone* defaultTimeZone = NULL;
	be_locale_roster->GetDefaultTimeZone(&defaultTimeZone);

	// Get a list of countries and, for each country, get all the timezones and
	// AddUnder() them (only if there are multiple ones).
	// Finally expand the current country and highlight the active TZ.

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

		BTimeZone* timeZone;
		TimeZoneListItem* countryItem;
		switch (country.GetTimeZones(tzList))
		{
			case 0:
				// No TZ info for this country. Sorry guys!
				break;
			case 1:
				// Only one Timezone, no need to add it to the list
				timeZone = (BTimeZone*)tzList.ItemAt(0);
				countryItem
					= new TimeZoneListItem(fullName, &country, timeZone);
				fCityList->AddItem(countryItem);
				if (timeZone->Code() == defaultTimeZone->Code())
					fCurrentZone = countryItem;
				break;
			default:
				countryItem = new TimeZoneListItem(fullName, &country, NULL);
				countryItem->SetExpanded(false);
				fCityList->AddItem(countryItem);

				for (int j = 0;
						(timeZone = (BTimeZone*)tzList.ItemAt(j)) != NULL;
						j++) {
					TimeZoneListItem* tzItem = new TimeZoneListItem(
						timeZone->Name(), NULL, timeZone);
					fCityList->AddUnder(tzItem, countryItem);
					if (timeZone->Code() == defaultTimeZone->Code())
					{
						fCurrentZone = tzItem;
						countryItem->SetExpanded(true);
					}
				}
				break;
		}
	}

	fOldZone = fCurrentZone;

	delete defaultTimeZone;

	struct ListSorter {
		static int compare(const BListItem* first, const BListItem* second)
		{
			return sCollator.Compare(((BStringItem*)first)->Text(),
				((BStringItem*)second)->Text());
		}
	};
	fCityList->SortItemsUnder(NULL, true, ListSorter::compare);

}


void
TimeZoneView::_SetPreview()
{
	int32 selection = fCityList->CurrentSelection();
	if (selection >= 0) {
		TimeZoneListItem* item
			= (TimeZoneListItem*)fCityList->ItemAt(selection);

		// calc preview time
		time_t current = time(NULL) + item->OffsetFromGMT();
		struct tm localTime;
		gmtime_r(&current, &localTime);

		// update prview
		fPreview->SetText(item->Text());
		fPreview->SetTime(localTime.tm_hour, localTime.tm_min);

		fSetZone->SetEnabled((strcmp(fCurrent->Text(), item->Text()) != 0));
	}
}


void
TimeZoneView::_SetCurrent(const char* text)
{
	_SetTimeZone(fCurrentZone->Code().String());

	time_t current = time(NULL);
	struct tm localTime;
	localtime_r(&current, &localTime);

	fCurrent->SetText(text);
	fCurrent->SetTime(localTime.tm_hour, localTime.tm_min);
}


void
TimeZoneView::_SetTimeZone()
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

	const BString& code
		= ((TimeZoneListItem*)fCityList->ItemAt(selection))->Code();
	_SetTimeZone(code.String());

	// update display
	time_t current = time(NULL);
	struct tm localTime;
	localtime_r(&current, &localTime);

	set_timezone(code.String());
	// disable button
	fSetZone->SetEnabled(false);

	time_t newTime = mktime(&localTime);
	localtime_r(&newTime, &localTime);
	stime(&newTime);

	fHour = localTime.tm_hour;
	fMinute = localTime.tm_min;
	fCurrentZone = (TimeZoneListItem*)(fCityList->ItemAt(selection));
	_SetCurrent(((TimeZoneListItem*)fCityList->ItemAt(selection))->Text());
}


void
TimeZoneView::_SetTimeZone(const char* zone)
{
	putenv(BString("TZ=").Append(zone).String());
	tzset();

	be_locale_roster->SetDefaultTimeZone(zone);
}

