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

#include <Debug.h>

#include <string.h>

#include <Autolock.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <Screen.h>

#include "icons.h"
#include "icons_logo.h"
#include "BarApp.h"
#include "BarMenuTitle.h"
#include "BarView.h"
#include "DeskbarMenu.h"
#include "DeskbarUtils.h"
#include "ExpandoMenuBar.h"
#include "ResourceSet.h"
#include "ShowHideMenuItem.h"
#include "StatusView.h"
#include "TeamMenuItem.h"
#include "WindowMenu.h"
#include "WindowMenuItem.h"

const float kDefaultDeskbarMenuWidth = 50.0f;
const float kSepItemWidth = 5.0f;

const uint32 kMinimizeTeam = 'mntm';
const uint32 kBringTeamToFront = 'bftm';


bool TExpandoMenuBar::sDoMonitor = false;
thread_id TExpandoMenuBar::sMonThread = B_ERROR;
BLocker TExpandoMenuBar::sMonLocker("expando monitor");


TExpandoMenuBar::TExpandoMenuBar(TBarView* bar, BRect frame, const char* name,
	bool vertical, bool drawLabel)
	:
	BMenuBar(frame, name, B_FOLLOW_NONE,
		vertical ? B_ITEMS_IN_COLUMN : B_ITEMS_IN_ROW, vertical),
	fVertical(vertical),
	fOverflow(false),
	fDrawLabel(drawLabel),
	fIsScrolling(false),
	fShowTeamExpander(static_cast<TBarApp*>(be_app)->Settings()->superExpando),
	fExpandNewTeams(static_cast<TBarApp*>(be_app)->Settings()->expandNewTeams),
	fDeskbarMenuWidth(kDefaultDeskbarMenuWidth),
	fBarView(bar),
	fFirstApp(0),
	fPreviousDragTargetItem(NULL),
	fLastClickItem(NULL)
{
	SetItemMargins(0.0f, 0.0f, 0.0f, 0.0f);
	SetFont(be_plain_font);
	SetMaxContentWidth(sMinimumWindowWidth);
}


int
TExpandoMenuBar::CompareByName(const void* first, const void* second)
{
	return strcasecmp((*(static_cast<BarTeamInfo* const*>(first)))->name,
		(*(static_cast<BarTeamInfo* const*>(second)))->name);
}


void
TExpandoMenuBar::AttachedToWindow()
{
	BMessenger self(this);
	BList teamList;
	TBarApp::Subscribe(self, &teamList);
	float width = fVertical ? Frame().Width() : sMinimumWindowWidth;
	float height = -1.0f;

	// top or bottom mode, add deskbar menu and sep for menubar tracking
	// consistency
	if (!fVertical) {
		TDeskbarMenu* beMenu = new TDeskbarMenu(fBarView);
		TBarWindow::SetDeskbarMenu(beMenu);
		const BBitmap* logoBitmap = AppResSet()->FindBitmap(B_MESSAGE_TYPE,
			R_LeafLogoBitmap);
		if (logoBitmap != NULL)
			fDeskbarMenuWidth = logoBitmap->Bounds().Width() + 16;
		fDeskbarMenuItem = new TBarMenuTitle(fDeskbarMenuWidth, Frame().Height(),
			logoBitmap, beMenu, true);
		AddItem(fDeskbarMenuItem);

		fSeparatorItem = new TTeamMenuItem(kSepItemWidth, height, fVertical);
		AddItem(fSeparatorItem);
		fSeparatorItem->SetEnabled(false);
		fFirstApp = 2;
	} else {
		fDeskbarMenuItem = NULL;
		fSeparatorItem = NULL;
	}

	desk_settings* settings = ((TBarApp*)be_app)->Settings();

	if (settings->sortRunningApps)
		teamList.SortItems(CompareByName);

	int32 count = teamList.CountItems();
	for (int32 i = 0; i < count; i++) {
		BarTeamInfo* barInfo = (BarTeamInfo*)teamList.ItemAt(i);
		if ((barInfo->flags & B_BACKGROUND_APP) == 0
			&& strcasecmp(barInfo->sig, kDeskbarSignature) != 0) {
			if (settings->trackerAlwaysFirst
				&& !strcmp(barInfo->sig, kTrackerSignature)) {
				AddItem(new TTeamMenuItem(barInfo->teams, barInfo->icon,
					barInfo->name, barInfo->sig, width, height,
					fDrawLabel, fVertical), fFirstApp);
			} else {
				AddItem(new TTeamMenuItem(barInfo->teams, barInfo->icon,
					barInfo->name, barInfo->sig, width, height,
					fDrawLabel, fVertical));
			}

			barInfo->teams = NULL;
			barInfo->icon = NULL;
			barInfo->name = NULL;
			barInfo->sig = NULL;
		}

		delete barInfo;
	}

	BMenuBar::AttachedToWindow();

	if (CountItems() == 0) {
		// If we're empty, BMenuBar::AttachedToWindow() resizes us to some
		// weird value - we just override it again
		ResizeTo(width, 0);
	}

	if (fVertical) {
		sDoMonitor = true;
		sMonThread = spawn_thread(monitor_team_windows,
			"Expando Window Watcher", B_LOW_PRIORITY, this);
		resume_thread(sMonThread);
	}
}


