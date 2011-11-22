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

#include <string.h>

#include <Application.h>
#include <Debug.h>
#include <Roster.h>

#include "BarApp.h"
#include "BarMenuBar.h"
#include "DeskbarUtils.h"
#include "TeamMenuItem.h"
#include "TeamMenu.h"


TTeamMenu::TTeamMenu()
	: BMenu("Team Menu")
{
	SetItemMargins(0.0f, 0.0f, 0.0f, 0.0f);
	SetFont(be_plain_font);
}


int
TTeamMenu::CompareByName(const void* first, const void* second)
{
	return strcasecmp((*(static_cast<BarTeamInfo* const*>(first)))->name,
		(*(static_cast<BarTeamInfo* const*>(second)))->name);
}


void
TTeamMenu::AttachedToWindow()
{
	RemoveItems(0, CountItems(), true);

	BMessenger self(this);
	BList teamList;
	TBarApp::Subscribe(self, &teamList);

	TBarView* barview = (dynamic_cast<TBarApp*>(be_app))->BarView();
	bool dragging = barview && barview->Dragging();

	desk_settings* settings = ((TBarApp*)be_app)->Settings();

	if (settings->sortRunningApps)
		teamList.SortItems(CompareByName);

	int32 count = teamList.CountItems();
	for (int32 i = 0; i < count; i++) {
		BarTeamInfo* barInfo = (BarTeamInfo*)teamList.ItemAt(i);

		if (((barInfo->flags & B_BACKGROUND_APP) == 0)
			&& (strcasecmp(barInfo->sig, kDeskbarSignature) != 0)) {
			TTeamMenuItem* item = new TTeamMenuItem(barInfo->teams,
				barInfo->icon, barInfo->name, barInfo->sig, -1, -1, true, true);

			if ((settings->trackerAlwaysFirst)
				&& (strcmp(barInfo->sig, kTrackerSignature) == 0))
				AddItem(item, 0);
			else
				AddItem(item);

			if (dragging && item) {
				bool canhandle = (dynamic_cast<TBarApp*>(be_app))->BarView()->
					AppCanHandleTypes(item->Signature());
				if (item->IsEnabled() != canhandle)
					item->SetEnabled(canhandle);

				BMenu* menu = item->Submenu();
				if (menu)
					menu->SetTrackingHook(barview->MenuTrackingHook,
						barview->GetTrackingHookData());
			}

			barInfo->teams = NULL;
			barInfo->icon = NULL;
			barInfo->name = NULL;
			barInfo->sig = NULL;
		}
		delete barInfo;
	}

	if (CountItems() == 0) {
		BMenuItem* item = new BMenuItem("no application running", NULL);
		item->SetEnabled(false);
		AddItem(item);
	}

	if (dragging && barview->LockLooper()) {
		SetTrackingHook(barview->MenuTrackingHook,
			barview->GetTrackingHookData());
		barview->DragStart();
		barview->UnlockLooper();
	}

	BMenu::AttachedToWindow();
}


void
TTeamMenu::DetachedFromWindow()
{
	TBarView* barView = (dynamic_cast<TBarApp*>(be_app))->BarView();
	if (barView) {
		BLooper* looper = barView->Looper();
		if (looper->Lock()) {
			barView->DragStop();
			looper->Unlock();
		}
	}

	BMenu::DetachedFromWindow();

	BMessenger self(this);
	TBarApp::Unsubscribe(self);
}


void
TTeamMenu::DrawBackground(BRect)
{
}

