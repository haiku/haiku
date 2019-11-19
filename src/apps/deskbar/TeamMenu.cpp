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


#include "TeamMenu.h"

#include <algorithm>
#include <strings.h>

#include <Application.h>
#include <Collator.h>
#include <Debug.h>
#include <Mime.h>
#include <Roster.h>

#include "BarApp.h"
#include "BarMenuBar.h"
#include "BarView.h"
#include "DeskbarUtils.h"
#include "StatusView.h"
#include "TeamMenuItem.h"


const float kIconPadding = 8.0f;


//	#pragma mark - TTeamMenuItem


TTeamMenu::TTeamMenu(TBarView* barView)
	:
	BMenu("Team Menu"),
	fBarView(barView)
{
	SetItemMargins(0.0f, 0.0f, 0.0f, 0.0f);
	SetFont(be_plain_font);
}


int
TTeamMenu::CompareByName(const void* first, const void* second)
{
	BCollator collator;
	BLocale::Default()->GetCollator(&collator);

	return collator.Compare(
		(*(static_cast<BarTeamInfo* const*>(first)))->name,
		(*(static_cast<BarTeamInfo* const*>(second)))->name);
}


void
TTeamMenu::AttachedToWindow()
{
	RemoveItems(0, CountItems(), true);
		// remove all items

	BMessenger self(this);
	BList teamList;
	TBarApp::Subscribe(self, &teamList);

	bool dragging = fBarView != NULL && fBarView->Dragging();
	desk_settings* settings = static_cast<TBarApp*>(be_app)->Settings();
	int32 iconSize = static_cast<TBarApp*>(be_app)->IconSize();
	float iconOnlyWidth = iconSize + kIconPadding;

	// calculate the minimum item width based on font and icon size
	float minItemWidth = 0;
	if (settings->hideLabels) {
		minItemWidth = std::max(floorf(gMinimumWindowWidth / 2),
			iconOnlyWidth);
	} else {
		float labelWidth = gMinimumWindowWidth - iconOnlyWidth
			+ (be_plain_font->Size() - 12) * 4;
		if (iconSize <= B_LARGE_ICON) // label wraps after 32x32
			labelWidth += iconSize - kMinimumIconSize;
		minItemWidth = iconOnlyWidth + labelWidth;
	}

	float maxItemWidth = minItemWidth;

	int32 itemCount = teamList.CountItems();
	if (!settings->hideLabels) {
		// go through list and find the widest label
		for (int32 i = 0; i < itemCount; i++) {
			BarTeamInfo* barInfo = (BarTeamInfo*)teamList.ItemAt(i);
			float labelWidth = StringWidth(barInfo->name);
			// label wraps after 32x32
			float itemWidth = iconSize > B_LARGE_ICON
				? std::max(labelWidth, iconOnlyWidth)
				: labelWidth + iconOnlyWidth + kMinimumIconSize
					+ (be_plain_font->Size() - 12) * 4;
			maxItemWidth = std::max(maxItemWidth, itemWidth);
		}

		// but not too wide
		maxItemWidth = std::min(maxItemWidth, gMaximumWindowWidth);
	}

	SetMaxContentWidth(maxItemWidth);

	if (settings->sortRunningApps)
		teamList.SortItems(TTeamMenu::CompareByName);

	// go through list and add the items
	for (int32 i = 0; i < itemCount; i++) {
		// add items back
		BarTeamInfo* barInfo = (BarTeamInfo*)teamList.ItemAt(i);
		TTeamMenuItem* item = new TTeamMenuItem(barInfo->teams,
			barInfo->icon, barInfo->name, barInfo->sig, maxItemWidth);

		if (settings->trackerAlwaysFirst
			&& strcasecmp(barInfo->sig, kTrackerSignature) == 0) {
			AddItem(item, 0);
		} else
			AddItem(item);

		if (dragging && item != NULL) {
			bool canhandle = fBarView->AppCanHandleTypes(item->Signature());
			if (item->IsEnabled() != canhandle)
				item->SetEnabled(canhandle);

			BMenu* menu = item->Submenu();
			if (menu != NULL) {
				menu->SetTrackingHook(fBarView->MenuTrackingHook,
					fBarView->GetTrackingHookData());
			}
		}
	}

	if (CountItems() == 0) {
		BMenuItem* item = new BMenuItem("no application running", NULL);
		item->SetEnabled(false);
		AddItem(item);
	}

	if (dragging && fBarView->LockLooper()) {
		SetTrackingHook(fBarView->MenuTrackingHook,
			fBarView->GetTrackingHookData());
		fBarView->DragStart();
		fBarView->UnlockLooper();
	}

	BMenu::AttachedToWindow();
}


void
TTeamMenu::DetachedFromWindow()
{
	if (fBarView != NULL) {
		BLooper* looper = fBarView->Looper();
		if (looper != NULL && looper->Lock()) {
			fBarView->DragStop();
			looper->Unlock();
		}
	}

	BMenu::DetachedFromWindow();

	BMessenger self(this);
	TBarApp::Unsubscribe(self);
}
