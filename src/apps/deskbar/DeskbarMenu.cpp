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


#include "DeskbarMenu.h"

#include <Debug.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Dragger.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Roster.h>

#include "BarApp.h"
#include "BarView.h"
#include "DeskbarUtils.h"
#include "IconMenuItem.h"
#include "MountMenu.h"
#include "IconMenuItem.h"
#include "MountMenu.h"
#include "IconMenuItem.h"
#include "MountMenu.h"
#include "PublicCommands.h"
#include "RecentItems.h"
#include "StatusView.h"
#include "tracker_private.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeskbarMenu"

#define ROSTER_SIG "application/x-vnd.Be-ROST"

#ifdef MOUNT_MENU_IN_DESKBAR

class DeskbarMountMenu : public BPrivate::MountMenu {
	public:
		DeskbarMountMenu(const char* name);
		virtual bool AddDynamicItem(add_state s);
};

#endif

// #define SHOW_RECENT_FIND_ITEMS

namespace BPrivate {
	BMenu* TrackerBuildRecentFindItemsMenu(const char*);
}


using namespace BPrivate;


//	#pragma mark -


TDeskbarMenu::TDeskbarMenu(TBarView* barView)
	: BNavMenu("DeskbarMenu", B_REFS_RECEIVED, DefaultTarget()),
	fAddState(kStart),
	fBarView(barView)
{
}


void
TDeskbarMenu::AttachedToWindow()
{
	if (fBarView && fBarView->LockLooper()) {
		if (fBarView->Dragging()) {
			SetTypesList(fBarView->CachedTypesList());
			SetTarget(BMessenger(fBarView));
			SetTrackingHookDeep(this, fBarView->MenuTrackingHook,
				fBarView->GetTrackingHookData());
			fBarView->DragStart();
		} else {
			SetTypesList(NULL);
			SetTarget(DefaultTarget());
			SetTrackingHookDeep(this, NULL, NULL);
		}

		fBarView->UnlockLooper();
	}

	BNavMenu::AttachedToWindow();
}


void
TDeskbarMenu::DetachedFromWindow()
{
	if (fBarView) {
		BLooper* looper = fBarView->Looper();
		if (looper && looper->Lock()) {
			fBarView->DragStop();
			looper->Unlock();
		}
	}

	// don't call BNavMenu::DetachedFromWindow
	// it sets the TypesList to NULL
	BMenu::DetachedFromWindow();
}


bool
TDeskbarMenu::StartBuildingItemList()
{
	RemoveItems(0, CountItems(), true);
	fAddState = kStart;
	return BNavMenu::StartBuildingItemList();
}


void
TDeskbarMenu::DoneBuildingItemList()
{
	if (fItemList->CountItems() <= 0) {
		BMenuItem* item
			= new BMenuItem(B_TRANSLATE("<Deskbar folder is empty>"), 0);
		item->SetEnabled(false);
		AddItem(item);
	} else
		BNavMenu::DoneBuildingItemList();
}


bool
TDeskbarMenu::AddNextItem()
{
	if (fAddState == kStart)
		return AddStandardDeskbarMenuItems();

	TrackingHookData* data = fBarView->GetTrackingHookData();
	if (fAddState == kAddingRecents) {
		static const char* recentTitle[] = {
			B_TRANSLATE_MARK("Recent documents"),
			B_TRANSLATE_MARK("Recent folders"),
			B_TRANSLATE_MARK("Recent applications")};
		const int recentType[] = {kRecentDocuments, kRecentFolders,
			kRecentApplications};
		const int recentTypes = 3;
		TRecentsMenu* recentItem[recentTypes];

		bool enabled = false;

		for (int i = 0; i < recentTypes; i++) {
			recentItem[i]
				= new TRecentsMenu(B_TRANSLATE_NOCOLLECT(recentTitle[i]),
					fBarView, recentType[i]);

			if (recentItem[i])
				enabled |= recentItem[i]->RecentsEnabled();
		}
		if (enabled) {
			AddSeparatorItem();

			for (int i = 0; i < recentTypes; i++) {
				if (!recentItem[i])
					continue;

				if (recentItem[i]->RecentsEnabled()) {
					recentItem[i]->SetTypesList(TypesList());
					recentItem[i]->SetTarget(Target());
					AddItem(recentItem[i]);
				}

				if (data && fBarView && fBarView->Dragging()) {
					recentItem[i]->InitTrackingHook(data->fTrackingHook,
						&data->fTarget, data->fDragMessage);
				}
			}
		}

		AddSeparatorItem();
		fAddState = kAddingDeskbarMenu;
		return true;
	}

	if (fAddState == kAddingDeskbarMenu) {
		// keep reentering and adding items
		// until this returns false
		bool done = BNavMenu::AddNextItem();
		BMenuItem* item = ItemAt(CountItems() - 1);
		if (item) {
			BNavMenu* menu = dynamic_cast<BNavMenu*>(item->Menu());
			if (menu) {
				if (data && fBarView->Dragging()) {
					menu->InitTrackingHook(data->fTrackingHook,
						&data->fTarget, data->fDragMessage);
				} else
					menu->InitTrackingHook(0, NULL, NULL);
			}
		}

		if (!done)
			fAddState = kDone;
		return done;
	}

	return false;
}


