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


#include "ZoneView.h"

#include <stdlib.h>

#include <new>

#include <AutoDeleter.h>
#include <Button.h>
#include <Collator.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <ListItem.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <OutlineListView.h>
#include <Path.h>
#include <ScrollView.h>
#include <StorageDefs.h>
#include <String.h>
#include <TimeZone.h>
#include <View.h>
#include <Window.h>

#include <localtime.h>
#include <syscalls.h>

#include <unicode/datefmt.h>
#include <unicode/utmscale.h>
#include <ICUWrapper.h>

#include "TimeMessages.h"
#include "TimeZoneListItem.h"
#include "TZDisplay.h"
#include "TimeWindow.h"


using BPrivate::gMutableLocaleRoster;
using BPrivate::ObjectDeleter;


TimeZoneView::TimeZoneView(BRect frame)
	:
	BView(frame, "timeZoneView", B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE_JUMP),
	fCurrentZoneItem(NULL),
	fOldZoneItem(NULL),
	fInitialized(false)
{
	_InitView();
}


bool
TimeZoneView::CheckCanRevert()
{
	return fCurrentZoneItem != fOldZoneItem;
}


void
TimeZoneView::_Revert()
{
	fCurrentZoneItem = fOldZoneItem;

	if (fCurrentZoneItem != NULL) {
		int32 currentZoneIndex = fZoneList->IndexOf(fCurrentZoneItem);
		fZoneList->Select(currentZoneIndex);
	} else
		fZoneList->DeselectAll();
	fZoneList->ScrollToSelection();

	_SetSystemTimeZone();
	_UpdatePreview();
	_UpdateCurrent();
}


TimeZoneView::~TimeZoneView()
{
}


