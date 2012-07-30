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

#include <string.h>
#include <stdlib.h>

#include <Debug.h>

#include <Application.h>
#include <Catalog.h>
#include <Directory.h>
#include <Locale.h>
#include <Path.h>
#include <Query.h>
#include <StopWatch.h>
#include <VolumeRoster.h>
#include <Volume.h>

#include "Attributes.h"
#include "Commands.h"
#include "ContainerWindow.h"
#include "DesktopPoseView.h"
#include "FSUtils.h"
#include "FunctionObject.h"
#include "IconMenuItem.h"
#include "NavMenu.h"
#include "PoseView.h"
#include "QueryPoseView.h"
#include "SlowContextPopup.h"
#include "Thread.h"
#include "Tracker.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SlowContextPopup"

BSlowContextMenu::BSlowContextMenu(const char* title)
	:	BPopUpMenu(title, false, false),
		fMenuBuilt(false),
		fMessage(B_REFS_RECEIVED),
		fParentWindow(NULL),
		fItemList(NULL),
		fContainer(NULL),
		fTypesList(NULL),
		fIsShowing(false)
{
	InitIconPreloader();

	SetFont(be_plain_font);
	SetTriggersEnabled(false);
}


BSlowContextMenu::~BSlowContextMenu()
{
}


void
BSlowContextMenu::AttachedToWindow()
{
	// showing flag is set immediately as
	// it may take a while to build the menu's
	// contents.
	//
	// it should get set only once when Go is called
	// and will get reset in DetachedFromWindow
	//
	// this flag is used in ContainerWindow::ShowContextMenu
	// to determine whether we should show this menu, and
	// the only reason we need to do this is because this
	// menu is spawned ::Go as an asynchronous menu, which
	// is done because we will deadlock if the target's
	// window is open...  so there
	fIsShowing = true;

	BPopUpMenu::AttachedToWindow();

	SpringLoadedFolderSetMenuStates(this, fTypesList);

	// allow an opportunity to reset the target for each of the items
	SetTargetForItems(Target());
}


void
BSlowContextMenu::DetachedFromWindow()
{
	// see note above in AttachedToWindow
	fIsShowing = false;
	// does this need to set this to null?
	// the parent, handling dnd should set this
	// appropriately
	//
	// if this changes, BeMenu and RecentsMenu
	// in Deskbar should also change
	fTypesList = NULL;
}


void
BSlowContextMenu::SetNavDir(const entry_ref* ref)
{
	ForceRebuild();
		// reset the slow menu building mechanism so we can add more stuff
	
	fNavDir = *ref;
}


void
BSlowContextMenu::ForceRebuild()
{
	ClearMenuBuildingState();
	fMenuBuilt = false;
}


bool
BSlowContextMenu::NeedsToRebuild() const
{
	return !fMenuBuilt;
}


void
BSlowContextMenu::ClearMenu()
{
	RemoveItems(0, CountItems(), true);

	fMenuBuilt = false;
}


void
BSlowContextMenu::ClearMenuBuildingState()
{
	delete fContainer;
	fContainer = NULL;

	// item list is non-owning, need to delete the items because
	// they didn't get added to the menu
	if (fItemList) {
		RemoveItems(0, fItemList->CountItems(), true);
		delete fItemList;
		fItemList = NULL;
	}
}


const int32 kItemsToAddChunk = 20;
const bigtime_t kMaxTimeBuildingMenu = 200000;


bool
BSlowContextMenu::AddDynamicItem(add_state state)
{
	if (fMenuBuilt)
		return false;
	
	if (state == B_ABORT) {
		ClearMenuBuildingState();
		return false;
	}

	if (state == B_INITIAL_ADD && !StartBuildingItemList()) {
		ClearMenuBuildingState();
		return false;
	}

	bigtime_t timeToBail = system_time() + kMaxTimeBuildingMenu;
	for (int32 count = 0; count < kItemsToAddChunk; count++) {
		if (!AddNextItem()) {
			fMenuBuilt = true;
			DoneBuildingItemList();
			ClearMenuBuildingState();
			return false;
				// done with menu, don't call again
		}
		if (system_time() > timeToBail)
			// we have been in here long enough, come back later
			break;
	}

	return true;	// call me again, got more to show
}


