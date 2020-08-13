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


#include "StatusView.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>

#include <fs_index.h>
#include <fs_info.h>

#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Debug.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#include "icons.h"

#include "BarApp.h"
#include "BarMenuBar.h"
#include "DeskbarUtils.h"
#include "ExpandoMenuBar.h"
#include "ResourceSet.h"
#include "StatusViewShelf.h"
#include "TimeView.h"


static const float kVerticalMiniMultiplier = 2.9f;


#ifdef DB_ADDONS
// Add-on support
//
// Item - internal item list (node, eref, etc)
// Icon - physical replicant handed to the DeskbarClass class
// AddOn - attribute based add-on

const char* const kInstantiateItemCFunctionName = "instantiate_deskbar_item";
const char* const kInstantiateEntryCFunctionName = "instantiate_deskbar_entry";
const char* const kReplicantSettingsFile = "replicants";
const char* const kReplicantPathField = "replicant_path";

float gMinimumWindowWidth = kGutter + kMinimumTrayWidth + kDragRegionWidth;
float gMaximumWindowWidth = gMinimumWindowWidth * 2;


static void
DumpItem(DeskbarItemInfo* item)
{
	printf("is addon: %i, id: %" B_PRId32 "\n", item->isAddOn, item->id);
	printf("entry_ref:  %" B_PRIdDEV ", %" B_PRIdINO ", %s\n",
		item->entryRef.device, item->entryRef.directory, item->entryRef.name);
	printf("node_ref:  %" B_PRIdDEV ", %" B_PRIdINO "\n", item->nodeRef.device,
		item->nodeRef.node);
}


static void
DumpList(BList* itemlist)
{
	int32 count = itemlist->CountItems() - 1;
	if (count < 0) {
		printf("no items in list\n");
		return;
	}
	for (int32 i = count; i >= 0; i--) {
		DeskbarItemInfo* item = (DeskbarItemInfo*)itemlist->ItemAt(i);
		if (!item)
			continue;

		DumpItem(item);
	}
}
#endif	/* DB_ADDONS */


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tray"

// don't change the name of this view to anything other than "Status"!

TReplicantTray::TReplicantTray(TBarView* barView)
	:
	BView(BRect(0, 0, 1, 1), "Status", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_FRAME_EVENTS),
	fTime(NULL),
	fBarView(barView),
	fShelf(new TReplicantShelf(this)),
	fAlignmentSupport(false)
{
	// scale replicants by font size
	fMaxReplicantHeight = std::max(kMinReplicantHeight,
		floorf(kMinReplicantHeight * be_plain_font->Size() / 12));
	// but not bigger than TabHeight which depends on be_bold_font
	// TODO this should only apply to mini-mode but we set it once here for all
	fMaxReplicantHeight = std::min(fMaxReplicantHeight,
		fBarView->TabHeight() - 4);
	// TODO: depends on window size... (so use something like
	// max(129, height * 3), and restrict the minimum window width for it)
	// Use bold font because it depends on the window tab height.
	fMaxReplicantWidth = 129;

	fMinTrayHeight = kGutter + fMaxReplicantHeight + kGutter;
	if (fBarView != NULL && fBarView->Vertical()
		&& (fBarView->ExpandoState() || fBarView->FullState())) {
		fMinimumTrayWidth = gMinimumWindowWidth - kGutter - kDragRegionWidth;
	} else
		fMinimumTrayWidth = kMinimumTrayWidth;

	// Create the time view
	fTime = new TTimeView(fMinimumTrayWidth, fMaxReplicantHeight - 1.0,
		fBarView);
}


TReplicantTray::~TReplicantTray()
{
	delete fShelf;
	delete fTime;
}


void
TReplicantTray::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (be_control_look != NULL) {
		AdoptParentColors();
	} else {
		SetViewUIColor(B_MENU_BACKGROUND_COLOR,	B_DARKEN_1_TINT);
	}
	SetDrawingMode(B_OP_COPY);

	Window()->SetPulseRate(1000000);

	clock_settings* clock = ((TBarApp*)be_app)->ClockSettings();
	fTime->SetShowSeconds(clock->showSeconds);
	fTime->SetShowDayOfWeek(clock->showDayOfWeek);
	fTime->SetShowTimeZone(clock->showTimeZone);

	AddChild(fTime);

	fTime->MoveTo(Bounds().right - fTime->Bounds().Width() - kTrayPadding, 2);
		// will be moved into place later

	if (!((TBarApp*)be_app)->Settings()->showClock)
		fTime->Hide();

#ifdef DB_ADDONS
	// load addons and rehydrate archives
#if !defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST)
	InitAddOnSupport();
#endif
#endif
	ResizeToPreferred();
}


void
TReplicantTray::DetachedFromWindow()
{
#ifdef DB_ADDONS
	// clean up add-on support
#if !defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST)
	DeleteAddOnSupport();
#endif
#endif
	BView::DetachedFromWindow();
}


/*! Width is set to a minimum of kMinimumReplicantCount by kMaxReplicantWidth
	if not in multirowmode and greater than kMinimumReplicantCount
	the width should be calculated based on the actual replicant widths
*/
void
TReplicantTray::GetPreferredSize(float* preferredWidth, float* preferredHeight)
{
	float width = 0;
	float height = fMinTrayHeight;

	if (fBarView->Vertical()) {
		width = static_cast<TBarApp*>(be_app)->Settings()->width
			- kDragWidth - kGutter;
		if (fRightBottomReplicant.IsValid())
			height = fRightBottomReplicant.bottom;
		else if (ReplicantCount() > 0) {
			// The height will be uniform for the number of rows necessary
			// to show all the replicants and gutters.
			int32 rowCount = (int32)(height / fMaxReplicantHeight);
			height = kGutter + (rowCount * fMaxReplicantHeight)
				+ ((rowCount - 1) * kIconGap) + kGutter;
			height = std::max(fMinTrayHeight, height);
		} else
			height = fMinTrayHeight;
	} else {
		// if last replicant overruns clock then resize to accomodate
		if (ReplicantCount() > 0) {
			if (!fTime->IsHidden(fTime) && Bounds().right - kTrayPadding - 2
						- fTime->Frame().Width() - kClockMargin
					< fRightBottomReplicant.right + kClockMargin) {
				width = fRightBottomReplicant.right + kClockMargin
					+ fTime->Frame().Width() + kTrayPadding + 2;
			} else
				width = fRightBottomReplicant.right + kIconGap + kGutter;
		}

		// this view has a fixed minimum width
		width = std::max(kMinimumTrayWidth, width);

		// if mini-mode set to tab height
		// else if horizontal mode set to team menu item height
		if (fBarView->MiniState())
			height = std::max(fMinTrayHeight, fBarView->TabHeight());
		else
			height = fBarView->TeamMenuItemHeight();
	}

	*preferredWidth = width;
	// add 1 for the border
	*preferredHeight = height + 1;
}