void
TExpandoMenuBar::DetachedFromWindow()
{
	BMenuBar::DetachedFromWindow();

	if (sMonThread != B_ERROR) {
		sDoMonitor = false;

		status_t returnCode;
		wait_for_thread(sMonThread, &returnCode);

		sMonThread = B_ERROR;
	}

	BMessenger self(this);
	BMessage message(kUnsubscribe);
	message.AddMessenger("messenger", self);
	be_app->PostMessage(&message);

	RemoveItems(0, CountItems(), true);
}


void
TExpandoMenuBar::MessageReceived(BMessage* message)
{
	int32 index;
	TTeamMenuItem* item;

	switch (message->what) {
		case B_SOME_APP_LAUNCHED: {
			BList* teams = NULL;
			message->FindPointer("teams", (void**)&teams);

			BBitmap* icon = NULL;
			message->FindPointer("icon", (void**)&icon);

			const char* signature;
			if (message->FindString("sig", &signature) == B_OK
				&&strcasecmp(signature, kDeskbarSignature) == 0) {
				delete teams;
				delete icon;
				break;
			}

			uint32 flags;
			if (message->FindInt32("flags", ((int32*) &flags)) == B_OK
				&& (flags & B_BACKGROUND_APP) != 0) {
				delete teams;
				delete icon;
				break;
			}

			const char* name = NULL;
			message->FindString("name", &name);

			AddTeam(teams, icon, strdup(name), strdup(signature));
			break;
		}

		case kAddTeam:
			AddTeam(message->FindInt32("team"), message->FindString("sig"));
			break;

		case kRemoveTeam:
		{
			team_id team = -1;
			message->FindInt32("team", &team);

			RemoveTeam(team, true);
			break;
		}

		case B_SOME_APP_QUIT:
		{
			team_id team = -1;
			message->FindInt32("team", &team);

			RemoveTeam(team, false);
			break;
		}

		case kMinimizeTeam:
		{
			index = message->FindInt32("itemIndex");
			item = dynamic_cast<TTeamMenuItem*>(ItemAt(index));
			if (item == NULL)
				break;

			TShowHideMenuItem::TeamShowHideCommon(B_MINIMIZE_WINDOW,
				item->Teams(),
				item->Menu()->ConvertToScreen(item->Frame()),
				true);
			break;
		}

		case kBringTeamToFront:
		{
			index = message->FindInt32("itemIndex");
			item = dynamic_cast<TTeamMenuItem*>(ItemAt(index));
			if (item == NULL)
				break;

			TShowHideMenuItem::TeamShowHideCommon(B_BRING_TO_FRONT,
				item->Teams(), item->Menu()->ConvertToScreen(item->Frame()),
				true);
			break;
		}

		default:
			BMenuBar::MessageReceived(message);
			break;
	}
}