bool
BSlowContextMenu::StartBuildingItemList()
{
	// return false when done building
	BEntry entry;

	if (fNavDir.device < 0 || entry.SetTo(&fNavDir) != B_OK
		|| !entry.Exists()) {
		return false;
	}

	fIteratingDesktop = false;
	
	BDirectory parent;
	status_t err = entry.GetParent(&parent);
	fItemList = new BObjectList<BMenuItem>(50);

	// if ref is the root item then build list of volume root dirs
	fVolsOnly = (err == B_ENTRY_NOT_FOUND);

	if (fVolsOnly)
		return true;

	Model startModel(&entry, true);
	if (startModel.InitCheck() == B_OK) {
		if (!startModel.IsContainer())
			return false;

		if (startModel.IsQuery())
			fContainer = new QueryEntryListCollection(&startModel);
		else if (startModel.IsDesktop()) {
			fIteratingDesktop = true;
			fContainer = DesktopPoseView::InitDesktopDirentIterator(0,
				startModel.EntryRef());
			AddRootItemsIfNeeded();
			AddTrashItem();
		} else {
			fContainer = new DirectoryEntryList(*dynamic_cast<BDirectory*>
				(startModel.Node()));
		}

		if (fContainer->InitCheck() != B_OK)
			return false;

		fContainer->Rewind();
	}

	return true;
}


void
BSlowContextMenu::AddRootItemsIfNeeded()
{
	BVolumeRoster roster;
	roster.Rewind();
	BVolume volume;
	while (roster.GetNextVolume(&volume) == B_OK) {

		BDirectory root;
		BEntry entry;
		if (!volume.IsPersistent()
			|| volume.GetRootDirectory(&root) != B_OK
			|| root.GetEntry(&entry) != B_OK)
			continue;

		Model model(&entry);
		AddOneItem(&model);
	}
}


void
BSlowContextMenu::AddTrashItem()
{
	BPath path;
	if (find_directory(B_TRASH_DIRECTORY, &path) == B_OK) {
		BEntry entry(path.Path());
		Model model(&entry);
		AddOneItem(&model);
	}
}


bool
BSlowContextMenu::AddNextItem()
{
	if (fVolsOnly) {
		BuildVolumeMenu();
		return false;
	}
	
	// limit nav menus to 500 items only
	if (fItemList->CountItems() > 500)
		return false;

	BEntry entry;
	if (fContainer->GetNextEntry(&entry) != B_OK)
		// we're finished
		return false;

	Model model(&entry, true);
	if (model.InitCheck() != B_OK) {
//		PRINT(("not showing hidden item %s, wouldn't open\n", model->Name()));
		return true;
	}

	PoseInfo poseInfo;

	if (model.Node())  {
		model.Node()->ReadAttr(kAttrPoseInfo, B_RAW_TYPE, 0,
			&poseInfo, sizeof(poseInfo));
	}

	model.CloseNode();
	
	if (!BPoseView::PoseVisible(&model, &poseInfo)) {
		return true;
	}

	AddOneItem(&model);
	return true;
}


void
BSlowContextMenu::AddOneItem(Model* model)
{
	BMenuItem* item = NewModelItem(model, &fMessage, fMessenger, false,
		dynamic_cast<BContainerWindow*>(fParentWindow) ?
		dynamic_cast<BContainerWindow*>(fParentWindow) : 0,
		fTypesList, &fTrackingHook);

	if (item)
		fItemList->AddItem(item);
}