void
TReplicantTray::AdjustPlacement()
{
	// called when an add-on has been added or removed
	// need to resize the parent of this accordingly

	BRect bounds = Bounds();
	float width, height;
	GetPreferredSize(&width, &height);

	if (width == bounds.Width() && height == bounds.Height()) {
		// no need to change anything
		return;
	}

	Parent()->ResizeToPreferred();
	fBarView->UpdatePlacement();
	Parent()->Invalidate();
	Invalidate();
}


void
TReplicantTray::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_LOCALE_CHANGED:
			if (fTime == NULL)
				return;

			fTime->UpdateTimeFormat();
			fTime->Update();
			// time string reformat -> realign
			goto realignReplicants;

		case kShowHideTime:
			// from context menu in clock and in this view
			ShowHideTime();
			break;

		case kShowSeconds:
			if (fTime == NULL)
				return;

			fTime->SetShowSeconds(!fTime->ShowSeconds());

			// time string reformat -> realign
			goto realignReplicants;

		case kShowDayOfWeek:
			if (fTime == NULL)
				return;

			fTime->SetShowDayOfWeek(!fTime->ShowDayOfWeek());

			// time string reformat -> realign
			goto realignReplicants;

		case kShowTimeZone:
			if (fTime == NULL)
				return;

			fTime->SetShowTimeZone(!fTime->ShowTimeZone());

			// time string reformat -> realign
			goto realignReplicants;

		case kGetClockSettings:
		{
			if (fTime == NULL)
				return;

			bool showClock = !fTime->IsHidden(fTime);
			bool showSeconds = fTime->ShowSeconds();
			bool showDayOfWeek = fTime->ShowDayOfWeek();
			bool showTimeZone = fTime->ShowTimeZone();

			BMessage reply(kGetClockSettings);
			reply.AddBool("showClock", showClock);
			reply.AddBool("showSeconds", showSeconds);
			reply.AddBool("showDayOfWeek", showDayOfWeek);
			reply.AddBool("showTimeZone", showTimeZone);
			message->SendReply(&reply);
			break;
		}

#ifdef DB_ADDONS
		case B_NODE_MONITOR:
			HandleEntryUpdate(message);
			break;
#endif

		case kRealignReplicants:
realignReplicants:
			RealignReplicants();
			AdjustPlacement();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TReplicantTray::MouseDown(BPoint where)
{
#ifdef DB_ADDONS
	if (modifiers() & B_CONTROL_KEY)
		DumpList(fItemList);
#endif

	uint32 buttons;

	Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);
	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		ShowReplicantMenu(where);
	} else {
		BPoint save = where;
		bigtime_t doubleClickSpeed;
		bigtime_t start = system_time();
		uint32 buttons;

		get_click_speed(&doubleClickSpeed);

		do {
			if (fabs(where.x - save.x) > 4 || fabs(where.y - save.y) > 4)
				// user moved out of bounds of click area
				break;

			if ((system_time() - start) > (2 * doubleClickSpeed)) {
				ShowReplicantMenu(where);
				break;
			}

			snooze(50000);
			GetMouse(&where, &buttons);
		} while (buttons);
	}
	BView::MouseDown(where);
}


void
TReplicantTray::ShowReplicantMenu(BPoint point)
{
	BPopUpMenu* menu = new BPopUpMenu("", false, false);
	menu->SetFont(be_plain_font);

	// If clock is visible show the extended menu, otherwise show "Show clock"

	if (!fTime->IsHidden(fTime))
		fTime->ShowTimeOptions(ConvertToScreen(point));
	else {
		BMenuItem* item = new BMenuItem(B_TRANSLATE("Show clock"),
			new BMessage(kShowHideTime));
		menu->AddItem(item);
		menu->SetTargetForItems(this);
		BPoint where = ConvertToScreen(point);
		menu->Go(where, true, true, BRect(where - BPoint(4, 4),
			where + BPoint(4, 4)), true);
	}
}


void
TReplicantTray::ShowHideTime()
{
	if (fTime == NULL)
		return;

	// Check from the point of view of fTime because we need to ignore
	// whether or not the parent window is hidden.
	if (fTime->IsHidden(fTime))
		fTime->Show();
	else
		fTime->Hide();

	RealignReplicants();
	AdjustPlacement();

	// Check from the point of view of fTime ignoring parent's state.
	bool showClock = !fTime->IsHidden(fTime);

	// Update showClock setting that gets saved to disk on quit
	static_cast<TBarApp*>(be_app)->Settings()->showClock = showClock;

	// Send a message to Time preferences telling it to update
	BMessenger messenger("application/x-vnd.Haiku-Time");
	BMessage message(kShowHideTime);
	message.AddBool("showClock", showClock);
	messenger.SendMessage(&message);
}


#ifdef DB_ADDONS


void
TReplicantTray::InitAddOnSupport()
{
	// list to maintain refs to each rep added/deleted
	fItemList = new BList();
	BPath path;

	if (GetDeskbarSettingsDirectory(path, true) == B_OK) {
		path.Append(kReplicantSettingsFile);

		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			status_t result;
			BEntry entry;
			int32 id;
			BString path;
			if (fAddOnSettings.Unflatten(&file) == B_OK) {
				for (int32 i = 0; fAddOnSettings.FindString(kReplicantPathField,
					i, &path) == B_OK; i++) {
					if (entry.SetTo(path.String()) == B_OK && entry.Exists()) {
						result = LoadAddOn(&entry, &id, false);
					} else
						result = B_ENTRY_NOT_FOUND;

					if (result != B_OK) {
						fAddOnSettings.RemoveData(kReplicantPathField, i);
						--i;
					}
				}
			}
		}
	}
}


void
TReplicantTray::DeleteAddOnSupport()
{
	_SaveSettings();

	for (int32 i = fItemList->CountItems() - 1; i >= 0; i--) {
		DeskbarItemInfo* item = (DeskbarItemInfo*)fItemList->RemoveItem(i);
		if (item) {
			if (item->isAddOn)
				watch_node(&(item->nodeRef), B_STOP_WATCHING, this, Window());

			delete item;
		}
	}
	delete fItemList;

	// stop the volume mount/unmount watch
	stop_watching(this, Window());
}