void
TExpandoMenuBar::MouseDown(BPoint where)
{
	BMessage* message = Window()->CurrentMessage();
	BMenuItem* menuItem;
	TTeamMenuItem* item = TeamItemAtPoint(where, &menuItem);

	// check for three finger salute, a.k.a. Vulcan Death Grip
	if (message != NULL && item != NULL && !fBarView->Dragging()) {
		int32 modifiers = 0;
		message->FindInt32("modifiers", &modifiers);

		if ((modifiers & B_COMMAND_KEY) != 0
			&& (modifiers & B_CONTROL_KEY) != 0
			&& (modifiers & B_SHIFT_KEY) != 0) {
			const BList* teams = item->Teams();
			int32 teamCount = teams->CountItems();

			team_id teamID;
			for (int32 team = 0; team < teamCount; team++) {
				teamID = (team_id)teams->ItemAt(team);
				kill_team(teamID);
				// remove the team immediately from display
				RemoveTeam(teamID, false);
			}

			return;
		}

		// control click - show all/hide all shortcut
		if ((modifiers & B_CONTROL_KEY) != 0) {
			// show/hide item's teams
			BMessage showMessage((modifiers & B_SHIFT_KEY) != 0
				? kMinimizeTeam : kBringTeamToFront);
			showMessage.AddInt32("itemIndex", IndexOf(item));
			Window()->PostMessage(&showMessage, this);
			return;
		}

		// Check the bounds of the expand Team icon
		if (fShowTeamExpander && fVertical) {
			BRect expanderRect = item->ExpanderBounds();
			if (expanderRect.Contains(where)) {
				// Let the update thread wait...
				BAutolock locker(sMonLocker);

				// Toggle the item
				item->ToggleExpandState(true);
				item->Draw();

				// Absorb the message.
				return;
			}
		}

		// double-click on an item brings the team to front
		int32 clicks;
		if (message->FindInt32("clicks", &clicks) == B_OK && clicks > 1
			&& item == menuItem && item == fLastClickItem) {
			// activate this team
			be_roster->ActivateApp((team_id)item->Teams()->ItemAt(0));
			return;
		}

		fLastClickItem = item;
	}

	BMenuBar::MouseDown(where);
}


void
TExpandoMenuBar::MouseMoved(BPoint where, uint32 code, const BMessage* message)
{
	if (!message) {
		// force a cleanup
		_FinishedDrag();
		BMenuBar::MouseMoved(where, code, message);
		return;
	}

	uint32 buttons;
	if (!(Window()->CurrentMessage())
		|| Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons)
		< B_OK)
		buttons = 0;

	if (buttons == 0)
		return;

	switch (code) {
		case B_ENTERED_VIEW:
			// fPreviousDragTargetItem should always be NULL here anyways.
			if (fPreviousDragTargetItem)
				_FinishedDrag();

			fBarView->CacheDragData(message);
			fPreviousDragTargetItem = NULL;
			break;

		case B_OUTSIDE_VIEW:
			// NOTE: Should not be here, but for the sake of defensive
			// programming...
		case B_EXITED_VIEW:
			_FinishedDrag();
			break;

		case B_INSIDE_VIEW:
			if (fBarView->Dragging()) {
				TTeamMenuItem* item = NULL;
				for (int32 i = 0; i < CountItems(); i++) {
					BMenuItem* _item = ItemAt(i);
					if (_item->Frame().Contains(where)) {
						item = dynamic_cast<TTeamMenuItem*>(_item);
						break;
					}
				}
				if (item == fPreviousDragTargetItem)
					break;
				if (fPreviousDragTargetItem != NULL)
					fPreviousDragTargetItem->SetOverrideSelected(false);
				if (item != NULL)
					item->SetOverrideSelected(true);
				fPreviousDragTargetItem = item;
			}
			break;
	}
}


void
TExpandoMenuBar::MouseUp(BPoint where)
{
	if (!fBarView->Dragging()) {
		BMenuBar::MouseUp(where);
		return;
	}

	_FinishedDrag(true);
}


