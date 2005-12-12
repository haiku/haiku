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

#include <Debug.h>
#include <string.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <Screen.h>
#include <Bitmap.h>
#include <Autolock.h>

#include "icons.h"
#include "icons_logo.h"
#include "BarApp.h"
#include "BarMenuTitle.h"
#include "BarView.h"
#include "BeMenu.h"
#include "DeskBarUtils.h"
#include "ExpandoMenuBar.h"
#include "ResourceSet.h"
#include "ShowHideMenuItem.h"
#include "StatusView.h"
#include "TeamMenuItem.h"
#include "WindowMenu.h"
#include "WindowMenuItem.h"

const float kBeMenuWidth = 50.0f;
const float kSepItemWidth = 5.0f;

const uint32 M_MINIMIZE_TEAM = 'mntm';
const uint32 M_BRING_TEAM_TO_FRONT = 'bftm';


bool TExpandoMenuBar::sDoMonitor = false;
thread_id TExpandoMenuBar::sMonThread = B_ERROR;
BLocker TExpandoMenuBar::sMonLocker("expando monitor");


TExpandoMenuBar::TExpandoMenuBar(TBarView *bar, BRect frame, const char *name,
	bool vertical, bool drawLabel)
	: BMenuBar(frame, name, B_FOLLOW_NONE,
			vertical ? B_ITEMS_IN_COLUMN : B_ITEMS_IN_ROW, vertical),
	fVertical(vertical),
	fOverflow(false),
	fDrawLabel(drawLabel),
	fIsScrolling(false),
	fShowTeamExpander(static_cast<TBarApp *>(be_app)->Settings()->superExpando),
	fExpandNewTeams(static_cast<TBarApp *>(be_app)->Settings()->expandNewTeams),
	fBarView(bar),
	fFirstApp(0)
{
#ifdef DOUBLECLICKBRINGSTOFRONT
	fLastClickItem = -1;
	fLastClickTime = 0;
#endif

	SetItemMargins(0.0f, 0.0f, 0.0f, 0.0f);	
	SetFont(be_plain_font);
	SetMaxContentWidth(kMinimumWindowWidth);
}


int
TExpandoMenuBar::CompareByName( const void *first, const void *second)
{
	return strcasecmp(
		(*(static_cast<BarTeamInfo * const*>(first )))->name,
		(*(static_cast<BarTeamInfo * const*>(second)))->name);
}


void
TExpandoMenuBar::AttachedToWindow()
{
	BMessenger self(this);
	BList teamList;
	TBarApp::Subscribe(self, &teamList);
	float width = fVertical ? Frame().Width() : kMinimumWindowWidth;
	float height = -1.0f;

	// top or bottom mode, add be menu and sep for menubar tracking consistency
	if (!fVertical) {	
		TBeMenu *beMenu = new TBeMenu(fBarView);
		TBarWindow::SetBeMenu(beMenu);
 		fBeMenuItem = new TBarMenuTitle(kBeMenuWidth, Frame().Height(),
 			AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_BeLogoIcon), beMenu, true);
		AddItem(fBeMenuItem);
		
		fSeparatorItem = new TTeamMenuItem(kSepItemWidth, height, fVertical);
		AddItem(fSeparatorItem);
		fSeparatorItem->SetEnabled(false);
		fFirstApp = 2;
	} else {
		fBeMenuItem = NULL;
		fSeparatorItem = NULL;
	}

	desk_settings *settings = ((TBarApp *)be_app)->Settings();
	
	if (settings->sortRunningApps)
		teamList.SortItems(CompareByName);

	int32 count = teamList.CountItems();
	for (int32 i = 0; i < count; i++) {
		BarTeamInfo *barInfo = (BarTeamInfo *)teamList.ItemAt(i);
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
	BMessage message(msg_Unsubscribe);
	message.AddMessenger("messenger", self);
	be_app->PostMessage(&message);

	BMenuItem *item = NULL;
	while ((item = RemoveItem(0L)) != NULL)
		delete item;
}