DeskbarItemInfo*
TReplicantTray::DeskbarItemFor(node_ref& nodeRef)
{
	for (int32 i = fItemList->CountItems() - 1; i >= 0; i--) {
		DeskbarItemInfo* item = (DeskbarItemInfo*)fItemList->ItemAt(i);
		if (item == NULL)
			continue;

		if (item->nodeRef == nodeRef)
			return item;
	}

	return NULL;
}


DeskbarItemInfo*
TReplicantTray::DeskbarItemFor(int32 id)
{
	for (int32 i = fItemList->CountItems() - 1; i >= 0; i--) {
		DeskbarItemInfo* item = (DeskbarItemInfo*)fItemList->ItemAt(i);
		if (item == NULL)
			continue;

		if (item->id == id)
			return item;
	}

	return NULL;
}


bool
TReplicantTray::NodeExists(node_ref& nodeRef)
{
	return DeskbarItemFor(nodeRef) != NULL;
}


/*! This handles B_NODE_MONITOR & B_QUERY_UPDATE messages received
	for the registered add-ons.
*/
void
TReplicantTray::HandleEntryUpdate(BMessage* message)
{
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_OK)
		return;

	BPath path;
	switch (opcode) {
		case B_ENTRY_MOVED:
		{
			entry_ref ref;
			ino_t todirectory;
			ino_t node;
			const char* name;
			if (message->FindString("name", &name) == B_OK
				&& message->FindInt64("from directory", &(ref.directory))
					== B_OK
				&& message->FindInt64("to directory", &todirectory) == B_OK
				&& message->FindInt32("device", &(ref.device)) == B_OK
				&& message->FindInt64("node", &node) == B_OK ) {

				if (name == NULL)
					break;

				ref.set_name(name);
				// change the directory reference to
				// the new directory
				MoveItem(&ref, todirectory);
			}
			break;
		}

		case B_ENTRY_REMOVED:
		{
			// entry was rm'd from the device
			node_ref nodeRef;
			if (message->FindInt32("device", &(nodeRef.device)) == B_OK
				&& message->FindInt64("node", &(nodeRef.node)) == B_OK) {
				DeskbarItemInfo* item = DeskbarItemFor(nodeRef);
				if (item == NULL)
					break;

				// If there is a team running where the add-on comes from,
				// we don't want to remove the icon yet.
				if (be_roster->IsRunning(&item->entryRef))
					break;

				UnloadAddOn(&nodeRef, NULL, true, false);
			}
			break;
		}
	}
}


/*! The add-ons must support the exported C function API
	if they do, they will be loaded and added to deskbar
	primary function is the Instantiate function
*/
status_t
TReplicantTray::LoadAddOn(BEntry* entry, int32* id, bool addToSettings)
{
	if (entry == NULL)
		return B_BAD_VALUE;

	node_ref nodeRef;
	entry->GetNodeRef(&nodeRef);
	// no duplicates
	if (NodeExists(nodeRef))
		return B_ERROR;

	BNode node(entry);
	BPath path;
	status_t status = entry->GetPath(&path);
	if (status != B_OK)
		return status;

	// load the add-on
	image_id image = load_add_on(path.Path());
	if (image < B_OK)
		return image;

	// get the view loading function symbol
	//    we first look for a symbol that takes an image_id
	//    and entry_ref pointer, if not found, go with normal
	//    instantiate function
	BView* (*entryFunction)(image_id, const entry_ref*, float, float);
	BView* (*itemFunction)(float, float);
	BView* view = NULL;

	entry_ref ref;
	entry->GetRef(&ref);

	if (get_image_symbol(image, kInstantiateEntryCFunctionName,
			B_SYMBOL_TYPE_TEXT, (void**)&entryFunction) >= B_OK) {
		view = (*entryFunction)(image, &ref, fMaxReplicantWidth,
			fMaxReplicantHeight);
	} else if (get_image_symbol(image, kInstantiateItemCFunctionName,
			B_SYMBOL_TYPE_TEXT, (void**)&itemFunction) >= B_OK) {
		view = (*itemFunction)(fMaxReplicantWidth, fMaxReplicantHeight);
	} else {
		unload_add_on(image);
		return B_ERROR;
	}

	if (view == NULL || IconExists(view->Name())) {
		delete view;
		unload_add_on(image);
		return B_ERROR;
	}

	BMessage* data = new BMessage;
	view->Archive(data);
	delete view;

	// add the rep; adds info to list
	if (AddIcon(data, id, &ref) != B_OK)
		delete data;

	if (addToSettings) {
		fAddOnSettings.AddString(kReplicantPathField, path.Path());
		_SaveSettings();
	}

	return B_OK;
}


status_t
TReplicantTray::AddItem(int32 id, node_ref nodeRef, BEntry& entry, bool isAddOn)
{
	DeskbarItemInfo* item = new DeskbarItemInfo;
	if (item == NULL)
		return B_NO_MEMORY;

	item->id = id;
	item->isAddOn = isAddOn;

	if (entry.GetRef(&item->entryRef) != B_OK) {
		item->entryRef.device = -1;
		item->entryRef.directory = -1;
		item->entryRef.name = NULL;
	}
	item->nodeRef = nodeRef;

	fItemList->AddItem(item);

	if (isAddOn)
		watch_node(&nodeRef, B_WATCH_NAME | B_WATCH_ATTR, this, Window());

	return B_OK;
}


/**	from entry_removed message, when attribute removed
 *	or when a device is unmounted (use removeall, by device)
 */

void
TReplicantTray::UnloadAddOn(node_ref* nodeRef, dev_t* device, bool which,
	bool removeAll)
{
	for (int32 i = fItemList->CountItems() - 1; i >= 0; i--) {
		DeskbarItemInfo* item = (DeskbarItemInfo*)fItemList->ItemAt(i);
		if (item == NULL)
			continue;

		if ((which && nodeRef != NULL && item->nodeRef == *nodeRef)
			|| (device != NULL && item->nodeRef.device == *device)) {

			if (device != NULL && be_roster->IsRunning(&item->entryRef))
				continue;

			RemoveIcon(item->id);

			if (!removeAll)
				break;
		}
	}
}