bool
TDeskbarMenu::AddStandardDeskbarMenuItems()
{
	bool dragging = false;
	if (fBarView)
		dragging = fBarView->Dragging();

	BMenuItem* item;
	BRoster roster;
	if (!roster.IsRunning(kTrackerSignature)) {
		item = new BMenuItem(B_TRANSLATE("Restart Tracker"),
			new BMessage(kRestartTracker));
		AddItem(item);
		AddSeparatorItem();
	}

// One of them is used if HAIKU_DISTRO_COMPATIBILITY_OFFICIAL, and the other if
// not. However, we want both of them to end up in the catalog, so we have to
// make them visible to collectcatkeys in either case.
#if defined(B_COLLECTING_CATKEYS)||defined(HAIKU_DISTRO_COMPATIBILITY_OFFICIAL)
	static const char* kAboutHaikuMenuItemStr = B_TRANSLATE_MARK(
		"About Haiku");
#endif

#if defined(B_COLLECTING_CATKEYS)||!defined(HAIKU_DISTRO_COMPATIBILITY_OFFICIAL)
	static const char* kAboutThisSystemMenuItemStr = B_TRANSLATE_MARK(
		"About this system");
#endif

	item = new BMenuItem(
#ifdef HAIKU_DISTRO_COMPATIBILITY_OFFICIAL
	B_TRANSLATE_NOCOLLECT(kAboutHaikuMenuItemStr)
#else
	B_TRANSLATE_NOCOLLECT(kAboutThisSystemMenuItemStr)
#endif
		, new BMessage(kShowSplash));
	item->SetEnabled(!dragging);
	AddItem(item);

	static const char* kFindMenuItemStr
		= B_TRANSLATE_MARK("Find" B_UTF8_ELLIPSIS);

#ifdef SHOW_RECENT_FIND_ITEMS
	item = new BMenuItem(
		TrackerBuildRecentFindItemsMenu(kFindMenuItemStr),
		new BMessage(kFindButton));
#else
	item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kFindMenuItemStr),
		new BMessage(kFindButton));
#endif
	item->SetEnabled(!dragging);
	AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Show replicants"),
		new BMessage(kToggleDraggers));
	item->SetEnabled(!dragging);
	item->SetMarked(BDragger::AreDraggersDrawn());
	AddItem(item);

	static const char* kMountMenuStr = B_TRANSLATE_MARK("Mount");

#ifdef MOUNT_MENU_IN_DESKBAR
	DeskbarMountMenu* mountMenu = new DeskbarMountMenu(
		B_TRANSLATE_NOCOLLECT(kMountMenuStr));
	mountMenu->SetEnabled(!dragging);
	AddItem(mountMenu);
#endif

	item = new BMenuItem(B_TRANSLATE("Deskbar preferences" B_UTF8_ELLIPSIS),
		new BMessage(kConfigShow));
	item->SetTarget(be_app);
	AddItem(item);

	AddSeparatorItem();

	BMenu* shutdownMenu = new BMenu(B_TRANSLATE("Shutdown" B_UTF8_ELLIPSIS));

	item = new BMenuItem(B_TRANSLATE("Restart system"),
		new BMessage(kRebootSystem));
	item->SetEnabled(!dragging);
	shutdownMenu->AddItem(item);

#if defined(APM_SUPPORT) || defined(B_COLLECTING_CATKEYS)
	static const char* kSuspendMenuItemStr = B_TRANSLATE_MARK("Suspend");
#endif

#ifdef APM_SUPPORT
	if (_kapm_control_(APM_CHECK_ENABLED) == B_OK) {
		item = new BMenuItem(B_TRANSLATE_NOCOLLECT(kSuspendMenuItemStr),
			new BMessage(kSuspendSystem));
		item->SetEnabled(!dragging);
		shutdownMenu->AddItem(item);
	}