bool
TExpandoMenuBar::InDeskbarMenu(BPoint loc) const
{
	if (!fVertical) {
		if (fDeskbarMenuItem && fDeskbarMenuItem->Frame().Contains(loc))
			return true;
	} else {
		TBarWindow* window = dynamic_cast<TBarWindow*>(Window());
		if (window) {
			if (TDeskbarMenu* bemenu = window->DeskbarMenu()) {
				bool inDeskbarMenu = false;
				if (bemenu->LockLooper()) {
					inDeskbarMenu = bemenu->Frame().Contains(loc);
					bemenu->UnlockLooper();
				}
				return inDeskbarMenu;
			}
		}
	}

	return false;
}


/*!	Returns the team menu item that belongs to the item under the
	specified \a point.
	If \a _item is given, it will return the exact menu item under
	that point (which might be a window item when the expander is on).
*/
TTeamMenuItem*
TExpandoMenuBar::TeamItemAtPoint(BPoint point, BMenuItem** _item)
{
	TTeamMenuItem* lastApp = NULL;
	int32 count = CountItems();

	for (int32 i = fFirstApp; i < count; i++) {
		BMenuItem* item = ItemAt(i);

		if (dynamic_cast<TTeamMenuItem*>(item) != NULL)
			lastApp = (TTeamMenuItem*)item;

		if (item && item->Frame().Contains(point)) {
			if (_item != NULL)
				*_item = item;

			return lastApp;
		}
	}

	// no item found

	if (_item != NULL)
		*_item = NULL;

	return NULL;
}


void
TExpandoMenuBar::AddTeam(BList* team, BBitmap* icon, char* name,
	char* signature)
{
	float itemWidth = fVertical ? fBarView->Bounds().Width()
		: sMinimumWindowWidth;
	float itemHeight = -1.0f;

	desk_settings* settings = ((TBarApp*)be_app)->Settings();
	TTeamMenuItem* item = new TTeamMenuItem(team, icon, name, signature,
		itemWidth, itemHeight, fDrawLabel, fVertical);

	if (settings->trackerAlwaysFirst && !strcmp(signature, kTrackerSignature)) {
		AddItem(item, fFirstApp);
	} else if (settings->sortRunningApps) {
		TTeamMenuItem* teamItem
			= dynamic_cast<TTeamMenuItem*>(ItemAt(fFirstApp));
		int32 firstApp = fFirstApp;

		// if Tracker should always be the first item, we need to skip it
		// when sorting in the current item
		if (settings->trackerAlwaysFirst && teamItem != NULL
			&& !strcmp(teamItem->Signature(), kTrackerSignature)) {
			firstApp++;
		}

		int32 count = CountItems(), i;
		for (i = firstApp; i < count; i++) {
			teamItem = dynamic_cast<TTeamMenuItem*>(ItemAt(i));
			if (teamItem != NULL && strcasecmp(teamItem->Name(), name) > 0) {
				AddItem(item, i);
				break;
			}
		}
		// was the item added to the list yet?
		if (i == count)
			AddItem(item);
	} else
		AddItem(item);

	if (fVertical) {
		if (item && fShowTeamExpander && fExpandNewTeams)
			item->ToggleExpandState(false);

		fBarView->SizeWindow(BScreen(Window()).Frame());
	} else
		CheckItemSizes(1);

	Window()->UpdateIfNeeded();
}


void
TExpandoMenuBar::AddTeam(team_id team, const char* signature)
{
	int32 count = CountItems();
	for (int32 i = fFirstApp; i < count; i++) {
		// Only add to team menu items
		if (TTeamMenuItem* item = dynamic_cast<TTeamMenuItem*>(ItemAt(i))) {
			if (strcasecmp(item->Signature(), signature) == 0) {
				if (!(item->Teams()->HasItem((void*)team)))
					item->Teams()->AddItem((void*)team);

				break;
			}
		}
	}
}


