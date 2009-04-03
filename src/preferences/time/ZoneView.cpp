/*
 * Copyright 2004-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Philippe Saint-Pierre <stpere@gmail.com>
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
#include <ListView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StorageDefs.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include <syscalls.h>

#include "TimeMessages.h"
#include "TZDisplay.h"
#include "TimeWindow.h"


class TZoneItem : public BStringItem {
	public:
					TZoneItem(const char *text, const char *zone)
						: BStringItem(text), fZone(new BPath(zone)) { }

					~TZoneItem() {	delete fZone;	}

		const char 	*Zone() const	{	return fZone->Leaf();	}
		const char 	*Path() const	{	return fZone->Path();	}

	private:
		BPath 		*fZone;
};


TimeZoneView::TimeZoneView(BRect frame)
	: BView(frame, "timeZoneView", B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE_JUMP),
	  fInitialized(false)
{
	ReadTimeZoneLink();
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
	BPath parent;

	fCurrentZone = fOldZone;
	int32 czone = 0;

	if (fCurrentZone.Leaf() != NULL
		&& strcmp(fCurrentZone.Leaf(), "Greenwich") == 0) {
		if (BMenuItem* item = fRegionPopUp->FindItem("Others"))
			item->SetMarked(true);
		czone = FillCityList("Others");
	} else if (fCurrentZone.GetParent(&parent) == B_OK) {
		if (BMenuItem* item = fRegionPopUp->FindItem(parent.Leaf()))
			item->SetMarked(true);
		czone = FillCityList(parent.Path());
	}

	if (czone > -1) {
		fCityList->Select(czone);
		fCityList->ScrollToSelection();
		fCurrent->SetText(((TZoneItem *)fCityList->ItemAt(czone))->Text());
		SetPreview();
	}
	SetTimeZone();
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
		fCityList->SetTarget(this);
		fRegionPopUp->SetTargetForItems(this);

		// update displays
		BPath parent;
		if (fCurrentZone.Leaf() != NULL
			&& strcmp(fCurrentZone.Leaf(), "Greenwich") != 0) {
			fCurrentZone.GetParent(&parent);
			int32 czone = FillCityList(parent.Path());
			if (czone > -1) {
				fCityList->Select(czone);
				fCurrent->SetText(((TZoneItem *)fCityList->ItemAt(czone))->Text());
			}
		} else {
			int32 czone = FillCityList("Others");
			if (czone > -1) {
				fCityList->Select(czone);
				fCurrent->SetText(((TZoneItem *)fCityList->ItemAt(czone))->Text());
			}
		}
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

		case H_REGION_CHANGED:
			ChangeRegion(message);
			break;

		case H_SET_TIME_ZONE:
		{
			SetTimeZone();
			BMessage msg(*message);
			msg.what = kRTCUpdate;
			Window()->PostMessage(&msg);
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
	// Zone menu
	fRegionPopUp = new BPopUpMenu("", true, true, B_ITEMS_IN_COLUMN);

	BuildRegionMenu();

	// left side
	BRect frameLeft(Bounds());
	frameLeft.right = frameLeft.Width() / 2.0;
	frameLeft.InsetBy(10.0f, 10.0f);

	BMenuField *menuField = new BMenuField(frameLeft, "regions", NULL, fRegionPopUp, false);
	AddChild(menuField);
	menuField->ResizeToPreferred();

	frameLeft.top = menuField->Frame().bottom + 10.0;
	frameLeft.right -= B_V_SCROLL_BAR_WIDTH;

	// City Listing
	fCityList = new BListView(frameLeft, "cityList", B_SINGLE_SELECTION_LIST);
	fCityList->SetSelectionMessage(new BMessage(H_CITY_CHANGED));
	fCityList->SetInvocationMessage(new BMessage(H_SET_TIME_ZONE));

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
	fSetZone = new BButton(frameRight, "setTimeZone", "Set Time Zone",
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
	BPath path;
	if (_GetTimezonesPath(path) != B_OK)
		return;

	// get current region
	BPath region;
	fCurrentZone.GetParent(&region);

	bool markit;
	BEntry entry;
	BMenuItem *item;
	BDirectory dir(path.Path());

	dir.Rewind();

	// walk timezones dir and add entries to menu
	BString itemtext;
	while (dir.GetNextEntry(&entry) == B_NO_ERROR) {
		if (entry.IsDirectory()) {
			path.SetTo(&entry);
			itemtext = path.Leaf();

			// skip Etc directory
			if (itemtext.Compare("Etc", 3) == 0)
				continue;

			markit = itemtext.Compare(region.Leaf()) == 0;

			// add Ocean to oceans :)
			if (itemtext.Compare("Atlantic", 8) == 0
				 ||itemtext.Compare("Pacific", 7) == 0
				 ||itemtext.Compare("Indian", 6) == 0)
				itemtext.Append(" Ocean");

			// underscores are spaces
			itemtext = itemtext.ReplaceAll('_', ' ');

			BMessage *msg = new BMessage(H_REGION_CHANGED);
			msg->AddString("region", path.Path());

			item = new BMenuItem(itemtext.String(), msg);
			item->SetMarked(markit);
			fRegionPopUp->AddItem(item);
		}
	}
	BMessage *msg = new BMessage(H_REGION_CHANGED);
	msg->AddString("region", "Others");

	item = new BMenuItem("Others", msg);
	if (fCurrentZone.Leaf() != NULL)
		item->SetMarked(strcmp(fCurrentZone.Leaf(), "Greenwich") == 0);
	fRegionPopUp->AddItem(item);
}


int32
TimeZoneView::FillCityList(const char *area)
{
	// clear list
	int32 count = fCityList->CountItems();
	if (count > 0) {
		for (int32 idx = count; idx >= 0; idx--)
			delete fCityList->RemoveItem(idx);
		fCityList->MakeEmpty();
	}

 	BStringItem *city;
	int32 index = -1;
	if (strcmp(area, "Others") != 0) {
		// Enter time zones directory. Find subdir with name that matches string
		// stored in area. Enter subdirectory and count the items. For each item,
		// add a StringItem to fCityList Time zones directory

		BPath path;
		_GetTimezonesPath(path);

		BPath areaPath(area);
	 	BDirectory zoneDir(path.Path());
	 	BDirectory cityDir;
	 	BString city_name;
	 	BEntry entry;

		// locate subdirectory:
		if (zoneDir.Contains(areaPath.Leaf(), B_DIRECTORY_NODE)) {
			cityDir.SetTo(&zoneDir, areaPath.Leaf());

			// There is a subdir with a name that matches 'area'. That's the one!!
			// Iterate over the items in the subdir, fill listview with TZoneItems
			while(cityDir.GetNextEntry(&entry) == B_NO_ERROR) {
				if (!entry.IsDirectory()) {
					BPath zone(&entry);
					city_name = zone.Leaf();
					city_name.ReplaceAll("_IN", ", Indiana");
					city_name.ReplaceAll("__Calif", ", Calif");
					city_name.ReplaceAll("__", ", ");
					city_name.ReplaceAll("_", " ");
					city = new TZoneItem(city_name.String(), zone.Path());
					fCityList->AddItem(city);
					if (fCurrentZone.Leaf() != NULL
						&& strcmp(fCurrentZone.Leaf(), zone.Leaf()) == 0)
						index = fCityList->IndexOf(city);
				}
			}
		}
	} else {
		BPath path;
		_GetTimezonesPath(path);
		path.Append("Greenwich");

		city = new TZoneItem("Greenwich", path.Path());
		fCityList->AddItem(city);
		if (fCurrentZone.Leaf() != NULL
			&& strcmp(fCurrentZone.Leaf(), "Greenwich") == 0) {
			index = fCityList->IndexOf(city);
		}
	}
	return index;
}


void
TimeZoneView::ChangeRegion(BMessage *message)
{
	BString area;
	message->FindString("region", &area);

	FillCityList(area.String());
}


void
TimeZoneView::ReadTimeZoneLink()
{
	char tzFileName[B_PATH_NAME_LENGTH];
	bool isGMT;
	_kern_get_tzfilename(tzFileName, B_PATH_NAME_LENGTH, &isGMT);

	BEntry tzLink;
	tzLink.SetTo(tzFileName);

	if (!tzLink.Exists()) {
		BPath path;
		_GetTimezonesPath(path);
		path.Append("Greenwich");

		tzLink.SetTo(path.Path());
	}

	// we need something in the current zone
	fCurrentZone.SetTo(&tzLink);
	fOldZone.SetTo(&tzLink);
}


void
TimeZoneView::SetPreview()
{
	int32 selection = fCityList->CurrentSelection();
	if (selection >= 0) {
		TZoneItem *item = (TZoneItem *)fCityList->ItemAt(selection);

		// set timezone to selection
		SetTimeZone(item->Path());

		// calc preview time
		time_t current = time(0);
		struct tm *ltime = localtime(&current);

		// update prview
		fPreview->SetText(item->Text());
		fPreview->SetTime(ltime->tm_hour, ltime->tm_min);

		// set timezone back to current
		SetTimeZone(fCurrentZone.Path());

		fSetZone->SetEnabled((strcmp(fCurrent->Text(), item->Text()) != 0));
	}
}


void
TimeZoneView::SetCurrent(const char *text)
{
	SetTimeZone(fCurrentZone.Path());

	time_t current = time(0);
	struct tm *ltime = localtime(&current);

	fCurrent->SetText(text);
	fCurrent->SetTime(ltime->tm_hour, ltime->tm_min);
}


void
TimeZoneView::SetTimeZone()
{
	/*	set time based on supplied timezone. How to do this?
		1) replace symlink "timezone" in B_USER_SETTINGS_DIR with a link to the new timezone
		2) set TZ environment var
		3) call settz()
		4) call set_timezone from OS.h passing path to timezone file
	*/

	// get path to current link
	// update/create timezone symlink in B_USER_SETTINGS_DIRECTORY
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("timezone");

	// build target for new link
	int32 selection = fCityList->CurrentSelection();
	if (selection < 0)
		return;

	BPath target(((TZoneItem *)fCityList->ItemAt(selection))->Path());

	// remove old
	BEntry entry(path.Path());
	if (entry.Exists())
		entry.Remove();

	// create new
	BDirectory dir(target.Path());
	if (dir.CreateSymLink(path.Path(), target.Path(), NULL) != B_OK)
		fprintf(stderr, "timezone not linked\n");

	// update environment
	SetTimeZone(target.Path());

	// update display
	time_t current = time(0);
	struct tm *ltime = localtime(&current);

	char tza[B_PATH_NAME_LENGTH];
	sprintf(tza, "%s", target.Path());
	set_timezone(tza);

	// disable button
	fSetZone->SetEnabled(false);

	time_t newtime = mktime(ltime);
	ltime = localtime(&newtime);
	stime(&newtime);

	fHour = ltime->tm_hour;
	fMinute = ltime->tm_min;
	fCurrentZone.SetTo(target.Path());
	SetCurrent(((TZoneItem *)fCityList->ItemAt(selection))->Text());
}


void
TimeZoneView::SetTimeZone(const char *zone)
{
	putenv(BString("TZ=").Append(zone).String());
	tzset();
}


status_t
TimeZoneView::_GetTimezonesPath(BPath& path)
{
	status_t status = find_directory(B_SYSTEM_DATA_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	return path.Append("timezones");
}