#endif

	item = new BMenuItem(B_TRANSLATE("Power off"),
		new BMessage(kShutdownSystem));
	item->SetEnabled(!dragging);
	shutdownMenu->AddItem(item);
	shutdownMenu->SetFont(be_plain_font);

	shutdownMenu->SetTargetForItems(be_app);
	BMessage* message = new BMessage(kShutdownSystem);
	message->AddBool("confirm", true);
	AddItem(new BMenuItem(shutdownMenu, message));

	fAddState = kAddingRecents;

	return true;
}


void
TDeskbarMenu::ClearMenuBuildingState()
{
	fAddState = kDone;
	fMenuBuilt = false;
		// force the menu to get rebuilt each time
	BNavMenu::ClearMenuBuildingState();
}


void
TDeskbarMenu::ResetTargets()
{
	// This method does not recurse into submenus
	// and does not affect menu items in submenus.
	// (e.g. "Restart System" and "Power Off")

	BNavMenu::ResetTargets();

	// if we are dragging, set the target to whatever was set
	// else set it to the default (Tracker)
	if (!fBarView->Dragging())
		SetTarget(DefaultTarget());

	// now set the target for the menuitems to the currently
	// set target, which may or may not be tracker
	SetTargetForItems(Target());

	for (int32 i = 0; ; i++) {
		BMenuItem* item = ItemAt(i);
		if (item == NULL)
			break;

		if (item->Message()) {
			switch (item->Message()->what) {
				case kFindButton:
					item->SetTarget(BMessenger(kTrackerSignature));
					break;

				case kShowSplash:
				case kToggleDraggers:
				case kConfigShow:
				case kAlwaysTop:
				case kExpandNewTeams:
				case kHideLabels:
				case kResizeTeamIcons:
				case kSortRunningApps:
				case kTrackerFirst:
				case kRebootSystem:
				case kSuspendSystem:
				case kShutdownSystem:
					item->SetTarget(be_app);
					break;

				case kShowHideTime:
				case kShowSeconds:
				case kShowDayOfWeek:
					item->SetTarget(fBarView->fReplicantTray);
					break;
			}
		}
	}
}


BPoint
TDeskbarMenu::ScreenLocation()
{
	bool vertical = fBarView->Vertical();
	int32 expando = (fBarView->State() == kExpandoState);
	BPoint point;

	BRect rect = Supermenu()->Bounds();
	Supermenu()->ConvertToScreen(&rect);

	if (expando && vertical && fBarView->Left()) {
		PRINT(("Left\n"));
		point = rect.RightTop() + BPoint(0, 3);
	} else if (expando && vertical && !fBarView->Left()) {
		PRINT(("Right\n"));
		point = rect.LeftTop() - BPoint(Bounds().Width(), 0) + BPoint(0, 3);
	} else
		point = BMenu::ScreenLocation();

	return point;
}


/*static*/
BMessenger
TDeskbarMenu::DefaultTarget()
{
	// if Tracker is not available we target the BarApp
	BMessenger target(kTrackerSignature);
	if (target.IsValid())
		return target;

	return BMessenger(be_app);
}


//	#pragma mark -


TRecentsMenu::TRecentsMenu(const char* name, TBarView* bar, int32 which,
		const char* signature, entry_ref* appRef)
	: BNavMenu(name, B_REFS_RECEIVED, TDeskbarMenu::DefaultTarget()),
	fWhich(which),
	fAppRef(NULL),
	fSignature(NULL),
	fRecentsCount(0),
	fRecentsEnabled(false),
	fItemIndex(0),
	fBarView(bar)
{
	TBarApp* app = dynamic_cast<TBarApp*>(be_app);
	if (app == NULL)
		return;

	switch (which) {
		case kRecentDocuments:
			fRecentsCount = app->Settings()->recentDocsCount;
			fRecentsEnabled = app->Settings()->recentDocsEnabled;
			break;
		case kRecentApplications:
			fRecentsCount = app->Settings()->recentAppsCount;
			fRecentsEnabled = app->Settings()->recentAppsEnabled;
			break;
		case kRecentAppDocuments:
			fRecentsCount = app->Settings()->recentDocsCount;
			fRecentsEnabled = app->Settings()->recentDocsEnabled;
			if (signature != NULL)
				fSignature = strdup(signature);
			if (appRef != NULL)
				fAppRef = new entry_ref(*appRef);
			break;
		case kRecentFolders:
			fRecentsCount = app->Settings()->recentFoldersCount;
			fRecentsEnabled = app->Settings()->recentFoldersEnabled;
			break;
	}
}


TRecentsMenu::~TRecentsMenu()
{
	delete fAppRef;
	free(fSignature);
}


void
TRecentsMenu::DetachedFromWindow()
{
	// BNavMenu::DetachedFromWindow sets the TypesList to NULL
	BMenu::DetachedFromWindow();
}