void
TReplicantTray::RemoveItem(int32 id)
{
	DeskbarItemInfo* item = DeskbarItemFor(id);
	if (item == NULL)
		return;

	// attribute was added via Deskbar API (AddItem(entry_ref*, int32*)
	if (item->isAddOn) {
		BPath path(&item->entryRef);
		BString storedPath;
		for (int32 i = 0;
			fAddOnSettings.FindString(kReplicantPathField, i, &storedPath)
				== B_OK; i++) {
			if (storedPath == path.Path()) {
				fAddOnSettings.RemoveData(kReplicantPathField, i);
				break;
			}
		}
		_SaveSettings();

		BNode node(&item->entryRef);
		watch_node(&item->nodeRef, B_STOP_WATCHING, this, Window());
	}

	fItemList->RemoveItem(item);
	delete item;
}


/**	ENTRY_MOVED message, moving only occurs on a device
 *	copying will occur (ENTRY_CREATED) between devices
 */

void
TReplicantTray::MoveItem(entry_ref* ref, ino_t toDirectory)
{
	if (ref == NULL)
		return;

	// scan for a matching entry_ref and update it
	//
	// don't need to change node info as it does not change

	for (int32 i = fItemList->CountItems() - 1; i >= 0; i--) {
		DeskbarItemInfo* item = (DeskbarItemInfo*)fItemList->ItemAt(i);
		if (item == NULL)
			continue;

		if (strcmp(item->entryRef.name, ref->name) == 0
			&& item->entryRef.device == ref->device
			&& item->entryRef.directory == ref->directory) {
			item->entryRef.directory = toDirectory;
			break;
		}
	}
}

#endif // add-on support

//	external add-on API routines
//	called using the new BDeskbar class

//	existence of icon/replicant by name or ID
//	returns opposite
//	note: name and id are semi-private limiting
//		the ability of non-host apps to remove
//		icons without a little bit of work

/**	for a specific id
 *	return the name of the replicant (name of view)
 */

status_t
TReplicantTray::ItemInfo(int32 id, const char** name)
{
	if (id < 0)
		return B_BAD_VALUE;

	int32 index;
	int32 temp;
	BView* view = ViewAt(&index, &temp, id, false);
	if (view != NULL) {
		*name = view->Name();
		return B_OK;
	}

	return B_ERROR;
}


/**	for a specific name
 *	return the id (internal to Deskbar)
 */

status_t
TReplicantTray::ItemInfo(const char* name, int32* id)
{
	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	int32 index;
	BView* view = ViewAt(&index, id, name);

	return view != NULL ? B_OK : B_ERROR;
}


/**	at a specific index
 *	return both the name and the id of the replicant
 */

status_t
TReplicantTray::ItemInfo(int32 index, const char** name, int32* id)
{
	if (index < 0)
		return B_BAD_VALUE;

	BView* view;
	fShelf->ReplicantAt(index, &view, (uint32*)id, NULL);
	if (view != NULL) {
		*name = view->Name();
		return B_OK;
	}

	return B_ERROR;
}


/**	replicant exists, by id/index */

bool
TReplicantTray::IconExists(int32 target, bool byIndex)
{
	int32 index;
	int32 id;
	BView* view = ViewAt(&index, &id, target, byIndex);

	return view && index >= 0;
}


/**	replicant exists, by name */

bool
TReplicantTray::IconExists(const char* name)
{
	if (name == NULL || *name == '\0')
		return false;

	int32 index;
	int32 id;
	BView* view = ViewAt(&index, &id, name);

	return view != NULL && index >= 0;
}


int32
TReplicantTray::ReplicantCount() const
{
	return fShelf->CountReplicants();
}


/*! Message must contain an archivable view for later rehydration.
	This function takes over ownership of the provided message on success
	only.
	Returns the current replicant ID.
*/
status_t
TReplicantTray::AddIcon(BMessage* archive, int32* id, const entry_ref* addOn)
{
	if (archive == NULL || id == NULL)
		return B_BAD_VALUE;

	// find entry_ref

	entry_ref ref;
	if (addOn != NULL) {
		// Use it if we got it
		ref = *addOn;
	} else {
		const char* signature;

		status_t status = archive->FindString("add_on", &signature);
		if (status == B_OK) {
			BRoster roster;
			status = roster.FindApp(signature, &ref);
		}
		if (status != B_OK)
			return status;
	}

	BFile file;
	status_t status = file.SetTo(&ref, B_READ_ONLY);
	if (status != B_OK)
		return status;

	node_ref nodeRef;
	status = file.GetNodeRef(&nodeRef);
	if (status != B_OK)
		return status;

	BEntry entry(&ref, true);
		// TODO: this resolves an eventual link for the item being added - this
		// is okay for now, but in multi-user environments, one might want to
		// have links that carry the be:deskbar_item_status attribute
	status = entry.InitCheck();
	if (status != B_OK)
		return status;

	*id = 999;
	if (archive->what == B_ARCHIVED_OBJECT)
		archive->what = 0;

	BRect originalBounds = archive->FindRect("_frame");
		// this is a work-around for buggy replicants that change their size in
		// AttachedToWindow() (such as "SVM")

	// TODO: check for name collisions?
	status = fShelf->AddReplicant(archive, BPoint(1, 1));
	if (status != B_OK)
		return status;

	int32 count = ReplicantCount();
	BView* view;
	fShelf->ReplicantAt(count - 1, &view, (uint32*)id, NULL);

	if (view != NULL && originalBounds != view->Bounds()) {
		// The replicant changed its size when added to the window, so we need
		// to recompute all over again (it's already done once via
		// BShelf::AddReplicant() and TReplicantShelf::CanAcceptReplicantView())
		RealignReplicants();
	}

	float oldWidth = Bounds().Width();
	float oldHeight = Bounds().Height();
	float width, height;
	GetPreferredSize(&width, &height);
	if (oldWidth != width || oldHeight != height)
		AdjustPlacement();

	// add the item to the add-on list

	AddItem(*id, nodeRef, entry, addOn != NULL);
	return B_OK;
}


void
TReplicantTray::RemoveIcon(int32 target, bool byIndex)
{
	if (target < 0)
		return;

	int32 index;
	int32 id;
	BView* view = ViewAt(&index, &id, target, byIndex);
	if (view != NULL && index >= 0) {
		// remove the reference from the item list & the shelf
		RemoveItem(id);
		fShelf->DeleteReplicant(index);

		// force a placement update,  !! need to fix BShelf
		RealReplicantAdjustment(index);
	}
}


