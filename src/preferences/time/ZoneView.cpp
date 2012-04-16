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
 *		Hamish Morrison <hamish@lavabit.com>
 */


#include "ZoneView.h"

#include <stdlib.h>
#include <syscalls.h>

#include <map>
#include <new>
#include <vector>

#include <AutoDeleter.h>
#include <Button.h>
#include <Catalog.h>
#include <Collator.h>
#include <ControlLook.h>
#include <Country.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <ListItem.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <OutlineListView.h>
#include <Path.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <StorageDefs.h>
#include <String.h>
#include <StringView.h>
#include <TimeZone.h>
#include <ToolTip.h>
#include <View.h>
#include <Window.h>

#include <unicode/datefmt.h>
#include <unicode/utmscale.h>
#include <ICUWrapper.h>

#include "TimeMessages.h"
#include "TimeZoneListItem.h"
#include "TZDisplay.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Time"


using BPrivate::MutableLocaleRoster;
using BPrivate::ObjectDeleter;


struct TimeZoneItemLess {
	bool operator()(const BString& first, const BString& second)
	{
		// sort anything starting with '<' behind anything else
		if (first.ByteAt(0) == '<') {
			if (second.ByteAt(0) != '<')
				return false;
		} else if (second.ByteAt(0) == '<')
			return true;
		return fCollator.Compare(first.String(), second.String()) < 0;
	}
private:
	BCollator fCollator;
};



TimeZoneView::TimeZoneView(const char* name)
	:
	BGroupView(name, B_HORIZONTAL, B_USE_DEFAULT_SPACING),
	fGmtTime(NULL),
	fToolTip(NULL),
	fUseGmtTime(false),
	fCurrentZoneItem(NULL),
	fOldZoneItem(NULL),
	fInitialized(false)
{
	_ReadRTCSettings();
	_InitView();
}


bool
TimeZoneView::CheckCanRevert()
{
	// check GMT vs Local setting
	bool enable = fUseGmtTime != fOldUseGmtTime;

	return enable || fCurrentZoneItem != fOldZoneItem;
}


TimeZoneView::~TimeZoneView()
{
	if (fToolTip != NULL)
		fToolTip->ReleaseReference();
	_WriteRTCSettings();
}


void
TimeZoneView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	if (!fInitialized) {
		fInitialized = true;

		fSetZone->SetTarget(this);
		fZoneList->SetTarget(this);
	}
}