bool
TRecentsMenu::StartBuildingItemList()
{
	RemoveItems(0, CountItems(), true);

	// !! note: don't call inherited from here
	// the navref is not set for this menu
	// but it still needs to be a draggable navmenu
	// simply return true so that AddNextItem is called
	//
	// return BNavMenu::StartBuildingItemList();
	return true;
}


bool
TRecentsMenu::AddNextItem()
{
	if (fRecentsCount > 0 && fRecentsEnabled && AddRecents(fRecentsCount))
		return true;

	fItemIndex = 0;
	return false;
}


bool
TRecentsMenu::AddRecents(int32 count)
{
	if (fItemIndex == 0) {
		fRecentList.MakeEmpty();
		BRoster roster;

		switch (fWhich) {
			case kRecentDocuments:
				roster.GetRecentDocuments(&fRecentList, count);
				break;
			case kRecentApplications:
				roster.GetRecentApps(&fRecentList, count);
				break;
			case kRecentAppDocuments:
				roster.GetRecentDocuments(&fRecentList, count, NULL,
					fSignature);
				break;
			case kRecentFolders:
				roster.GetRecentFolders(&fRecentList, count);
				break;
			default:
				return false;
		}
	}

	for (;;) {
		entry_ref ref;
		if (fRecentList.FindRef("refs", fItemIndex++, &ref) != B_OK)
			break;

		if (ref.name && strlen(ref.name) > 0) {
			Model model(&ref, true);

			if (fWhich != kRecentApplications) {
				BMessage* message = new BMessage(B_REFS_RECEIVED);
				if (fWhich == kRecentAppDocuments) {
					// add application as handler
					message->AddRef("handler", fAppRef);
				}

				ModelMenuItem* item = BNavMenu::NewModelItem(&model,
					message, Target(), false, NULL, TypesList());

				if (item)
					AddItem(item);
			} else {
				// The application items expand to a list of recent documents
				// for that application - so they must be handled extra
				BFile file(&ref, B_READ_ONLY);
				char signature[B_MIME_TYPE_LENGTH];

				BAppFileInfo appInfo(&file);
				if (appInfo.InitCheck() != B_OK
					|| appInfo.GetSignature(signature) != B_OK)
					continue;

				ModelMenuItem* item = NULL;
				BMessage doc;
				be_roster->GetRecentDocuments(&doc, 1, NULL, signature);
					// ToDo: check if the documents do exist at all to
					//		avoid the creation of the submenu.

				if (doc.CountNames(B_REF_TYPE) > 0) {
					// create recents menu that will contain the recent docs of
					// this app
					TRecentsMenu* docs = new TRecentsMenu(model.Name(),
						fBarView, kRecentAppDocuments, signature, &ref);
					docs->SetTypesList(TypesList());
					docs->SetTarget(Target());

					item = new ModelMenuItem(&model, docs);
				} else
					item = new ModelMenuItem(&model, model.Name(), NULL);

				if (item) {
					// add refs-message so that the recent app can be launched
					BMessage* msg = new BMessage(B_REFS_RECEIVED);
					msg->AddRef("refs", &ref);
					item->SetMessage(msg);
					item->SetTarget(Target());

					AddItem(item);
				}
			}

			// return true so that we know to reenter this list
			return true;
		}
	}

	// return false if we are done with this list
	return false;
}


void
TRecentsMenu::DoneBuildingItemList()
{
	// !! note: don't call inherited here
	// the object list is not built
	// and this list does not need to be sorted
	// BNavMenu::DoneBuildingItemList();

	if (CountItems() > 0)
		SetTargetForItems(Target());
}


void
TRecentsMenu::ClearMenuBuildingState()
{
	fMenuBuilt = false;
	BNavMenu::ClearMenuBuildingState();
}


void
TRecentsMenu::ResetTargets()
{
	BNavMenu::ResetTargets();

	// if we are dragging, set the target to whatever was set
	// else set it to the default (Tracker)
	if (!fBarView->Dragging())
		SetTarget(TDeskbarMenu::DefaultTarget());

	// now set the target for the menuitems to the currently
	// set target, which may or may not be tracker
	SetTargetForItems(Target());
}


//*****************************************************************************
//	#pragma mark -


#ifdef MOUNT_MENU_IN_DESKBAR

DeskbarMountMenu::DeskbarMountMenu(const char* name)
	: BPrivate::MountMenu(name)
{
	SetFont(be_plain_font);
}


bool
DeskbarMountMenu::AddDynamicItem(add_state s)
{
	BPrivate::MountMenu::AddDynamicItem(s);

	SetTargetForItems(BMessenger(kTrackerSignature));

	return false;
}

#endif