void
TReplicantTray::RemoveIcon(const char* name)
{
	if (name == NULL || *name == '\0')
		return;

	int32 index;
	int32 id;
	BView* view = ViewAt(&index, &id, name);
	if (view != NULL && index >= 0) {
		// remove the reference from the item list & shelf
		RemoveItem(id);
		fShelf->DeleteReplicant(index);

		// force a placement update,  !! need to fix BShelf
		RealReplicantAdjustment(index);
	}
}


void
TReplicantTray::RealReplicantAdjustment(int32 startIndex)
{
	if (startIndex < 0)
		return;

	if (startIndex == fLastReplicant)
		startIndex = 0;

	// reset the locations of all replicants after the one deleted
	RealignReplicants(startIndex);

	float oldWidth = Bounds().Width();
	float oldHeight = Bounds().Height();
	float width, height;
	GetPreferredSize(&width, &height);
	if (oldWidth != width || oldHeight != height) {
		// resize view to accomodate the replicants, redraw as necessary
		AdjustPlacement();
	}
}


/**	looking for a replicant by id/index
 *	return the view and index
 */

BView*
TReplicantTray::ViewAt(int32* index, int32* id, int32 target, bool byIndex)
{
	*index = -1;

	BView* view;
	if (byIndex) {
		if (fShelf->ReplicantAt(target, &view, (uint32*)id)) {
			if (view != NULL) {
				*index = target;

				return view;
			}
		}
	} else {
		int32 count = ReplicantCount() - 1;
		int32 localid;
		for (int32 repIndex = count; repIndex >= 0; repIndex--) {
			fShelf->ReplicantAt(repIndex, &view, (uint32*)&localid);
			if (localid == target && view != NULL) {
				*index = repIndex;
				*id = localid;

				return view;
			}
		}
	}

	return NULL;
}


/**	looking for a replicant with a view by name
 *	return the view, index and the id of the replicant
 */

BView*
TReplicantTray::ViewAt(int32* index, int32* id, const char* name)
{
	*index = -1;
	*id = -1;

	BView* view;
	int32 count = ReplicantCount() - 1;
	for (int32 repIndex = count; repIndex >= 0; repIndex--) {
		fShelf->ReplicantAt(repIndex, &view, (uint32*)id);
		if (view != NULL && view->Name() != NULL
			&& strcmp(name, view->Name()) == 0) {
			*index = repIndex;

			return view;
		}
	}

	return NULL;
}


/**	Shelf will call to determine where and if
 *	the replicant is to be added
 */

bool
TReplicantTray::AcceptAddon(BRect replicantFrame, BMessage* message)
{
	if (message == NULL)
		return false;

	if (replicantFrame.Height() > fMaxReplicantHeight)
		return false;

	alignment align = B_ALIGN_LEFT;
	if (fAlignmentSupport && message->HasBool("deskbar:dynamic_align")) {
		if (!fBarView->Vertical() && !fBarView->MiniState())
			align = B_ALIGN_RIGHT;
		else
			align = fBarView->Left() ? B_ALIGN_LEFT : B_ALIGN_RIGHT;
	} else if (message->HasInt32("deskbar:align"))
		message->FindInt32("deskbar:align", (int32*)&align);

	if (message->HasInt32("deskbar:private_align"))
		message->FindInt32("deskbar:private_align", (int32*)&align);
	else
		align = B_ALIGN_LEFT;

	BPoint loc = LocationForReplicant(ReplicantCount(),
		replicantFrame.Width());
	message->AddPoint("_pjp_loc", loc);

	return true;
}


/**	based on the previous (index - 1) replicant in the list
 *	calculate where the left point should be for this
 *	replicant.  replicant will flow to the right on its own
 */

BPoint
TReplicantTray::LocationForReplicant(int32 index, float replicantWidth)
{
	BPoint loc(kTrayPadding, 0);
	if (fBarView->Vertical() || fBarView->MiniState()) {
		if (fBarView->Vertical() && !fBarView->Left())
			loc.x += kDragWidth; // move past dragger on left

		loc.y = floorf((fBarView->TabHeight() - fMaxReplicantHeight) / 2) - 1;
	} else {
		loc.x -= 2; // keeps everything lined up nicely
		const int32 iconSize = static_cast<TBarApp*>(be_app)->IconSize();
		float yOffset = iconSize > B_MINI_ICON ? 3 : 2;
			// squeeze icons in there at 16x16, reduce border by 1px

		if (fBarView->Top()) {
			// align top
			loc.y = yOffset;
		} else {
			// align bottom
			loc.y = (fBarView->TeamMenuItemHeight() + 1)
				- fMaxReplicantHeight - yOffset;
		}
	}

	// move clock vertically centered in first row next to replicants
	fTime->MoveTo(Bounds().right - fTime->Bounds().Width() - kTrayPadding,
		loc.y + floorf((fMaxReplicantHeight - fTime->fHeight) / 2));

	if (fBarView->Vertical()) {
		// try to find free space in every row
		for (int32 row = 0; ; loc.y += fMaxReplicantHeight + kIconGap, row++) {
			// determine free space in this row
			BRect rowRect(loc.x, loc.y,
				loc.x + static_cast<TBarApp*>(be_app)->Settings()->width
					- (kTrayPadding + kDragWidth + kGutter) * 2,
				loc.y + fMaxReplicantHeight);
			if (row == 0 && !fTime->IsHidden(fTime))
				rowRect.right -= kClockMargin + fTime->Frame().Width();

			BRect replicantRect = rowRect;
			for (int32 i = 0; i < index; i++) {
				BView* view = NULL;
				fShelf->ReplicantAt(i, &view);
				if (view == NULL || view->Frame().top != rowRect.top)
					continue;

				// push this replicant placement past the last one
				replicantRect.left = view->Frame().right + kIconGap + 1;
			}

			// calculated left position, add replicantWidth to get the
			// right position
			replicantRect.right = replicantRect.left + replicantWidth;

			// check if replicant fits in this row
			if (replicantRect.right < rowRect.right) {
				// replicant fits in this row
				loc = replicantRect.LeftTop();
				break;
			}

			// check next row
		}
	} else {
		// horizontal
		if (index > 0) {
			// get the last replicant added for placement reference
			BView* view = NULL;
			fShelf->ReplicantAt(index - 1, &view);
			if (view != NULL) {
				// push this replicant placement past the last one
				loc.x = view->Frame().right + kIconGap + 1;
			}
		}
	}

	if (loc.y > fRightBottomReplicant.top
		|| (loc.y == fRightBottomReplicant.top
			&& loc.x > fRightBottomReplicant.left)) {
		fRightBottomReplicant.Set(loc.x, loc.y, loc.x + replicantWidth,
			loc.y + fMaxReplicantHeight);
		fLastReplicant = index;
	}

	return loc;
}