void
TExpandoMenuBar::MessageReceived(BMessage *message)
{
	int32 index;
	TTeamMenuItem *item;

	switch (message->what) {
		case B_SOME_APP_LAUNCHED: {
			BList *teams = NULL;
			message->FindPointer("teams", (void **)&teams);

			BBitmap *icon = NULL;
			message->FindPointer("icon", (void **)&icon);

			const char *signature;
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

			const char *name = NULL;
			message->FindString("name", &name);

			AddTeam(teams, icon, strdup(name), strdup(signature));
			break;
		}

		case msg_AddTeam:
			AddTeam(message->FindInt32("team"), message->FindString("sig"));
			break;

		case msg_RemoveTeam:
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

		case M_MINIMIZE_TEAM:
		{
			index = message->FindInt32("itemIndex");
			item = dynamic_cast<TTeamMenuItem *>(ItemAt(index));
			if (item == NULL)
				break;

			TShowHideMenuItem::TeamShowHideCommon(B_MINIMIZE_WINDOW,
				item->Teams(), 
				item->Menu()->ConvertToScreen(item->Frame()), 
				true);
			break;
		}

		case M_BRING_TEAM_TO_FRONT:
		{
			index = message->FindInt32("itemIndex");
			item = dynamic_cast<TTeamMenuItem *>(ItemAt(index));
			if (item == NULL)
				break;

			TShowHideMenuItem::TeamShowHideCommon(B_BRING_TO_FRONT,
				item->Teams(), 
				item->Menu()->ConvertToScreen(item->Frame()),
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
	BMessage *message = Window()->CurrentMessage();

	// check for three finger salute, a.k.a. Vulcan Death Grip
	if (message != NULL) {
		int32 modifiers = 0;
		message->FindInt32("modifiers", &modifiers);

		if ((modifiers & B_COMMAND_KEY) != 0
			&& (modifiers & B_OPTION_KEY) != 0
			&& (modifiers & B_SHIFT_KEY) != 0
			&& !fBarView->Dragging()) {

			TTeamMenuItem *item = ItemAtPoint(where);
			if (item) {
				const BList	*teams = item->Teams();
				int32 teamCount = teams->CountItems();

				team_id teamID;
				for (int32 team = 0; team < teamCount; team++) {
					teamID = (team_id)teams->ItemAt(team);
					kill_team(teamID);
					//	remove the team immediately
					//	from display
					RemoveTeam(teamID, false);
				}

				return;
			}		
		}
	}

	const int32 count = CountItems();

// This feature is broken because the menu bar never receives
// the second click
#ifdef DOUBLECLICKBRINGSTOFRONT
	// doubleclick on an item brings all to front
	for (int32 i = fFirstApp; i < count; i++) {
		TTeamMenuItem *item = (TTeamMenuItem *)ItemAt(i);
		if (item->Frame().Contains(where)) {
			bigtime_t clickSpeed = 0;
			get_click_speed(&clickSpeed);
			if ( (fLastClickItem == i) && 
				 (clickSpeed > (system_time() - fLastClickTime)) ) {
				// bring this team's window to the front
				BMessage showMessage(M_BRING_TEAM_TO_FRONT);
				showMessage.AddInt32("itemIndex", i);
				Window()->PostMessage(&showMessage, this);
				return;
			}

			fLastClickItem = i;
			fLastClickTime = system_time();
			break;
		}
	}
#endif

	// control click - show all/hide all shortcut
	if (message != NULL) {
		int32 modifiers = 0;
		message->FindInt32("modifiers", &modifiers);
		if ((modifiers & B_CONTROL_KEY) != 0
			&& ! fBarView->Dragging()) {
			int32 lastApp = -1;

			// find the clicked item
			for (int32 i = fFirstApp; i < count; i++) {
				const TTeamMenuItem *item = (TTeamMenuItem *)ItemAt(i);

				// check if this item is really a team item	(what a cruel way...)
				// "lastApp" will always point to the last application in
				// the list - the other entries might be windows (due to the team expander)
				if (item->Submenu())
					 lastApp = i;

				if (item->Frame().Contains(where)) {
					// show/hide item's teams
					BMessage showMessage((modifiers & B_SHIFT_KEY) != 0
						? M_MINIMIZE_TEAM : M_BRING_TEAM_TO_FRONT);
					showMessage.AddInt32("itemIndex", lastApp);
					Window()->PostMessage(&showMessage, this);
					return;
				}
			}
		}
	}

	// Check the bounds of the expand Team icon
	if (fShowTeamExpander && fVertical && !fBarView->Dragging()) {
		TTeamMenuItem *item = ItemAtPoint(where);
		if (item->Submenu()) {
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
	}

	BMenuBar::MouseDown(where);
}


void
TExpandoMenuBar::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{		
	if (!message) {
		//	force a cleanup
		fBarView->DragStop(true);
		BMenuBar::MouseMoved(where, code, message);
		return;
	}

	BPoint loc;
	uint32 buttons;
	GetMouse(&loc, &buttons);

	switch (code) {
		case B_ENTERED_VIEW:
			if (message && buttons != 0) {
				fBarView->CacheDragData((BMessage *)message);
				MouseDown(loc);
			}
			break;

		case B_EXITED_VIEW:
			if (fBarView->Dragging() && buttons != 0) {
				if (!ItemAtPoint(where)
					&& !InBeMenu(where)
					&& (fSeparatorItem && !fSeparatorItem->Frame().Contains(where))
					&& !Frame().Contains(where))
					fBarView->DragStop();
	
			}
			break;
	}
	BMenuBar::MouseMoved(where, code, message);
}


bool
TExpandoMenuBar::InBeMenu(BPoint loc) const
{
	if (!fVertical) {
		if (fBeMenuItem && fBeMenuItem->Frame().Contains(loc))
			return true;
	} else {
		TBarWindow *window = dynamic_cast<TBarWindow*>(Window());
		if (window) {
			TBeMenu *bemenu = window->BeMenu();
			if (bemenu && bemenu->Frame().Contains(loc))
				return true;
		}					
	}

	return false;
}


TTeamMenuItem *
TExpandoMenuBar::ItemAtPoint(BPoint point)
{
	TTeamMenuItem *item = NULL;
	int32 count = CountItems();

	for (int32 i = fFirstApp; i < count; i++) {
		item = (TTeamMenuItem *)ItemAt(i);
		if (item && item->Frame().Contains(point))
			return item;
	}
	return NULL;
}


void
TExpandoMenuBar::AddTeam(BList *team, BBitmap *icon, char *name, char *signature)
{
	float itemWidth = fVertical ? Frame().Width() : kMinimumWindowWidth;
	float itemHeight = -1.0f;

	desk_settings *settings = ((TBarApp *)be_app)->Settings();
	TTeamMenuItem *item = new TTeamMenuItem(team, icon, name, signature, itemWidth,
		itemHeight, fDrawLabel, fVertical);

	if (settings->trackerAlwaysFirst && !strcmp(signature, kTrackerSignature)) {
		AddItem(item, fFirstApp);
	} else if (settings->sortRunningApps) {
		TTeamMenuItem *teamItem = dynamic_cast<TTeamMenuItem *>(ItemAt(fFirstApp));
		int32 firstApp = fFirstApp;

		// if Tracker should always be the first item, we need to skip it
		// when sorting in the current item
		if (settings->trackerAlwaysFirst && teamItem != NULL
			&& !strcmp(teamItem->Signature(), kTrackerSignature)) {
			firstApp++;
		}

		int32 count = CountItems(), i;
		for (i = firstApp; i < count; i++) {
			teamItem = dynamic_cast<TTeamMenuItem *>(ItemAt(i));
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
TExpandoMenuBar::AddTeam(team_id team, const char *signature)
{
	int32 count = CountItems();
	for (int32 i = fFirstApp; i < count; i++) {
		// Only add to team menu items
		if (TTeamMenuItem *item = dynamic_cast<TTeamMenuItem *>(ItemAt(i))) {
			if (strcasecmp(item->Signature(), signature) == 0) {
				if (!(item->Teams()->HasItem((void *)team)))
					item->Teams()->AddItem((void *)team);

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
		if (TTeamMenuItem *item = dynamic_cast<TTeamMenuItem *>(ItemAt(i))) {
			if (item->Teams()->HasItem((void *)team)) {
				item->Teams()->RemoveItem(team);

				if (partial)
					return;

#ifdef DOUBLECLICKBRINGSTOFRONT
				if (fLastClickItem == i)
					fLastClickItem = -1;
#endif

				RemoveItem(i);

				if (fVertical) {
					//	instead of resizing the window here and there in the code
					//	the resize method will be centered in one place
					//	thus, the same behavior (good or bad) will be used whereever
					//	window sizing is done
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
	float fullWidth = (kMinimumWindowWidth * count);

	if (!fBarView->Vertical()) {
		// in this case there are 2 extra items:
		//		The Be Menu
		//		The little separator item
		fullWidth = fullWidth - (kMinimumWindowWidth * 2) + (kBeMenuWidth + kSepItemWidth);
		width -= (kBeMenuWidth + kSepItemWidth);
		count -= 2;
	}

	if (delta >= 0 && fullWidth > width) {
		fOverflow = true;
		reset = true;
		newWidth = floorf(width/count);
	} else if (delta < 0 && fOverflow) {
		reset = true;
		if (fullWidth > width)
			newWidth = floorf(width/count);
		else
			newWidth = kMinimumWindowWidth;
	}
	if (newWidth > kMinimumWindowWidth)
		newWidth = kMinimumWindowWidth;

	if (reset) {
		SetMaxContentWidth(newWidth);
		if (newWidth == kMinimumWindowWidth)
			fOverflow = false;
		InvalidateLayout();

		for (int32 index = fFirstApp; ; index++) {
			TTeamMenuItem *item = (TTeamMenuItem *)ItemAt(index);
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
	BRect bounds(Bounds());
	rgb_color menuColor = ui_color(B_MENU_BACKGROUND_COLOR);
	rgb_color hilite = tint_color(menuColor, B_DARKEN_1_TINT);
	rgb_color dark = tint_color(menuColor, B_DARKEN_2_TINT);
	rgb_color vlight = tint_color(menuColor, B_LIGHTEN_2_TINT);
	int32 last = CountItems() - 1;
	float start;

	if (last >= 0) 
		start = ItemAt(last)->Frame().right + 1;
	else 
		start = 0;

	if (!fVertical) {
		SetHighColor(vlight);
		StrokeLine(BPoint(start, bounds.top+1), bounds.RightTop() + BPoint(0,1));
		StrokeLine(BPoint(start, bounds.top+1), BPoint(start, bounds.bottom));
		SetHighColor(hilite);
		StrokeLine(BPoint(start+1, bounds.bottom), bounds.RightBottom());
	}
}


//	something to help determine if we are showing too many apps
//	need to add in scrolling functionality

void
TExpandoMenuBar::CheckForSizeOverrun()
{
	BRect screenFrame = (BScreen(Window())).Frame();
	if (fVertical) 
		fIsScrolling = Window()->Frame().bottom > screenFrame.bottom;									
	else 
		fIsScrolling = false;
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
TExpandoMenuBar::monitor_team_windows(void *arg)
{
	TExpandoMenuBar *teamMenu = (TExpandoMenuBar *)arg;

	while (teamMenu->sDoMonitor) {
		sMonLocker.Lock();

		if (teamMenu->Window()->LockWithTimeout(50000) == B_OK) {
			int32 totalItems = teamMenu->CountItems();

			// Set all WindowMenuItems to require an update.
			TWindowMenuItem *item = NULL;
			for (int32 i = 0; i < totalItems; i++) {
				if (!teamMenu->SubmenuAt(i)){
					item = static_cast<TWindowMenuItem *>(teamMenu->ItemAt(i));
					item->SetRequireUpdate();
				}
			}

			// Perform SetTo() on all the items that still exist as well as add new items.
			bool itemModified = false, resize = false;
			TTeamMenuItem *teamItem = NULL;

			for (int32 i = 0; i < totalItems; i++) {
				if (teamMenu->SubmenuAt(i) == NULL)
					continue;

				teamItem = static_cast<TTeamMenuItem *>(teamMenu->ItemAt(i));
				if (teamItem->IsExpanded()) {
					int32 teamCount = teamItem->Teams()->CountItems();
					for (int32 j = 0; j < teamCount; j++) {
						// The following code is almost a copy/paste from
						// WindowMenu.cpp
						team_id	theTeam = (team_id)teamItem->Teams()->ItemAt(j);
						int32 count = 0;
						int32 *tokens = get_token_list(theTeam, &count);

						for (int32 k = 0; k < count; k++) {
							window_info *wInfo = get_window_info(tokens[k]);
							if (wInfo == NULL)
								continue;

							if (TWindowMenu::WindowShouldBeListed(wInfo->w_type)
								&& (wInfo->show_hide_level <= 0 || wInfo->is_mini)) {
								// Check if we have a matching window item...
								item = teamItem->ExpandedWindowItem(wInfo->id);
								if (item) {
									item->SetTo(wInfo->name, wInfo->id, wInfo->is_mini,
										((1 << current_workspace()) & wInfo->workspaces) != 0);

									if (strcmp(wInfo->name, item->Label()) != 0)
										item->SetLabel(wInfo->name);

									if (item->ChangedState())
										itemModified = true;
								} else if (teamItem->IsExpanded()) {
									// Add the item
									item = new TWindowMenuItem(wInfo->name, wInfo->id, 
										wInfo->is_mini,
										((1 << current_workspace()) & wInfo->workspaces) != 0,
										false);
									item->ExpandedItem(true);
									teamMenu->AddItem(item, i + 1);
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
				if (!teamMenu->SubmenuAt(i)){
					item = static_cast<TWindowMenuItem *>(teamMenu->ItemAt(i));
					if (item && item->RequiresUpdate()) {
						item = static_cast<TWindowMenuItem *>(teamMenu->RemoveItem(i));
						delete item;
						totalItems--;
	
						resize = true;
					}
				}
			}
	
			// If any of the WindowMenuItems changed state, we need to force a repaint.
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