void
TimeZoneView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	if (!fInitialized) {
		fInitialized = true;

		fSetZone->SetTarget(this);
		fZoneList->SetTarget(this);

		// update displays
		int32 czone = 0;
		if (fCurrentZoneItem != NULL) {
			czone = fZoneList->IndexOf(fCurrentZoneItem);
		} else {
			// TODO : else, select ??!
			fCurrentZoneItem = (TimeZoneListItem*)fZoneList->ItemAt(0);
		}

		fZoneList->Select(czone);

		fZoneList->ScrollToSelection();
		fCurrent->SetText(fCurrentZoneItem->Text());
	}
	fZoneList->ScrollToSelection();
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
			_SetSystemTimeZone();
			((TTimeWindow*)Window())->SetRevertStatus();
			break;
		}

		case kMsgRevert:
			_Revert();
			break;

		case H_CITY_CHANGED:
			_UpdatePreview();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TimeZoneView::_UpdateDateTime(BMessage* message)
{
	// only need to update once every minute
	int32 minute;
	if (message->FindInt32("minute", &minute) == B_OK) {
		if (fLastUpdateMinute != minute) {
			_UpdateCurrent();
			_UpdatePreview();

			fLastUpdateMinute = minute;
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
	fZoneList = new BOutlineListView(frameLeft, "cityList",
		B_SINGLE_SELECTION_LIST);
	fZoneList->SetSelectionMessage(new BMessage(H_CITY_CHANGED));
	fZoneList->SetInvocationMessage(new BMessage(H_SET_TIME_ZONE));

	_BuildRegionMenu();

	BScrollView* scrollList = new BScrollView("scrollList", fZoneList,
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
	BTimeZone defaultTimeZone = NULL;
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
		country.GetName(fullName);

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
				fZoneList->AddItem(countryItem);
				if (timeZone->Code() == defaultTimeZone.Code())
					fCurrentZoneItem = countryItem;
				break;
			default:
				countryItem = new TimeZoneListItem(fullName, &country, NULL);
				countryItem->SetExpanded(false);
				fZoneList->AddItem(countryItem);

				for (int j = 0;
						(timeZone = (BTimeZone*)tzList.ItemAt(j)) != NULL;
						j++) {
					TimeZoneListItem* tzItem = new TimeZoneListItem(
						timeZone->Name(), NULL, timeZone);
					fZoneList->AddUnder(tzItem, countryItem);
					if (timeZone->Code() == defaultTimeZone.Code())
					{
						fCurrentZoneItem = tzItem;
						countryItem->SetExpanded(true);
					}
				}
				break;
		}
	}

	// add an artifical (i.e. belongs to no country) entry for GMT/UTC
	BTimeZone* gmtZone = new(std::nothrow) BTimeZone(BTimeZone::kNameOfGmtZone);
	TimeZoneListItem* gmtItem
		= new TimeZoneListItem("- GMT/UTC (Universal Time) -", NULL, gmtZone);
	fZoneList->AddItem(gmtItem);
	if (gmtZone->Code() == defaultTimeZone.Code())
		fCurrentZoneItem = gmtItem;

	fOldZoneItem = fCurrentZoneItem;

	struct ListSorter {
		static int compare(const BListItem* first, const BListItem* second)
		{
			static BCollator collator;
			return collator.Compare(((BStringItem*)first)->Text(),
				((BStringItem*)second)->Text());
		}
	};
	fZoneList->SortItemsUnder(NULL, false, ListSorter::compare);
}


void
TimeZoneView::_UpdatePreview()
{
	int32 selection = fZoneList->CurrentSelection();
	if (selection < 0)
		return;

	TimeZoneListItem* item = (TimeZoneListItem*)fZoneList->ItemAt(selection);
	if (!item->HasTimeZone())
		return;

	BString timeString = _FormatTime(item);
	fPreview->SetText(item->Text());
	fPreview->SetTime(timeString.String());

	fSetZone->SetEnabled((strcmp(fCurrent->Text(), item->Text()) != 0));
}


void
TimeZoneView::_UpdateCurrent()
{
	if (fCurrentZoneItem == NULL)
		return;

	BString timeString = _FormatTime(fCurrentZoneItem);
	fCurrent->SetText(fCurrentZoneItem->Text());
	fCurrent->SetTime(timeString.String());
}


void
TimeZoneView::_SetSystemTimeZone()
{
	/*	Set sytem timezone for all different API levels. How to do this?
	 *	1) tell locale-roster about new default timezone
	 *	2) tell kernel about new timezone offset
	 *	3) write new POSIX-timezone-info file
	 */

	int32 selection = fZoneList->CurrentSelection();
	if (selection < 0)
		return;

	fCurrentZoneItem = (TimeZoneListItem*)(fZoneList->ItemAt(selection));
	const BTimeZone& timeZone = fCurrentZoneItem->TimeZone();

	gMutableLocaleRoster->SetDefaultTimeZone(timeZone);

	_kern_set_timezone(timeZone.OffsetFromGMT());

	BPath path;
	status_t status = find_directory(B_COMMON_SETTINGS_DIRECTORY, &path, true);
	BFile file;
	if (status == B_OK) {
		path.Append(BPrivate::skPosixTimeZoneInfoFile);
		status = file.SetTo(path.Path(),
			B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	}
	if (status == B_OK) {
		BPrivate::time_zone_info tzInfo;
		tzInfo.offset_from_gmt = timeZone.OffsetFromGMT();
		tzInfo.uses_daylight_saving = timeZone.SupportsDaylightSaving();
		strlcpy(tzInfo.short_std_name, timeZone.ShortName().String(),
			BPrivate::skTimeZoneInfoNameMax);
		strlcpy(tzInfo.short_dst_name,
			timeZone.ShortDaylightSavingName().String(),
			BPrivate::skTimeZoneInfoNameMax);
		file.Write(&tzInfo, sizeof(tzInfo));
		file.Sync();
	}

	fSetZone->SetEnabled(false);
	fLastUpdateMinute = -1;
		// just to trigger updating immediately
}


BString
TimeZoneView::_FormatTime(TimeZoneListItem* zoneItem)
{
	BString result;

	if (zoneItem == NULL)
		return result;

	time_t nowInTimeZone = time(NULL) + zoneItem->OffsetFromGMT();
	BLocale locale;
	be_locale_roster->GetDefaultLocale(&locale);
	locale.FormatTime(&result, nowInTimeZone, false);

	return result;
}
