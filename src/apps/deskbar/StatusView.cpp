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
#include <Directory.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <MenuItem.h>
#include <MutableLocaleRoster.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#include "icons_logo.h"
#include "BarApp.h"
#include "DeskbarUtils.h"
#include "ResourceSet.h"
#include "StatusView.h"
#include "StatusViewShelf.h"
#include "TimeView.h"

using std::max;

#ifdef DB_ADDONS
// Add-on support
//
// Item - internal item list (node, eref, etc)
// Icon - physical replicant handed to the DeskbarClass class
// AddOn - attribute based add-on

const char* const kInstantiateItemCFunctionName = "instantiate_deskbar_item";
const char* const kInstantiateEntryCFunctionName = "instantiate_deskbar_entry";
const char* const kReplicantSettingsFile = "Deskbar_replicants";
const char* const kReplicantPathField = "replicant_path";

float sMinimumWindowWidth = kGutter + kMinimumTrayWidth + kDragRegionWidth;


static void
DumpItem(DeskbarItemInfo* item)
{
	printf("is addon: %i, id: %li\n", item->isAddOn, item->id);
	printf("entry_ref:  %ld, %Ld, %s\n", item->entryRef.device,
		item->entryRef.directory, item->entryRef.name);
	printf("node_ref:  %ld, %Ld\n", item->nodeRef.device, item->nodeRef.node);
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

TReplicantTray::TReplicantTray(TBarView* parent, bool vertical)
	: BView(BRect(0, 0, 1, 1), "Status", B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_FRAME_EVENTS),
	fTime(NULL),
	fBarView(parent),
	fShelf(new TReplicantShelf(this)),
	fMultiRowMode(vertical),
	fMinimumTrayWidth(kMinimumTrayWidth),
	fAlignmentSupport(false)
{
	// init the minimum window width according to the logo.
	const BBitmap* logoBitmap = AppResSet()->FindBitmap(B_MESSAGE_TYPE,
		R_LeafLogoBitmap);
	if (logoBitmap != NULL) {
		sMinimumWindowWidth = max_c(sMinimumWindowWidth,
			2 * (logoBitmap->Bounds().Width() + 8));
		fMinimumTrayWidth = sMinimumWindowWidth - kGutter - kDragRegionWidth;
	}

	BFormattingConventions conventions;
	BLocale::Default()->GetFormattingConventions(&conventions);
	bool use24HourClock = conventions.Use24HourClock();
	desk_settings* settings = ((TBarApp*)be_app)->Settings();

	// Create the time view
	fTime = new TTimeView(fMinimumTrayWidth, kMaxReplicantHeight - 1.0,
		use24HourClock, settings->showSeconds, settings->showDayOfWeek,
		settings->showTimeZone);
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
		SetViewColor(Parent()->ViewColor());
	} else {
		SetViewColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_DARKEN_1_TINT));
	}
	SetDrawingMode(B_OP_COPY);

	Window()->SetPulseRate(1000000);

	AddChild(fTime);
	fTime->MoveTo(Bounds().right - fTime->Bounds().Width() - 1, 2);
	if (!((TBarApp*)be_app)->Settings()->showTime) {
		fTime->Hide();
		RealignReplicants();
		AdjustPlacement();
	}

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
	float width = 0, height = kMinimumTrayHeight;

	if (fMultiRowMode) {
		if (fShelf->CountReplicants() > 0)
			height = fRightBottomReplicant.bottom;

		// the height will be uniform for the number of rows necessary to show
		// all the reps + any gutters necessary for spacing
		int32 rowCount = (int32)(height / kMaxReplicantHeight);
		height = kGutter + (rowCount * kMaxReplicantHeight)
			+ ((rowCount - 1) * kIconGap) + kGutter;
		height = max(kMinimumTrayHeight, height);
		width = fMinimumTrayWidth;
	} else {
		// if last replicant overruns clock then resize to accomodate
		if (fShelf->CountReplicants() > 0) {
			if (!fTime->IsHidden() && fTime->Frame().left
				< fRightBottomReplicant.right + 6) {
				width = fRightBottomReplicant.right + 6
					+ fTime->Frame().Width();
			} else
				width = fRightBottomReplicant.right + 3;
		}

		// this view has a fixed minimum width
		width = max(fMinimumTrayWidth, width);
		height = kGutter + static_cast<TBarApp*>(be_app)->IconSize() + kGutter;
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
		case kShowHideTime:
			// from context menu in clock and in this view
			ShowHideTime();
			break;

		case kTimeIntervalChanged:
		{
			if (fTime == NULL)
				return;

			bool use24HourClock;
			if (message->FindBool("use24HourClock", &use24HourClock)
				== B_OK) {
				BFormattingConventions conventions;
				BLocale::Default()->GetFormattingConventions(&conventions);
				conventions.SetExplicitUse24HourClock(use24HourClock);
				BPrivate::MutableLocaleRoster::Default()->
					SetDefaultFormattingConventions(conventions);
				fTime->SetUse24HourClock(use24HourClock);
				// time string reformat -> realign
				RealignReplicants();
				AdjustPlacement();
			}

			break;
		}

		case kShowSeconds:
			if (fTime == NULL)
				return;

			fTime->SetShowSeconds(!fTime->ShowSeconds());

			// time string reformat -> realign
			RealignReplicants();
			AdjustPlacement();
			break;

		case kShowDayOfWeek:
			if (fTime == NULL)
				return;

			fTime->SetShowDayOfWeek(!fTime->ShowDayOfWeek());

			// time string reformat -> realign
			RealignReplicants();
			AdjustPlacement();
			break;

		case kShowTimeZone:
			if (fTime == NULL)
				return;

			fTime->SetShowTimeZone(!fTime->ShowTimeZone());

			// time string reformat -> realign
			RealignReplicants();
			AdjustPlacement();
			break;

#ifdef DB_ADDONS
		case B_NODE_MONITOR:
			HandleEntryUpdate(message);
			break;
#endif

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

	// If clock is visible show the extended menu, otherwise show "Show time"

	if (!fTime->IsHidden())
		fTime->ShowTimeOptions(ConvertToScreen(point));
	else {
		BMenuItem* item = new BMenuItem(B_TRANSLATE("Show time"),
			new BMessage(kShowHideTime));
		menu->AddItem(item);
		menu->SetTargetForItems(this);
		BPoint where = ConvertToScreen(point);
		menu->Go(where, true, true, BRect(where - BPoint(4, 4),
			where + BPoint(4, 4)), true);
	}
}