BRect
TReplicantTray::IconFrame(int32 target, bool byIndex)
{
	int32 index;
	int32 id;
	BView* view = ViewAt(&index, &id, target, byIndex);

	return view != NULL ? view->Frame() : BRect(0, 0, 0, 0);
}


BRect
TReplicantTray::IconFrame(const char* name)
{
	if (name == NULL)
		return BRect(0, 0, 0, 0);

	int32 index;
	int32 id;
	BView* view = ViewAt(&index, &id, name);

	return view != NULL ? view->Frame() : BRect(0, 0, 0, 0);
}


/**	Scan from the startIndex and reset the location
 *	as defined in LocationForReplicant()
 */

void
TReplicantTray::RealignReplicants(int32 startIndex)
{
	if (startIndex < 0)
		startIndex = 0;

	int32 replicantCount = ReplicantCount();
	if (replicantCount <= 0)
		return;

	if (startIndex == 0)
		fRightBottomReplicant.Set(0, 0, 0, 0);

	BView* view = NULL;
	for (int32 index = startIndex; index < replicantCount; index++) {
		fShelf->ReplicantAt(index, &view);
		if (view == NULL)
			continue;

		float replicantWidth = view->Frame().Width();
		BPoint loc = LocationForReplicant(index, replicantWidth);
		if (view->Frame().LeftTop() != loc)
			view->MoveTo(loc);
	}
}


status_t
TReplicantTray::_SaveSettings()
{
	status_t result;
	BPath path;
	if ((result = GetDeskbarSettingsDirectory(path, true)) == B_OK) {
		path.Append(kReplicantSettingsFile);

		BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		if ((result = file.InitCheck()) == B_OK)
			result = fAddOnSettings.Flatten(&file);
	}

	return result;
}


void
TReplicantTray::SaveTimeSettings()
{
	if (fTime == NULL)
		return;

	clock_settings* settings = ((TBarApp*)be_app)->ClockSettings();
	settings->showSeconds = fTime->ShowSeconds();
	settings->showDayOfWeek = fTime->ShowDayOfWeek();
	settings->showTimeZone = fTime->ShowTimeZone();
}


//	#pragma mark - TDragRegion


/*! Draggable region that is asynchronous so that dragging does not block
	other activities.
*/
TDragRegion::TDragRegion(TBarView* barView, BView* replicantTray)
	:
	BControl(BRect(0, 0, 0, 0), "", "", NULL, B_FOLLOW_NONE,
		B_WILL_DRAW | B_DRAW_ON_CHILDREN | B_FRAME_EVENTS),
	fBarView(barView),
	fReplicantTray(replicantTray),
	fDragLocation(kAutoPlaceDragRegion)
{
}


void
TDragRegion::AttachedToWindow()
{
	BView::AttachedToWindow();

	CalculateRegions();

	if (be_control_look != NULL)
		SetViewUIColor(B_MENU_BACKGROUND_COLOR, 1.1);
	else
		SetViewUIColor(B_MENU_BACKGROUND_COLOR);

	ResizeToPreferred();
}


void
TDragRegion::GetPreferredSize(float* width, float* height)
{
	fReplicantTray->ResizeToPreferred();
	*width = fReplicantTray->Bounds().Width();
	*height = fReplicantTray->Bounds().Height();

	if (fDragLocation != kNoDragRegion)
		*width += kDragWidth + kGutter;
	else
		*width += 6;

	if (fBarView->Vertical() && !fBarView->MiniState())
		*height += 3; // add a pixel for an extra border on top
	else
		*height += 2; // all other modes have a 1px border on top and bottom
}


void
TDragRegion::Draw(BRect updateRect)
{
	rgb_color menuColor = ViewColor();
	rgb_color hilite = tint_color(menuColor, B_DARKEN_1_TINT);
	rgb_color ldark = tint_color(menuColor, 1.02);
	rgb_color dark = tint_color(menuColor, B_DARKEN_2_TINT);

	BRect frame(Bounds());
	BeginLineArray(4);

	if (fBarView->Vertical()) {
		// vertical expando full or mini state, draw 2 lines at the top
		AddLine(frame.LeftTop(), frame.RightTop(), dark);
		AddLine(BPoint(frame.left, frame.top + 1),
			BPoint(frame.right, frame.top + 1), ldark);
		// add hilight along bottom
		AddLine(BPoint(frame.left + 1, frame.bottom),
			BPoint(frame.right - 1, frame.bottom), hilite);
	} else {
		// mini-mode or horizontal, draw hilight along top left and bottom
		AddLine(frame.LeftTop(), frame.RightTop(), hilite);
		AddLine(BPoint(frame.left, frame.top + 1), frame.LeftBottom(), hilite);
		if (!fBarView->Vertical()) {
			// only draw bottom hilight in horizontal mode
			AddLine(BPoint(frame.left + 1, frame.bottom - 3),
				BPoint(frame.right - 1, frame.bottom - 3), hilite);
		}
	}

	EndLineArray();
}


void
TDragRegion::DrawAfterChildren(BRect updateRect)
{
	if (fDragLocation != kDontDrawDragRegion || fDragLocation != kNoDragRegion)
		DrawDragger();
}


void
TDragRegion::DrawDragger()
{
	BRect dragRegion(DragRegion());

	rgb_color menuColor = ViewColor();
	rgb_color menuHilite = menuColor;
	if (IsTracking()) {
		// draw drag region highlighted if tracking mouse
		menuHilite = tint_color(menuColor, B_HIGHLIGHT_BACKGROUND_TINT);
		SetHighColor(menuHilite);
		FillRect(dragRegion.InsetByCopy(0, -1));
	} else {
		SetHighColor(menuColor);
		FillRect(dragRegion.InsetByCopy(0, 1));
	}

	rgb_color vdark = tint_color(menuHilite, B_DARKEN_3_TINT);
	rgb_color light = tint_color(menuHilite, B_LIGHTEN_2_TINT);

	rgb_color dark = tint_color(menuHilite, B_DARKEN_2_TINT);

	BeginLineArray(dragRegion.IntegerHeight() + 2);
	BPoint where;
	where.x = floorf((dragRegion.left + dragRegion.right) / 2 + 0.5) - 1;
	where.y = dragRegion.top + 2;

	while (where.y + 1 <= dragRegion.bottom - 2) {
		AddLine(where, where, vdark);
		AddLine(where + BPoint(1, 1), where + BPoint(1, 1), light);

		where.y += 3;
	}

	if (fBarView != NULL && fBarView->Vertical() && fBarView->MiniState()
		&& !fBarView->Top()) {
		// extend bottom border in bottom mini-mode
		AddLine(BPoint(dragRegion.left, dragRegion.bottom - 2),
			BPoint(dragRegion.right, dragRegion.bottom - 2),
				IsTracking() ? menuHilite : dark);
	}

	EndLineArray();
}