ModelMenuItem*
BSlowContextMenu::NewModelItem(Model* model, const BMessage* invokeMessage,
	const BMessenger &target, bool suppressFolderHierarchy,
	BContainerWindow* parentWindow, const BObjectList<BString>* typeslist,
	TrackingHookData* hook)
{
	if (model->InitCheck() != B_OK)
		return NULL;

	entry_ref ref;
	bool container = false;
	if (model->IsSymLink()) {
	
		Model* newResolvedModel = NULL;
		Model* result = model->LinkTo();

		if (!result) {
			newResolvedModel = new Model(model->EntryRef(), true, true);

			if (newResolvedModel->InitCheck() != B_OK) {
				// broken link, still can show though, bail
				delete newResolvedModel;
				newResolvedModel = NULL;
			}
			
			result = newResolvedModel;
		}

		if (result) {
			BModelOpener opener(result);
				// open the model, if it ain't open already
					
			PoseInfo poseInfo;
			
			if (result->Node()) {
				result->Node()->ReadAttr(kAttrPoseInfo, B_RAW_TYPE, 0,
					&poseInfo, sizeof(poseInfo));
			}
	
			result->CloseNode();

			ref = *result->EntryRef();
			container = result->IsContainer();
		}
		model->SetLinkTo(result);
	} else {
		ref = *model->EntryRef();
		container = model->IsContainer();
	}

	BMessage* message = new BMessage(*invokeMessage);
	message->AddRef("refs", model->EntryRef());

	// Truncate the name if necessary
	BString truncatedString(model->Name());
	be_plain_font->TruncateString(&truncatedString, B_TRUNCATE_END,
		BNavMenu::GetMaxMenuWidth());

	ModelMenuItem* item = NULL;
	if (!container || suppressFolderHierarchy) {
		item = new ModelMenuItem(model, truncatedString.String(), message);
		if (invokeMessage->what != B_REFS_RECEIVED)
			item->SetEnabled(false);
	} else {
		BNavMenu* menu = new BNavMenu(truncatedString.String(),
			invokeMessage->what, target, parentWindow, typeslist);
		
		menu->SetNavDir(&ref);
		if (hook)
			menu->InitTrackingHook(hook->fTrackingHook, &(hook->fTarget),
				hook->fDragMessage);

		item = new ModelMenuItem(model, menu);
		item->SetMessage(message);
	}

	return item;
}


void
BSlowContextMenu::BuildVolumeMenu()
{
	BVolumeRoster roster;
	BVolume	volume;

	roster.Rewind();
	while (roster.GetNextVolume(&volume) == B_OK) {
		
		if (!volume.IsPersistent())
			continue;
		
		BDirectory startDir;
		if (volume.GetRootDirectory(&startDir) == B_OK) {
			BEntry entry;
			startDir.GetEntry(&entry);

			Model* model = new Model(&entry);
			if (model->InitCheck() != B_OK) {
				delete model;
				continue;
			}
			
			BNavMenu* menu = new BNavMenu(model->Name(), fMessage.what,
				fMessenger, fParentWindow, fTypesList);

			menu->SetNavDir(model->EntryRef());
			menu->InitTrackingHook(fTrackingHook.fTrackingHook,
				&(fTrackingHook.fTarget), fTrackingHook.fDragMessage);

			ASSERT(menu->Name());

			ModelMenuItem* item = new ModelMenuItem(model, menu);
			BMessage* message = new BMessage(fMessage);

			message->AddRef("refs", model->EntryRef());
			item->SetMessage(message);
			fItemList->AddItem(item);
			ASSERT(item->Label());
		}
	}
}


void
BSlowContextMenu::DoneBuildingItemList()
{
	// add sorted items to menu
	if (TrackerSettings().SortFolderNamesFirst())
		fItemList->SortItems(&BNavMenu::CompareFolderNamesFirstOne);
	else
		fItemList->SortItems(&BNavMenu::CompareOne);

	int32 count = fItemList->CountItems();
	for (int32 index = 0; index < count; index++)
		AddItem(fItemList->ItemAt(index));

	fItemList->MakeEmpty();

	if (!count) {
		BMenuItem* item = new BMenuItem(B_TRANSLATE("Empty folder"), 0);
		item->SetEnabled(false);
		AddItem(item);
	}

	SetTargetForItems(fMessenger);
}


void
BSlowContextMenu::SetTypesList(const BObjectList<BString>* list)
{
	fTypesList = list;
}


void
BSlowContextMenu::SetTarget(const BMessenger &target)
{
	fMessenger = target;
}


TrackingHookData*
BSlowContextMenu::InitTrackingHook(bool (*hook)(BMenu*, void*),
	const BMessenger* target, const BMessage* dragMessage)
{
	fTrackingHook.fTrackingHook = hook;
	if (target)
		fTrackingHook.fTarget = *target;
	fTrackingHook.fDragMessage = dragMessage;
	SetTrackingHookDeep(this, hook, &fTrackingHook);
	return &fTrackingHook;
}


void
BSlowContextMenu::SetTrackingHookDeep(BMenu* menu,
	bool (*func)(BMenu*, void*), void* state)
{
	menu->SetTrackingHook(func, state);
	int32 count = menu->CountItems();
	for (int32 index = 0; index < count; index++) {
		BMenuItem* item = menu->ItemAt(index);
		if (!item)
			continue;

		BMenu* submenu = item->Submenu();
		if (submenu)
			SetTrackingHookDeep(submenu, func, state);
	}
}