void
TimeZoneView::DoLayout()
{
	BView::DoLayout();
	if (fCurrentZoneItem != NULL) {
		fZoneList->Select(fZoneList->IndexOf(fCurrentZoneItem));
		fCurrent->SetText(fCurrentZoneItem->Text());
		fZoneList->ScrollToSelection();
	}
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

		case H_CITY_CHANGED:
			_UpdatePreview();
			break;

		case H_SET_TIME_ZONE:
		{
			_SetSystemTimeZone();
			break;
		}

		case kMsgRevert:
			_Revert();
			break;

		case kRTCUpdate:
			fUseGmtTime = fGmtTime->Value() == B_CONTROL_ON;
			_UpdateGmtSettings();
			_UpdateCurrent();
			_UpdatePreview();
			break;

		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


bool
TimeZoneView::GetToolTipAt(BPoint point, BToolTip** _tip)
{
	TimeZoneListItem* item = static_cast<TimeZoneListItem*>(
		fZoneList->ItemAt(fZoneList->IndexOf(point)));
	if (item == NULL || !item->HasTimeZone())
		return false;

	BString nowInTimeZone;
	time_t now = time(NULL);
	BLocale::Default()->FormatTime(&nowInTimeZone, now, B_SHORT_TIME_FORMAT,
		&item->TimeZone());

	BString dateInTimeZone;
	BLocale::Default()->FormatDate(&dateInTimeZone, now, B_SHORT_DATE_FORMAT,
		&item->TimeZone());

	BString toolTip = item->Text();
	toolTip << '\n' << item->TimeZone().ShortName() << " / "
			<< item->TimeZone().ShortDaylightSavingName()
			<< B_TRANSLATE("\nNow: ") << nowInTimeZone
			<< " (" << dateInTimeZone << ')';

	if (fToolTip != NULL)
		fToolTip->ReleaseReference();
	fToolTip = new (std::nothrow) BTextToolTip(toolTip.String());
	if (fToolTip == NULL)
		return false;

	*_tip = fToolTip;

	return true;
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
	fZoneList = new BOutlineListView("cityList", B_SINGLE_SELECTION_LIST);
	fZoneList->SetSelectionMessage(new BMessage(H_CITY_CHANGED));
	fZoneList->SetInvocationMessage(new BMessage(H_SET_TIME_ZONE));
	_BuildZoneMenu();
	BScrollView* scrollList = new BScrollView("scrollList", fZoneList,
		B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	scrollList->SetExplicitMinSize(BSize(200, 0));

	fCurrent = new TTZDisplay("currentTime", B_TRANSLATE("Current time:"));
	fPreview = new TTZDisplay("previewTime", B_TRANSLATE("Preview time:"));

	fSetZone = new BButton("setTimeZone", B_TRANSLATE("Set time zone"),
		new BMessage(H_SET_TIME_ZONE));
	fSetZone->SetEnabled(false);
	fSetZone->SetExplicitAlignment(
		BAlignment(B_ALIGN_RIGHT, B_ALIGN_BOTTOM));

	BStringView* text = new BStringView("clockSetTo",
		B_TRANSLATE("Hardware clock set to:"));
	fLocalTime = new BRadioButton("localTime",
		B_TRANSLATE("Local time (Windows compatible)"), new BMessage(kRTCUpdate));
	fGmtTime = new BRadioButton("greenwichMeanTime",
		B_TRANSLATE("GMT (UNIX compatible)"), new BMessage(kRTCUpdate));

	if (fUseGmtTime)
		fGmtTime->SetValue(B_CONTROL_ON);
	else
		fLocalTime->SetValue(B_CONTROL_ON);
	_ShowOrHidePreview();
	fOldUseGmtTime = fUseGmtTime;


	const float kInset = be_control_look->DefaultItemSpacing();
	BLayoutBuilder::Group<>(this)
		.Add(scrollList)
		.AddGroup(B_VERTICAL, kInset)
			.Add(text)
			.AddGroup(B_VERTICAL, kInset)
				.Add(fLocalTime)
				.Add(fGmtTime)
			.End()
			.AddGlue()
			.Add(fCurrent)
			.Add(fPreview)
			.Add(fSetZone)
		.End()
		.SetInsets(kInset, kInset, kInset, kInset);
}


void
TimeZoneView::_BuildZoneMenu()
{
	BTimeZone defaultTimeZone;
	BLocaleRoster::Default()->GetDefaultTimeZone(&defaultTimeZone);

	BLanguage language;
	BLocale::Default()->GetLanguage(&language);

	/*
	 * Group timezones by regions, but filter out unwanted (duplicate) regions
	 * and add an additional region with generic GMT-offset timezones at the end
	 */
	typedef	std::map<BString, TimeZoneListItem*, TimeZoneItemLess> ZoneItemMap;
	ZoneItemMap zoneItemMap;
	const char* kOtherRegion = B_TRANSLATE_MARK("<Other>");
	const char* kSupportedRegions[] = {
		B_TRANSLATE_MARK("Africa"),		B_TRANSLATE_MARK("America"),
		B_TRANSLATE_MARK("Antarctica"),	B_TRANSLATE_MARK("Arctic"),
		B_TRANSLATE_MARK("Asia"),		B_TRANSLATE_MARK("Atlantic"),
		B_TRANSLATE_MARK("Australia"),	B_TRANSLATE_MARK("Europe"),
		B_TRANSLATE_MARK("Indian"),		B_TRANSLATE_MARK("Pacific"),
		kOtherRegion,
		NULL
	};
	// Since the zone-map contains translated country-names (we get those from
	// ICU), we need to use translated region names in the zone-map, too:
	typedef	std::map<BString, BString> TranslatedRegionMap;
	TranslatedRegionMap regionMap;
	for (const char** region = kSupportedRegions; *region != NULL; ++region) {
		BString translatedRegion = B_TRANSLATE_NOCOLLECT(*region);
		regionMap[*region] = translatedRegion;

		TimeZoneListItem* regionItem
			= new TimeZoneListItem(translatedRegion, NULL, NULL);
		regionItem->SetOutlineLevel(0);
		zoneItemMap[translatedRegion] = regionItem;
	}

	// Get all time zones
	BMessage zoneList;
	BLocaleRoster::Default()->GetAvailableTimeZonesWithRegionInfo(&zoneList);

	typedef std::map<BString, std::vector<const char*> > ZonesByCountyMap;
	ZonesByCountyMap zonesByCountryMap;
	const char *zoneID;
	BString countryCode;
	for (int tz = 0; zoneList.FindString("timeZone", tz, &zoneID) == B_OK
			&& zoneList.FindString("region", tz, &countryCode) == B_OK; tz++) {
		// From the global ("001") timezones, we only accept the generic GMT
		// timezones, as all the other world-zones are duplicates of others.
		if (countryCode == "001" && strncmp(zoneID, "Etc/GMT", 7) != 0)
			continue;
		zonesByCountryMap[countryCode].push_back(zoneID);
	}

	ZonesByCountyMap::const_iterator countryIter = zonesByCountryMap.begin();
	for (; countryIter != zonesByCountryMap.end(); ++countryIter) {
		BCountry country(countryIter->first.String());
		BString countryName;
		country.GetName(countryName);

		size_t zoneCountInCountry = countryIter->second.size();
		for (size_t tz = 0; tz < zoneCountInCountry; tz++) {
			BString zoneID(countryIter->second[tz]);
			int32 slashPos = zoneID.FindFirst('/');

			BString region(zoneID, slashPos);
			if (region == "Etc")
				region = kOtherRegion;

			// just accept timezones from our supported regions, others are
			// aliases and would just make the list even longer
			TranslatedRegionMap::iterator regionIter = regionMap.find(region);
			if (regionIter == regionMap.end())
				continue;

			BString fullCountryID = regionIter->second;
			bool countryIsRegion
				= countryName == regionIter->second || region == kOtherRegion;
			if (!countryIsRegion)
				fullCountryID << "/" << countryName;

			BTimeZone* timeZone = new BTimeZone(zoneID, &language);
			BString tzName;
			BString fullZoneID = fullCountryID;
			if (zoneCountInCountry > 1)
			{
				// we can't use the country name as timezone name, since there
				// are more than one timezones in this country - fetch the
				// localized name of the timezone and use that
				tzName = timeZone->Name();
				int32 openParenthesisPos = tzName.FindFirst('(');
				if (openParenthesisPos >= 0) {
					tzName.Remove(0, openParenthesisPos + 1);
					int32 closeParenthesisPos = tzName.FindLast(')');
					if (closeParenthesisPos >= 0)
						tzName.Truncate(closeParenthesisPos);
				}
				fullZoneID << "/" << tzName;
			} else {
				tzName = countryName;
				fullZoneID << "/" << zoneID;
			}

			// skip duplicates
			ZoneItemMap::iterator zoneIter = zoneItemMap.find(fullZoneID);
			if (zoneIter != zoneItemMap.end()) {
				delete timeZone;
				continue;
			}

			TimeZoneListItem* countryItem = NULL;
			TimeZoneListItem* zoneItem = NULL;
			if (zoneCountInCountry > 1) {
				ZoneItemMap::iterator countryIter
					= zoneItemMap.find(fullCountryID);
				if (countryIter == zoneItemMap.end()) {
					countryItem = new TimeZoneListItem(countryName, NULL, NULL);
					countryItem->SetOutlineLevel(1);
					zoneItemMap[fullCountryID] = countryItem;
				} else
					countryItem = countryIter->second;

				zoneItem = new TimeZoneListItem(tzName, NULL, timeZone);
				zoneItem->SetOutlineLevel(countryIsRegion ? 1 : 2);
			} else {
				zoneItem = new TimeZoneListItem(tzName, NULL, timeZone);
				zoneItem->SetOutlineLevel(1);
			}
			zoneItemMap[fullZoneID] = zoneItem;

			if (timeZone->ID() == defaultTimeZone.ID()) {
				fCurrentZoneItem = zoneItem;
				if (countryItem != NULL)
					countryItem->SetExpanded(true);
				ZoneItemMap::iterator regionItemIter
					= zoneItemMap.find(regionIter->second);
				if (regionItemIter != zoneItemMap.end())
					regionItemIter->second->SetExpanded(true);
			}
		}
	}

	fOldZoneItem = fCurrentZoneItem;

	ZoneItemMap::iterator zoneIter;
	bool lastWasCountryItem = false;
	TimeZoneListItem* currentCountryItem = NULL;
	for (zoneIter = zoneItemMap.begin(); zoneIter != zoneItemMap.end();
		++zoneIter) {
		if (zoneIter->second->OutlineLevel() == 2 && lastWasCountryItem) {
			/* Some countries (e.g. Spain and Chile) have their timezones
			 * spread across different regions. As a result, there might still
			 * be country items with only one timezone below them. We manually
			 * filter those country items here.
			 */
			ZoneItemMap::iterator next = zoneIter;
			++next;
			if (next != zoneItemMap.end()
				&& next->second->OutlineLevel() != 2) {
				fZoneList->RemoveItem(currentCountryItem);
				zoneIter->second->SetText(currentCountryItem->Text());
				zoneIter->second->SetOutlineLevel(1);
				delete currentCountryItem;
			}
		}

		fZoneList->AddItem(zoneIter->second);
		if (zoneIter->second->OutlineLevel() == 1) {
			lastWasCountryItem = true;
			currentCountryItem = zoneIter->second;
		} else
			lastWasCountryItem = false;
	}
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

	fUseGmtTime = fOldUseGmtTime;
	if (fUseGmtTime)
		fGmtTime->SetValue(B_CONTROL_ON);
	else
		fLocalTime->SetValue(B_CONTROL_ON);
	_ShowOrHidePreview();

	_UpdateGmtSettings();
	_SetSystemTimeZone();
	_UpdatePreview();
	_UpdateCurrent();
}


void
TimeZoneView::_UpdatePreview()
{
	int32 selection = fZoneList->CurrentSelection();
	TimeZoneListItem* item
		= selection < 0
			? NULL
			: (TimeZoneListItem*)fZoneList->ItemAt(selection);

	if (item == NULL || !item->HasTimeZone()) {
		fPreview->SetText("");
		fPreview->SetTime("");
		return;
	}

	BString timeString = _FormatTime(item->TimeZone());
	fPreview->SetText(item->Text());
	fPreview->SetTime(timeString.String());

	fSetZone->SetEnabled((strcmp(fCurrent->Text(), item->Text()) != 0));
}


void
TimeZoneView::_UpdateCurrent()
{
	if (fCurrentZoneItem == NULL)
		return;

	BString timeString = _FormatTime(fCurrentZoneItem->TimeZone());
	fCurrent->SetText(fCurrentZoneItem->Text());
	fCurrent->SetTime(timeString.String());
}


void
TimeZoneView::_SetSystemTimeZone()
{
	/*	Set system timezone for all different API levels. How to do this?
	 *	1) tell locale-roster about new default timezone
	 *	2) tell kernel about new timezone offset
	 */

	int32 selection = fZoneList->CurrentSelection();
	if (selection < 0)
		return;

	TimeZoneListItem* item
		= static_cast<TimeZoneListItem*>(fZoneList->ItemAt(selection));
	if (item == NULL || !item->HasTimeZone())
		return;

	fCurrentZoneItem = item;
	const BTimeZone& timeZone = item->TimeZone();

	MutableLocaleRoster::Default()->SetDefaultTimeZone(timeZone);

	_kern_set_timezone(timeZone.OffsetFromGMT(), timeZone.ID().String(),
		timeZone.ID().Length());

	fSetZone->SetEnabled(false);
	fLastUpdateMinute = -1;
		// just to trigger updating immediately
}


BString
TimeZoneView::_FormatTime(const BTimeZone& timeZone)
{
	BString result;

	time_t now = time(NULL);
	bool rtcIsGMT;
	_kern_get_real_time_clock_is_gmt(&rtcIsGMT);
	if (!rtcIsGMT) {
		int32 currentOffset
			= fCurrentZoneItem != NULL && fCurrentZoneItem->HasTimeZone()
				? fCurrentZoneItem->OffsetFromGMT()
				: 0;
		now -= timeZone.OffsetFromGMT() - currentOffset;
	}
	BLocale::Default()->FormatTime(&result, now, B_SHORT_TIME_FORMAT,
		&timeZone);

	return result;
}


void
TimeZoneView::_ReadRTCSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("RTC_time_settings");

	BEntry entry(path.Path());
	if (entry.Exists()) {
		BFile file(&entry, B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			char buffer[6];
			file.Read(buffer, 6);
			if (strncmp(buffer, "gmt", 3) == 0)
				fUseGmtTime = true;
		}
	}
}


void
TimeZoneView::_WriteRTCSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return;

	path.Append("RTC_time_settings");

	BFile file(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() == B_OK) {
		if (fUseGmtTime)
			file.Write("gmt", 3);
		else
			file.Write("local", 5);
	}
}


void
TimeZoneView::_UpdateGmtSettings()
{
	_WriteRTCSettings();

	_ShowOrHidePreview();

	_kern_set_real_time_clock_is_gmt(fUseGmtTime);
}


void
TimeZoneView::_ShowOrHidePreview()
{
	if (fUseGmtTime) {
		// Hardware clock uses GMT time, changing timezone will adjust the
		// offset and we need to display a preview
		fCurrent->Show();
		fPreview->Show();
	} else {
		// Hardware clock uses local time, changing timezone will adjust the
		// clock and there is no offset to manage, thus, no preview.
		fCurrent->Hide();
		fPreview->Hide();
	}
}