BRect
TDragRegion::DragRegion() const
{
	BRect dragRegion(Bounds());

	bool placeOnLeft = false;
	if (fDragLocation == kAutoPlaceDragRegion) {
		placeOnLeft = fBarView->Left()
			&& (fBarView->Vertical() || fBarView->MiniState());
	} else
		placeOnLeft = fDragLocation == kDragRegionLeft;

	if (placeOnLeft)
		dragRegion.right = dragRegion.left + kDragWidth;
	else
		dragRegion.left = dragRegion.right - kDragWidth;

	return dragRegion;
}


void
TDragRegion::MouseDown(BPoint where)
{
	uint32 buttons;
	BPoint mouseLoc;

	BRect dragRegion(DragRegion());
	dragRegion.InsetBy(-2, -2);
		// DragRegion() is designed for drawing, not clicking

	if (!dragRegion.Contains(where))
		return;

	while (true) {
		GetMouse(&mouseLoc, &buttons);
		if (buttons == 0)
			break;

		if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
			fPreviousPosition = where;
			SetTracking(true);
			SetMouseEventMask(B_POINTER_EVENTS,
				B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
			Invalidate(DragRegion());
			break;
		}

		snooze(25000);
	}
}


void
TDragRegion::MouseUp(BPoint where)
{
	if (IsTracking()) {
		SetTracking(false);
		Invalidate(DragRegion());
	} else
		BControl::MouseUp(where);
}


bool
TDragRegion::SwitchModeForRegion(BPoint where, BRegion region,
	bool newVertical, bool newLeft, bool newTop, int32 newState)
{
	if (!region.Contains(where)) {
		// not our region
		return false;
	}

	if (newVertical == fBarView->Vertical() && newLeft == fBarView->Left()
		&& newTop == fBarView->Top() && newState == fBarView->State()) {
		// already in the correct mode
		return true;
	}

	fBarView->ChangeState(newState, newVertical, newLeft, newTop, true);

	return true;
}


void
TDragRegion::CalculateRegions()
{
	const BRect screenFrame((BScreen(Window())).Frame());

	float menuBarHeight = fBarView->BarMenuBar()->Frame().Height();
	float hDivider = floorf(screenFrame.Width() / 4);
	float halfScreen = floorf(screenFrame.Height() / 2);

	// corners
	fTopLeftVertical.Set(BRect(screenFrame.left,
		screenFrame.top + menuBarHeight, screenFrame.left + hDivider,
		screenFrame.top + floorf(menuBarHeight * kVerticalMiniMultiplier)));
	fTopRightVertical.Set(BRect(screenFrame.right - hDivider,
		screenFrame.top + menuBarHeight, screenFrame.right,
		screenFrame.top + floorf(menuBarHeight * kVerticalMiniMultiplier)));
	fBottomLeftVertical.Set(BRect(screenFrame.left,
		screenFrame.bottom - floorf(menuBarHeight * kVerticalMiniMultiplier),
		screenFrame.left + hDivider, screenFrame.bottom - menuBarHeight));
	fBottomRightVertical.Set(BRect(screenFrame.right - hDivider,
		screenFrame.bottom - floorf(menuBarHeight * kVerticalMiniMultiplier),
		screenFrame.right, screenFrame.bottom - menuBarHeight));

	fTopLeftHorizontal.Set(BRect(screenFrame.left, screenFrame.top,
		screenFrame.left + hDivider, screenFrame.top + menuBarHeight));
	fTopRightHorizontal.Set(BRect(screenFrame.right - hDivider, screenFrame.top,
		screenFrame.right, screenFrame.top + menuBarHeight));
	fBottomLeftHorizontal.Set(BRect(screenFrame.left, screenFrame.bottom - menuBarHeight,
		screenFrame.left + hDivider, screenFrame.bottom));
	fBottomRightHorizontal.Set(BRect(screenFrame.right - hDivider,
		screenFrame.bottom - menuBarHeight, screenFrame.right,
		screenFrame.bottom));

	// left/right expando
	fMiddleLeft.Set(BRect(screenFrame.left, screenFrame.top,
		screenFrame.left + hDivider, screenFrame.bottom));
	fMiddleLeft.Exclude(&fTopLeftHorizontal);
	fMiddleLeft.Exclude(&fBottomLeftHorizontal);
	fMiddleLeft.Exclude(&fTopLeftVertical);
	fMiddleLeft.Exclude(&fBottomLeftVertical);

	fMiddleRight.Set(BRect(screenFrame.right - hDivider,
		screenFrame.top, screenFrame.right, screenFrame.bottom));
	fMiddleRight.Exclude(&fTopRightHorizontal);
	fMiddleRight.Exclude(&fBottomRightHorizontal);
	fMiddleRight.Exclude(&fTopRightVertical);
	fMiddleRight.Exclude(&fBottomRightVertical);

#ifdef FULL_MODE
	// left/right full
	fLeftSide.Set(BRect(screenFrame.left, screenFrame.bottom - halfScreen,
		screenFrame.left + hDivider, screenFrame.bottom));
	fLeftSide.Exclude(&fBottomLeftHorizontal);
	fLeftSide.Exclude(&fBottomLeftVertical);
	fMiddleLeft.Exclude(&fLeftSide);

	fRightSide.Set(BRect(screenFrame.right - hDivider,
		screenFrame.bottom - halfScreen, screenFrame.right,
		screenFrame.bottom));
	fRightSide.Exclude(&fBottomRightHorizontal);
	fRightSide.Exclude(&fBottomRightVertical);
	fMiddleRight.Exclude(&fRightSide);
#endif

	// top/bottom
	BRect leftSideRect(screenFrame.left, screenFrame.top,
		screenFrame.left + hDivider, screenFrame.bottom);
	BRect rightSideRect(screenFrame.right - hDivider, screenFrame.top,
		screenFrame.right, screenFrame.bottom);

	fTopHalf.Set(BRect(screenFrame.left, screenFrame.top, screenFrame.right,
		screenFrame.top + halfScreen));
	fTopHalf.Exclude(leftSideRect);
	fTopHalf.Exclude(rightSideRect);

	fBottomHalf.Set(BRect(screenFrame.left, screenFrame.bottom - halfScreen,
		screenFrame.right, screenFrame.bottom));
	fBottomHalf.Exclude(leftSideRect);
	fBottomHalf.Exclude(rightSideRect);
}


