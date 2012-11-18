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


#include "WindowMenu.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <Locale.h>
#include <Window.h>

#include "BarApp.h"
#include "BarView.h"
#include "ExpandoMenuBar.h"
#include "ShowHideMenuItem.h"
#include "TeamMenu.h"
#include "TeamMenuItem.h"
#include "tracker_private.h"
#include "WindowMenuItem.h"


const int32 kDesktopWindow = 1024;
const int32 kMenuWindow	= 1025;
const uint32 kWindowScreen = 1026;
const uint32 kNormalWindow = 0;
const int32 kTeamFloater = 4;
const int32 kListFloater = 5;
const int32 kSystemFloater = 6;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WindowMenu"

bool
TWindowMenu::WindowShouldBeListed(client_window_info* info)
{
	return ((info->feel == kNormalWindow || info->feel == kWindowScreen)
			// Window has the right feel
		&& info->show_hide_level <= 0);
			// Window is not hidden
}


TWindowMenu::TWindowMenu(const BList* team, const char* signature)
	: BMenu("Deskbar Team Menu"),
	fTeam(team),
	fApplicationSignature(signature),
	fExpanded(false),
	fExpandedIndex(0)
{
	SetItemMargins(0.0f, 0.0f, 0.0f, 0.0f);
}


void
TWindowMenu::AttachedToWindow()
{
	SetFont(be_plain_font);
	RemoveItems(0, CountItems(), true);
	int32 miniCount = 0;

	bool dragging = false;
	TBarView* barview =(static_cast<TBarApp*>(be_app))->BarView();
	if (barview && barview->LockLooper()) {
		// 'dragging' mode set in BarView::CacheDragData
		// invoke in MouseEnter in ExpandoMenuBar
		dragging = barview->Dragging();
		if (dragging) {
			// We don't want to show the menu when dragging, but it's not
			// possible to remove a submenu once it exists, so we simply hide it
			// Don't call BMenu::Hide(), it causes the menu to pop up every now
			// and then.
			Window()->Hide();
			// if in expando (horizontal or vertical)
			if (barview->ExpandoState()) {
				SetTrackingHook(barview->MenuTrackingHook,
					barview->GetTrackingHookData());
			}
			barview->DragStart();
		}
		barview->UnlockLooper();
	}

	int32 parentMenuItems = 0;

	int32 teamCount = fTeam->CountItems();
	for (int32 i = 0; i < teamCount; i++) {
		team_id theTeam = (addr_t)fTeam->ItemAt(i);
		int32 tokenCount = 0;
		int32* tokens = get_token_list(theTeam, &tokenCount);

		for (int32 j = 0; j < tokenCount; j++) {
			client_window_info* wInfo = get_window_info(tokens[j]);
			if (wInfo == NULL)
				continue;

			if (WindowShouldBeListed(wInfo)) {
				// Don't add new items if we're expanded. We've already done
				// this, they've just been moved.
				int32 numItems = CountItems();

				// Find first item that sorts alphabetically after this window,
				// so we know where to put it
				for (int32 addIndex = 0; addIndex < numItems; addIndex++) {
					TWindowMenuItem* item
						= static_cast<TWindowMenuItem*>(ItemAt(addIndex));
					if (item != NULL
						&& strcasecmp(item->FullTitle(), wInfo->name) > 0)
						break;
				}

				if (!fExpanded) {
					TWindowMenuItem* item = new TWindowMenuItem(wInfo->name,
						wInfo->server_token, wInfo->is_mini,
						((1 << current_workspace()) & wInfo->workspaces) != 0,
						dragging);

					// disable app's window dropping for now
					if (dragging)
						item->SetEnabled(false);

					AddItem(item,
						TWindowMenuItem::InsertIndexFor(this, 0, item));
				} else {
					TTeamMenuItem* parentItem
						= static_cast<TTeamMenuItem*>(Superitem());
					if (parentItem->ExpandedWindowItem(wInfo->server_token)) {
						TWindowMenuItem* item = parentItem->ExpandedWindowItem(
							wInfo->server_token);
						if (item == NULL)
							continue;

						item->SetTo(wInfo->name, wInfo->server_token,
							wInfo->is_mini,
							((1 << current_workspace()) & wInfo->workspaces)
								!= 0, dragging);
						parentMenuItems++;
					}
				}

				if (wInfo->is_mini)
					miniCount++;
			}
			free(wInfo);
		}
		free(tokens);
	}

	int32 itemCount = CountItems() + parentMenuItems;
	if (itemCount < 1) {
		TWindowMenuItem* noWindowsItem
			= new TWindowMenuItem(B_TRANSLATE("No windows"), -1, false, false);

		noWindowsItem->SetEnabled(false);
		AddItem(noWindowsItem);

		// Add a 'Quit application' item if no windows are open
		// unless the application is Tracker
		if (fApplicationSignature.ICompare(kTrackerSignature) != 0) {
			AddSeparatorItem();
			AddItem(new TShowHideMenuItem(B_TRANSLATE("Quit application"),
				fTeam, B_QUIT_REQUESTED));
		}
	} else {
		// Only add the window controls to the menu if we are not in drag mode
		if (!dragging) {
			TShowHideMenuItem* hide
				= new TShowHideMenuItem(B_TRANSLATE("Hide all"), fTeam,
					B_MINIMIZE_WINDOW);
			TShowHideMenuItem* show
				= new TShowHideMenuItem(B_TRANSLATE("Show all"), fTeam,
					B_BRING_TO_FRONT);
			TShowHideMenuItem* close
				= new TShowHideMenuItem(B_TRANSLATE("Close all"), fTeam,
					B_QUIT_REQUESTED);

			if (miniCount == itemCount)
				hide->SetEnabled(false);
			else if (miniCount == 0)
				show->SetEnabled(false);

			if (!parentMenuItems)
				AddSeparatorItem();

			AddItem(hide);
			AddItem(show);
			AddItem(close);
		}
	}

	BMenu::AttachedToWindow();
}


void
TWindowMenu::DetachedFromWindow()
{
	// in expando mode the teammenu will not call DragStop, thus, it needs to
	// be called from here
	TBarView* barview = (dynamic_cast<TBarApp*>(be_app))->BarView();
	if (barview && barview->ExpandoState() && barview->Dragging()
		&& barview->LockLooper()) {
		// We changed the show level in AttachedToWindow(). Undo it.
		Window()->Show();
		barview->DragStop();
		barview->UnlockLooper();
	}

	BMenu::DetachedFromWindow();
}


BPoint
TWindowMenu::ScreenLocation()
{
	return BMenu::ScreenLocation();
}


void
TWindowMenu::SetExpanded(bool status, int lastIndex)
{
	fExpanded = status;
	fExpandedIndex = lastIndex;
}