void
TReplicantTray::SetMultiRow(bool state)
{
	fMultiRowMode = state;
}


void
TReplicantTray::ShowHideTime()
{
	if (fTime == NULL)
		return;

	if (fTime->IsHidden())
		fTime->Show();
	else
		fTime->Hide();

	RealignReplicants();
	AdjustPlacement();
}


#ifdef DB_ADDONS


void
TReplicantTray::InitAddOnSupport()
{
	// list to maintain refs to each rep added/deleted
	fItemList = new BList();
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
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
	
	for (int32 i = fItemList->CountItems(); i-- > 0 ;) {
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
	for (int32 i = fItemList->CountItems(); i-- > 0 ;) {
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
	for (int32 i = fItemList->CountItems(); i-- > 0 ;) {
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

				if (!name)
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
	if (!entry)
		return B_ERROR;

	node_ref nodeRef;
	entry->GetNodeRef(&nodeRef);
	// no duplicates
	if (NodeExists(nodeRef))
		return B_ERROR;

	BNode node(entry);
	BPath path;
	status_t status = entry->GetPath(&path);
	if (status < B_OK)
		return status;

	// load the add-on
	image_id image = load_add_on(path.Path());
	if (image < B_OK)
		return image;

	// get the view loading function symbol
	//    we first look for a symbol that takes an image_id
	//    and entry_ref pointer, if not found, go with normal
	//    instantiate function
	BView* (*entryFunction)(image_id, const entry_ref*);
	BView* (*itemFunction)(void);
	BView* view = NULL;

	entry_ref ref;
	entry->GetRef(&ref);

	if (get_image_symbol(image, kInstantiateEntryCFunctionName,
			B_SYMBOL_TYPE_TEXT, (void**)&entryFunction) >= B_OK) {
		view = (*entryFunction)(image, &ref);
	} else if (get_image_symbol(image, kInstantiateItemCFunctionName,
			B_SYMBOL_TYPE_TEXT, (void**)&itemFunction) >= B_OK) {
		view = (*itemFunction)();
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

	AddIcon(data, id, &ref);
		// add the rep; adds info to list

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

	if (entry.GetRef(&item->entryRef) < B_OK) {
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
TReplicantTray::UnloadAddOn(node_ref* nodeRef, dev_t* device,
	bool which, bool removeAll)
{
	for (int32 i = fItemList->CountItems(); i-- > 0 ;) {
		DeskbarItemInfo* item = (DeskbarItemInfo*)fItemList->ItemAt(i);
		if (!item)
			continue;

		if ((which && nodeRef && item->nodeRef == *nodeRef)
			|| (device && item->nodeRef.device == *device)) {

			if (device && be_roster->IsRunning(&item->entryRef))
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
	if (!ref)
		return;

	// scan for a matching entry_ref and update it
	//
	// don't need to change node info as it does not change

	for (int32 i = fItemList->CountItems(); i-- > 0 ;) {
		DeskbarItemInfo* item = (DeskbarItemInfo*)fItemList->ItemAt(i);
		if (!item)
			continue;

		if (!strcmp(item->entryRef.name, ref->name)
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
		return B_ERROR;

	int32 index, temp;
	BView* view = ViewAt(&index, &temp, id, false);
	if (view) {
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
	if (!name || strlen(name) <= 0)
		return B_ERROR;

	int32 index;
	BView* view = ViewAt(&index, id, name);
	if (view)
		return B_OK;

	return B_ERROR;
}


/**	at a specific index
 *	return both the name and the id of the replicant
 */

status_t
TReplicantTray::ItemInfo(int32 index, const char** name, int32* id)
{
	if (index < 0)
		return B_ERROR;

	BView* view;
	fShelf->ReplicantAt(index, &view, (uint32*)id, NULL);
	if (view) {
		*name = view->Name();
		return B_OK;
	}

	return B_ERROR;
}


/**	replicant exists, by id/index */

bool
TReplicantTray::IconExists(int32 target, bool byIndex)
{
	int32 index, id;
	BView* view = ViewAt(&index, &id, target, byIndex);

	return view && index >= 0;
}


/**	replicant exists, by name */

bool
TReplicantTray::IconExists(const char* name)
{
	if (!name || strlen(name) == 0)
		return false;

	int32 index, id;
	BView* view = ViewAt(&index, &id, name);

	return view && index >= 0;
}


int32
TReplicantTray::IconCount() const
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
		return B_ERROR;

	// find entry_ref

	entry_ref ref;
	if (addOn) {
		// Use it if we got it
		ref = *addOn;
	} else {
		const char* signature;

		status_t status = archive->FindString("add_on", &signature);
		if (status == B_OK) {
			BRoster roster;
			status = roster.FindApp(signature, &ref);
		}
		if (status < B_OK)
			return status;
	}

	BFile file;
	status_t status = file.SetTo(&ref, B_READ_ONLY);
	if (status < B_OK)
		return status;

	node_ref nodeRef;
	status = file.GetNodeRef(&nodeRef);
	if (status < B_OK)
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

	int32 count = fShelf->CountReplicants();
	BView* view;
	fShelf->ReplicantAt(count - 1, &view, (uint32*)id, NULL);

	if (originalBounds != view->Bounds()) {
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

	int32 index, id;
	BView* view = ViewAt(&index, &id, target, byIndex);
	if (view && index >= 0) {
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
	if (!name || strlen(name) <= 0)
		return;

	int32 id, index;
	BView* view = ViewAt(&index, &id, name);
	if (view && index >= 0) {
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
			if (view) {
				*index = target;
				return view;
			}
		}
	} else {
		int32 count = fShelf->CountReplicants() - 1;
		int32 localid;
		for (int32 repIndex = count ; repIndex >= 0 ; repIndex--) {
			fShelf->ReplicantAt(repIndex, &view, (uint32*)&localid);
			if (localid == target && view) {
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
	int32 count = fShelf->CountReplicants()-1;
	for (int32 repIndex = count ; repIndex >= 0 ; repIndex--) {
		fShelf->ReplicantAt(repIndex, &view, (uint32*)id);
		if (view && view->Name() && strcmp(name, view->Name()) == 0) {
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
	if (!message)
		return false;

	if (replicantFrame.Height() > kMaxReplicantHeight)
		return false;

	alignment align = B_ALIGN_LEFT;
	if (fAlignmentSupport && message->HasBool("deskbar:dynamic_align")) {
		if (!fBarView->Vertical())
			align = B_ALIGN_RIGHT;
		else
			align = fBarView->Left() ? B_ALIGN_LEFT : B_ALIGN_RIGHT;
	} else if (message->HasInt32("deskbar:align"))
		message->FindInt32("deskbar:align", (int32*)&align);

	if (message->HasInt32("deskbar:private_align"))
		message->FindInt32("deskbar:private_align", (int32*)&align);
	else
		align = B_ALIGN_LEFT;

	BPoint loc = LocationForReplicant(fShelf->CountReplicants(),
		replicantFrame.Width());

	message->AddPoint("_pjp_loc", loc);
	return true;
}


/**	based on the previous (index - 1) replicant in the list
 *	calculate where the left point should be for this
 *	replicant.  replicant will flow to the right on its own
 */

BPoint
TReplicantTray::LocationForReplicant(int32 index, float width)
{
	BPoint loc(kIconGap + 1, kGutter + 1);

	if (fMultiRowMode) {
		// try to find free space in every row
		for (int32 row = 0; ; loc.y += kMaxReplicantHeight + kIconGap, row++) {
			// determine free space in this row
			BRect rect(loc.x, loc.y, loc.x + fMinimumTrayWidth - kIconGap
				- 2.0, loc.y + kMaxReplicantHeight);
			if (row == 0 && !fTime->IsHidden())
				rect.right -= fTime->Frame().Width() + kIconGap;

			for (int32 i = 0; i < index; i++) {
				BView* view = NULL;
				fShelf->ReplicantAt(i, &view);
				if (view == NULL || view->Frame().top != rect.top)
					continue;

				rect.left = view->Frame().right + kIconGap + 1;
			}

			if (rect.Width() >= width) {
				// the icon fits in this row
				loc = rect.LeftTop();
				break;
			}
		}
	} else {
		if (index > 0) {
			// get the last replicant added for placement reference
			BView* view = NULL;
			fShelf->ReplicantAt(index - 1, &view);
			if (view) {
				// push this rep placement past the last one
				loc.x = view->Frame().right + kIconGap + 1;
				loc.y = view->Frame().top;
			}
		}
	}

	if ((loc.y == fRightBottomReplicant.top && loc.x
		> fRightBottomReplicant.left) || loc.y > fRightBottomReplicant.top) {
		fRightBottomReplicant.Set(loc.x, loc.y, loc.x + width, loc.y
		+ kMaxReplicantHeight);
		fLastReplicant = index;
	}

	return loc;
}


BRect
TReplicantTray::IconFrame(int32 target, bool byIndex)
{
	int32 index, id;
	BView* view = ViewAt(&index, &id, target, byIndex);
	if (view)
		return view->Frame();

	return BRect(0, 0, 0, 0);
}


BRect
TReplicantTray::IconFrame(const char* name)
{
	if (!name)
		return BRect(0, 0, 0, 0);

	int32 id, index;
	BView* view = ViewAt(&index, &id, name);
	if (view)
		return view->Frame();

	return BRect(0, 0, 0, 0);
}


/**	Scan from the startIndex and reset the location
 *	as defined in LocationForReplicant()
 */

void
TReplicantTray::RealignReplicants(int32 startIndex)
{
	if (startIndex < 0)
		startIndex = 0;

	int32 count = fShelf->CountReplicants();
	if (count <= 0)
		return;

	if (startIndex == 0)
		fRightBottomReplicant.Set(0, 0, 0, 0);

	BView* view = NULL;
	for (int32 i = startIndex ; i < count ; i++) {
		fShelf->ReplicantAt(i, &view);
		if (view != NULL) {
			BPoint loc = LocationForReplicant(i, view->Frame().Width());
			if (view->Frame().LeftTop() != loc)
				view->MoveTo(loc);
		}
	}
}


status_t
TReplicantTray::_SaveSettings()
{
	status_t result;
	BPath path;
	if ((result = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true))
		 == B_OK) {
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

	desk_settings* settings = ((TBarApp*)be_app)->Settings();
	settings->showTime = !fTime->IsHidden();
	settings->showSeconds = fTime->ShowSeconds();
	settings->showDayOfWeek = fTime->ShowDayOfWeek();
	settings->showTimeZone = fTime->ShowTimeZone();
}


//	#pragma mark -


/*! Draggable region that is asynchronous so that dragging does not block
	other activities.
*/
TDragRegion::TDragRegion(TBarView* parent, BView* child)
	:
	BControl(BRect(0, 0, 0, 0), "", "", NULL, B_FOLLOW_NONE,
		B_WILL_DRAW | B_FRAME_EVENTS),
	fBarView(parent),
	fChild(child),
	fDragLocation(kAutoPlaceDragRegion)
{
}


void
TDragRegion::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (be_control_look != NULL)
		SetViewColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), 1.1));
	else
		SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	ResizeToPreferred();
}


void
TDragRegion::GetPreferredSize(float* width, float* height)
{
	fChild->ResizeToPreferred();
	*width = fChild->Bounds().Width();
	*height = fChild->Bounds().Height();

	if (fDragLocation != kNoDragRegion)
		*width += 7;
	else
		*width += 6;

	*height += 3;
}


void
TDragRegion::FrameMoved(BPoint)
{
	if (fBarView->Left() && fBarView->Vertical()
		&& fDragLocation != kNoDragRegion)
		fChild->MoveTo(5, 2);
	else
		fChild->MoveTo(2, 2);
}


void
TDragRegion::Draw(BRect)
{
	rgb_color menuColor = ViewColor();
	rgb_color hilite = tint_color(menuColor, B_DARKEN_1_TINT);
	rgb_color ldark = tint_color(menuColor, 1.02);
	rgb_color dark = tint_color(menuColor, B_DARKEN_2_TINT);
	rgb_color vvdark = tint_color(menuColor, B_DARKEN_4_TINT);
	rgb_color light = tint_color(menuColor, B_LIGHTEN_2_TINT);

	BRect frame(Bounds());
	BeginLineArray(4);

	if (be_control_look != NULL) {
		if (fBarView->Vertical()) {
			AddLine(frame.LeftTop(), frame.RightTop(), dark);
			AddLine(BPoint(frame.left, frame.top + 1),
				BPoint(frame.right, frame.top + 1), ldark);
			AddLine(frame.LeftBottom(), frame.RightBottom(), hilite);
		} else if (fBarView->AcrossTop() || fBarView->AcrossBottom()) {
			AddLine(frame.LeftTop(),
				BPoint(frame.left, frame.bottom), dark);
			AddLine(BPoint(frame.left + 1, frame.top + 1),
				BPoint(frame.right - 1, frame.top + 1), light);
			AddLine(BPoint(frame.right, frame.top + 2),
				BPoint(frame.right, frame.bottom), hilite);
			AddLine(BPoint(frame.left + 1, frame.bottom),
				BPoint(frame.right - 1, frame.bottom), hilite);
		}
	} else {
		if (fBarView->Vertical()) {
			AddLine(frame.LeftTop(), frame.RightTop(), light);
			AddLine(frame.LeftTop(), frame.LeftBottom(), light);
			AddLine(frame.RightBottom(), frame.RightTop(), hilite);
		} else if (fBarView->AcrossTop()) {
			AddLine(BPoint(frame.left, frame.top + 1),
				BPoint(frame.right - 1, frame.top + 1), light);
			AddLine(frame.RightTop(), frame.RightBottom(), vvdark);
			AddLine(BPoint(frame.right - 1, frame.top + 2),
				BPoint(frame.right - 1, frame.bottom - 1), hilite);
			AddLine(frame.LeftBottom(),
				BPoint(frame.right - 1, frame.bottom), hilite);
		} else if (fBarView->AcrossBottom()) {
			AddLine(BPoint(frame.left, frame.top + 1),
				BPoint(frame.right - 1, frame.top + 1), light);
			AddLine(frame.LeftBottom(), frame.RightBottom(), hilite);
			AddLine(frame.RightTop(), frame.RightBottom(), vvdark);
			AddLine(BPoint(frame.right - 1, frame.top + 1),
				BPoint(frame.right - 1, frame.bottom - 1), hilite);
		}
	}

	EndLineArray();

	if (fDragLocation != kDontDrawDragRegion || fDragLocation != kNoDragRegion)
		DrawDragRegion();
}


void
TDragRegion::DrawDragRegion()
{
	BRect dragRegion(DragRegion());

	rgb_color menuColor = ViewColor();
	rgb_color menuHilite = menuColor;
	if (IsTracking()) {
		// Draw drag region highlighted if tracking mouse
		menuHilite = tint_color(menuColor, B_HIGHLIGHT_BACKGROUND_TINT);
		SetHighColor(menuHilite);
		FillRect(dragRegion);
	}
	rgb_color vdark = tint_color(menuHilite, B_DARKEN_3_TINT);
	rgb_color light = tint_color(menuHilite, B_LIGHTEN_2_TINT);

	BeginLineArray(dragRegion.IntegerHeight());
	BPoint pt;
	pt.x = floorf((dragRegion.left + dragRegion.right) / 2 + 0.5) - 1;
	pt.y = dragRegion.top + 2;

	while (pt.y + 1 <= dragRegion.bottom) {
		AddLine(pt, pt, vdark);
		AddLine(pt + BPoint(1, 1), pt + BPoint(1, 1), light);

		pt.y += 3;
	}
	EndLineArray();
}


BRect
TDragRegion::DragRegion() const
{
	float kTopBottomInset = 2;
	float kLeftRightInset = 1;
	float kDragWidth = 3;
	if (be_control_look != NULL) {
		kTopBottomInset = 1;
		kLeftRightInset = 0;
		kDragWidth = 4;
	}

	BRect dragRegion(Bounds());
	dragRegion.top += kTopBottomInset;
	dragRegion.bottom -= kTopBottomInset;

	bool placeOnLeft = false;
	if (fDragLocation == kAutoPlaceDragRegion) {
		if (fBarView->Vertical() && fBarView->Left())
			placeOnLeft = true;
		else
			placeOnLeft = false;
	} else if (fDragLocation == kDragRegionLeft)
		placeOnLeft = true;
	else if (fDragLocation == kDragRegionRight)
		placeOnLeft = false;

	if (placeOnLeft) {
		dragRegion.left += kLeftRightInset;
		dragRegion.right = dragRegion.left + kDragWidth;
	} else {
		dragRegion.right -= kLeftRightInset;
		dragRegion.left = dragRegion.right - kDragWidth;
	}

	return dragRegion;
}


void
TDragRegion::MouseDown(BPoint thePoint)
{
	ulong buttons;
	BPoint where;
	BRect dragRegion(DragRegion());

	dragRegion.InsetBy(-2.0f, -2.0f);
		// DragRegion() is designed for drawing, not clicking

	if (!dragRegion.Contains(thePoint))
		return;

	while (true) {
		GetMouse(&where, &buttons);
		if (!buttons)
			break;

		if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
			fPreviousPosition = thePoint;
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
TDragRegion::MouseUp(BPoint pt)
{
	if (IsTracking()) {
		SetTracking(false);
		Invalidate(DragRegion());
	} else
		BControl::MouseUp(pt);
}


bool
TDragRegion::SwitchModeForRect(BPoint mouse, BRect rect,
	bool newVertical, bool newLeft, bool newTop, int32 newState)
{
	if (!rect.Contains(mouse)) {
		// not our rect
		return false;
	}

	if (newVertical == fBarView->Vertical() && newLeft == fBarView->Left()
		&& newTop == fBarView->Top() && newState == fBarView->State()) {
		// already in the correct mode
		return true;
	}

	fBarView->ChangeState(newState, newVertical, newLeft, newTop);
	return true;
}


void
TDragRegion::MouseMoved(BPoint where, uint32 code, const BMessage* message)
{
	if (IsTracking()) {
		BScreen screen;
		BRect frame = screen.Frame();

		float hDivider = frame.Width() / 6;
		hDivider = (hDivider < sMinimumWindowWidth + 10.0f)
			? sMinimumWindowWidth + 10.0f : hDivider;
		float miniDivider = frame.top + kMiniHeight + 10.0f;
		float vDivider = frame.Height() / 2;
#ifdef FULL_MODE
		float thirdScreen = frame.Height() / 3;
#endif
		BRect topLeft(frame.left, frame.top, frame.left + hDivider,
			miniDivider);
		BRect topMiddle(frame.left + hDivider, frame.top, frame.right
			- hDivider, vDivider);
		BRect topRight(frame.right - hDivider, frame.top, frame.right,
			miniDivider);

#ifdef FULL_MODE
		vDivider = miniDivider + thirdScreen;
#endif
		BRect middleLeft(frame.left, miniDivider, frame.left + hDivider,
			vDivider);
		BRect middleRight(frame.right - hDivider, miniDivider, frame.right,
			vDivider);

#ifdef FULL_MODE
		BRect leftSide(frame.left, vDivider, frame.left + hDivider,
			frame.bottom - thirdScreen);
		BRect rightSide(frame.right - hDivider, vDivider, frame.right,
			frame.bottom - thirdScreen);

		vDivider = frame.bottom - thirdScreen;
#endif
		BRect bottomLeft(frame.left, vDivider, frame.left + hDivider,
			frame.bottom);
		BRect bottomMiddle(frame.left + hDivider, vDivider, frame.right
			- hDivider, frame.bottom);
		BRect bottomRight(frame.right - hDivider, vDivider, frame.right,
			frame.bottom);

		if (where != fPreviousPosition) {
			fPreviousPosition = where;
			ConvertToScreen(&where);

			// use short circuit evaluation for convenience
			if (SwitchModeForRect(where, topLeft, true, true, true, kMiniState)
				|| SwitchModeForRect(where, topMiddle, false, true, true,
					kExpandoState)
				|| SwitchModeForRect(where, topRight, true, false, true,
					kMiniState)
				|| SwitchModeForRect(where, middleLeft, true, true, true,
					kExpandoState)
				|| SwitchModeForRect(where, middleRight, true, false, true,
					kExpandoState)

#ifdef FULL_MODE
				|| SwitchModeForRect(where, leftSide, true, true, true,
					kFullState)
				|| SwitchModeForRect(where, rightSide, true, false, true,
					kFullState)
#endif
				|| SwitchModeForRect(where, bottomLeft, true, true, false,
					kMiniState)
				|| SwitchModeForRect(where, bottomMiddle, false, true, false,
					kExpandoState)
				|| SwitchModeForRect(where, bottomRight, true, false, false,
					kMiniState))
				;
		}
	} else
		BControl::MouseMoved(where, code, message);
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