void
TExpandoMenuBar::RemoveTeam(team_id team, bool partial)
{
	int32 count = CountItems();
	for (int32 i = fFirstApp; i < count; i++) {
		if (TTeamMenuItem* item = dynamic_cast<TTeamMenuItem*>(ItemAt(i))) {
			if (item->Teams()->HasItem((void*)team)) {
				item->Teams()->RemoveItem(team);

				if (partial)
					return;

#ifdef DOUBLECLICKBRINGSTOFRONT
				if (fLastClickItem == i)
					fLastClickItem = -1;
#endif

				RemoveItem(i);

				if (fVertical) {
					// instead of resizing the window here and there in the
					// code the resize method will be centered in one place
					// thus, the same behavior (good or bad) will be used
					// whereever window sizing is done
					fBarView->SizeWindow(BScreen(Window()).Frame());
				} else
					CheckItemSizes(-1);

				Window()->UpdateIfNeeded();

				delete item;
				return;
			}
		}
	}
}


void
TExpandoMenuBar::CheckItemSizes(int32 delta)
{
	float width = Frame().Width();
	int32 count = CountItems();
	bool reset = false;
	float newWidth = 0;
	float fullWidth = (sMinimumWindowWidth * count);

	if (!fBarView->Vertical()) {
		// in this case there are 2 extra items:
		//   - The Be Menu
		//   - The little separator item
		fullWidth = fullWidth - (sMinimumWindowWidth * 2)
			+ (fDeskbarMenuWidth + kSepItemWidth);
		width -= (fDeskbarMenuWidth + kSepItemWidth);
		count -= 2;
	}

	if (delta >= 0 && fullWidth > width) {
		fOverflow = true;
		reset = true;
		newWidth = floorf(width / count);
	} else if (delta < 0 && fOverflow) {
		reset = true;
		if (fullWidth > width)
			newWidth = floorf(width / count);
		else
			newWidth = sMinimumWindowWidth;
	}
	if (newWidth > sMinimumWindowWidth)
		newWidth = sMinimumWindowWidth;

	if (reset) {
		SetMaxContentWidth(newWidth);
		if (newWidth == sMinimumWindowWidth)
			fOverflow = false;
		InvalidateLayout();

		for (int32 index = fFirstApp; ; index++) {
			TTeamMenuItem* item = (TTeamMenuItem*)ItemAt(index);
			if (!item)
				break;
			item->SetOverrideWidth(newWidth);
		}

		Invalidate();
		Window()->UpdateIfNeeded();
	}
}


menu_layout
TExpandoMenuBar::MenuLayout() const
{
	return Layout();
}


void
TExpandoMenuBar::Draw(BRect update)
{
	BMenu::Draw(update);
}


void
TExpandoMenuBar::DrawBackground(BRect)
{
	if (fVertical)
		return;

	BRect bounds(Bounds());
	rgb_color menuColor = ViewColor();
	rgb_color hilite = tint_color(menuColor, B_DARKEN_1_TINT);
	rgb_color vlight = tint_color(menuColor, B_LIGHTEN_2_TINT);

	int32 last = CountItems() - 1;
	if (last >= 0)
		bounds.left = ItemAt(last)->Frame().right + 1;
	else
		bounds.left = 0;

	if (be_control_look != NULL) {
		SetHighColor(tint_color(menuColor, 1.22));
		StrokeLine(bounds.LeftTop(), bounds.LeftBottom());
		bounds.left++;
		uint32 borders = BControlLook::B_TOP_BORDER
			| BControlLook::B_BOTTOM_BORDER | BControlLook::B_RIGHT_BORDER;

		be_control_look->DrawButtonBackground(this, bounds, bounds, menuColor,
			0, borders);
	} else {
		SetHighColor(vlight);
		StrokeLine(bounds.LeftTop(), bounds.RightTop());
		StrokeLine(BPoint(bounds.left, bounds.top + 1), bounds.LeftBottom());
		SetHighColor(hilite);
		StrokeLine(BPoint(bounds.left + 1, bounds.bottom),
			bounds.RightBottom());
	}
}


/*!	Something to help determine if we are showing too many apps
	need to add in scrolling functionality.
*/
void
TExpandoMenuBar::CheckForSizeOverrun()
{
	BRect screenFrame = (BScreen(Window())).Frame();

	fIsScrolling = fVertical ? Window()->Frame().bottom > screenFrame.bottom
		: false;
}


void
TExpandoMenuBar::SizeWindow()
{
	if (fVertical)
		fBarView->SizeWindow(BScreen(Window()).Frame());
	else
		CheckItemSizes(1);
}