void
TDragRegion::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (!IsTracking() || where == fPreviousPosition)
		return BControl::MouseMoved(where, transit, dragMessage);

	fPreviousPosition = where;

	// TODO: can't trust the passed in where param, get screen_where from
	// Window()->CurrentMessage() instead, why is this necessary?
	BPoint whereScreen;
	BMessage* currentMessage = Window()->CurrentMessage();
	if (currentMessage == NULL || currentMessage->FindPoint("screen_where",
			&whereScreen) != B_OK) {
		whereScreen = ConvertToScreen(where);
	}

	// use short circuit evaluation for convenience
	if (// vertical mini
		   SwitchModeForRegion(whereScreen, fTopLeftVertical, true,
			true, true, kMiniState)
		|| SwitchModeForRegion(whereScreen, fTopRightVertical, true,
			false, true, kMiniState)
		|| SwitchModeForRegion(whereScreen, fBottomLeftVertical, true,
			true, false, kMiniState)
		|| SwitchModeForRegion(whereScreen, fBottomRightVertical, true,
			false, false, kMiniState)
		// horizontal mini
		|| SwitchModeForRegion(whereScreen, fTopLeftHorizontal, false,
			true, true, kMiniState)
		|| SwitchModeForRegion(whereScreen, fTopRightHorizontal, false,
			false, true, kMiniState)
		|| SwitchModeForRegion(whereScreen, fBottomLeftHorizontal, false,
			true, false, kMiniState)
		|| SwitchModeForRegion(whereScreen, fBottomRightHorizontal, false,
			false, false, kMiniState)
		// expando
		|| SwitchModeForRegion(whereScreen, fMiddleLeft, true, true, true,
			kExpandoState)
		|| SwitchModeForRegion(whereScreen, fMiddleRight, true, false, true,
			kExpandoState)
#ifdef FULL_MODE
		// full
		|| SwitchModeForRegion(whereScreen, fLeftSide, true, true, true,
			kFullState)
		|| SwitchModeForRegion(whereScreen, fRightSide, true, false, true,
			kFullState)
#endif
		// horizontal
		|| SwitchModeForRegion(whereScreen, fTopHalf, false, true, true,
			kExpandoState)
		|| SwitchModeForRegion(whereScreen, fBottomHalf, false, true, false,
			kExpandoState)
	);
}


int32
TDragRegion::DragRegionLocation() const
{
	return fDragLocation;
}


void
TDragRegion::SetDragRegionLocation(int32 location)
{
	if (location == fDragLocation)
		return;

	fDragLocation = location;
	Invalidate();
}


//	#pragma mark - TResizeControl


/*! Draggable region that is asynchronous so that resizing does not block.
*/
TResizeControl::TResizeControl(TBarView* barView)
	:
	BControl(BRect(0, kDragWidth, 0, kMenuBarHeight), "", "", NULL,
		B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
	fBarView(barView)
{
}


TResizeControl::~TResizeControl()
{
}


void
TResizeControl::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (be_control_look != NULL)
		SetViewUIColor(B_MENU_BACKGROUND_COLOR, 1.1);
	else
		SetViewUIColor(B_MENU_BACKGROUND_COLOR);
}


void
TResizeControl::Draw(BRect updateRect)
{
	if (!fBarView->Vertical())
		return;

	BRect dragRegion(Bounds());

	int32 height = dragRegion.IntegerHeight();
	if (height <= 0)
		return;

	rgb_color menuColor = ViewColor();
	rgb_color menuHilite = menuColor;
	if (IsTracking()) {
		// draw drag region highlighted if tracking mouse
		menuHilite = tint_color(menuColor, B_HIGHLIGHT_BACKGROUND_TINT);
		SetHighColor(menuHilite);
		FillRect(dragRegion);
	} else {
		SetHighColor(menuColor);
		FillRect(dragRegion);
	}

	rgb_color vdark = tint_color(menuHilite, B_DARKEN_3_TINT);
	rgb_color light = tint_color(menuHilite, B_LIGHTEN_2_TINT);

	BeginLineArray(height);
	BPoint where;
	where.x = floorf((dragRegion.left + dragRegion.right) / 2 + 0.5) - 1;
	where.y = dragRegion.top + 2;

	while (where.y + 1 <= dragRegion.bottom) {
		AddLine(where, where, vdark);
		AddLine(where + BPoint(1, 1), where + BPoint(1, 1), light);

		where.y += 3;
	}
	EndLineArray();
}


void
TResizeControl::MouseDown(BPoint where)
{
	uint32 buttons;
	BPoint mouseLoc;

	while (true) {
		GetMouse(&mouseLoc, &buttons);
		if (buttons == 0)
			break;

		if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
			SetTracking(true);
			SetMouseEventMask(B_POINTER_EVENTS,
				B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
			Invalidate();
			break;
		}

		snooze(25000);
	}
}


void
TResizeControl::MouseUp(BPoint where)
{
	if (IsTracking()) {
		SetTracking(false);
		Invalidate();
	} else
		BControl::MouseUp(where);
}


void
TResizeControl::MouseMoved(BPoint where, uint32 code,
	const BMessage* dragMessage)
{
	if (!fBarView->Vertical() || !IsResizing())
		return BControl::MouseMoved(where, code, dragMessage);

	float windowWidth = Window()->Frame().Width();
	float delta = 0;
	BPoint whereScreen = ConvertToScreen(where);

	if (fBarView->Left()) {
		delta = whereScreen.x - Window()->Frame().right;
		if (delta > 0 && windowWidth >= gMaximumWindowWidth)
			; // do nothing
		else if (delta < 0 && windowWidth <= gMinimumWindowWidth)
			; // do nothing
		else
			Window()->ResizeBy(delta, 0);
	} else {
		delta = Window()->Frame().left - whereScreen.x;
		if (delta > 0 && windowWidth >= gMaximumWindowWidth)
			; // do nothing
		else if (delta < 0 && windowWidth <= gMinimumWindowWidth)
			; // do nothing
		else {
			Window()->MoveBy(delta, 0);
			Window()->ResizeBy(delta, 0);
		}
	}

	windowWidth = Window()->Frame().Width();

	BControl::MouseMoved(where, code, dragMessage);
}