int32
TExpandoMenuBar::monitor_team_windows(void* arg)
{
	TExpandoMenuBar* teamMenu = (TExpandoMenuBar*)arg;

	while (teamMenu->sDoMonitor) {
		sMonLocker.Lock();

		if (teamMenu->Window()->LockWithTimeout(50000) == B_OK) {
			int32 totalItems = teamMenu->CountItems();

			// Set all WindowMenuItems to require an update.
			TWindowMenuItem* item = NULL;
			for (int32 i = 0; i < totalItems; i++) {
				if (!teamMenu->SubmenuAt(i)) {
					item = static_cast<TWindowMenuItem*>(teamMenu->ItemAt(i));
					item->SetRequireUpdate();
				}
			}

			// Perform SetTo() on all the items that still exist as well as add
			// new items.
			bool itemModified = false, resize = false;
			TTeamMenuItem* teamItem = NULL;

			for (int32 i = 0; i < totalItems; i++) {
				if (teamMenu->SubmenuAt(i) == NULL)
					continue;

				teamItem = static_cast<TTeamMenuItem*>(teamMenu->ItemAt(i));
				if (teamItem->IsExpanded()) {
					int32 teamCount = teamItem->Teams()->CountItems();
					for (int32 j = 0; j < teamCount; j++) {
						// The following code is almost a copy/paste from
						// WindowMenu.cpp
						team_id	theTeam = (team_id)teamItem->Teams()->ItemAt(j);
						int32 count = 0;
						int32* tokens = get_token_list(theTeam, &count);

						for (int32 k = 0; k < count; k++) {
							client_window_info* wInfo
								= get_window_info(tokens[k]);
							if (wInfo == NULL)
								continue;

							if (TWindowMenu::WindowShouldBeListed(wInfo->feel)
								&& (wInfo->show_hide_level <= 0
									|| wInfo->is_mini)) {
								// Check if we have a matching window item...
								item = teamItem->ExpandedWindowItem(
									wInfo->server_token);
								if (item) {
									item->SetTo(wInfo->name,
										wInfo->server_token, wInfo->is_mini,
										((1 << current_workspace())
											& wInfo->workspaces) != 0);

									if (strcasecmp(wInfo->name,
										item->FullTitle()) != 0)
										item->SetLabel(wInfo->name);

									if (item->ChangedState())
										itemModified = true;
								} else if (teamItem->IsExpanded()) {
									// Add the item
									item = new TWindowMenuItem(wInfo->name,
										wInfo->server_token, wInfo->is_mini,
										((1 << current_workspace())
											& wInfo->workspaces) != 0, false);
									item->ExpandedItem(true);
									teamMenu->AddItem(item,
										TWindowMenuItem::InsertIndexFor(
											teamMenu, i + 1, item));
									resize = true;
								}
							}
							free(wInfo);
						}
						free(tokens);
					}
				}
			}

			// Remove any remaining items which require an update.
			for (int32 i = 0; i < totalItems; i++) {
				if (!teamMenu->SubmenuAt(i)) {
					item = static_cast<TWindowMenuItem*>(teamMenu->ItemAt(i));
					if (item && item->RequiresUpdate()) {
						item = static_cast<TWindowMenuItem*>
							(teamMenu->RemoveItem(i));
						delete item;
						totalItems--;

						resize = true;
					}
				}
			}

			// If any of the WindowMenuItems changed state, we need to force a
			// repaint.
			if (itemModified || resize) {
				teamMenu->Invalidate();
				if (resize)
					teamMenu->SizeWindow();
			}

			teamMenu->Window()->Unlock();
		}

		sMonLocker.Unlock();

		// sleep for a bit...
		snooze(150000);
	}
	return B_OK;
}


void
TExpandoMenuBar::_FinishedDrag(bool invoke)
{
	if (fPreviousDragTargetItem != NULL) {
		if (invoke)
			fPreviousDragTargetItem->Invoke();
		fPreviousDragTargetItem->SetOverrideSelected(false);
		fPreviousDragTargetItem = NULL;
	}
	if (!invoke && fBarView->Dragging())
		fBarView->DragStop(true);
}

