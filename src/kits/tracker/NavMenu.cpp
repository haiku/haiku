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
trademarks of Be Incorporated in the United States and other countries.
Other brand product names are registered trademarks or trademarks of
their respective holders. All rights reserved.
*/

//	NavMenu is a hierarchical menu of volumes, folders, files and queries
//	displays icons, uses the SlowMenu API for full interruptability


#include "NavMenu.h"

#include <algorithm>

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <Application.h>
#include <Catalog.h>
#include <Debug.h>
#include <Directory.h>
#include <Locale.h>
#include <Path.h>
#include <Query.h>
#include <Screen.h>
#include <StopWatch.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "Attributes.h"
#include "Commands.h"
#include "ContainerWindow.h"
#include "DesktopPoseView.h"
#include "FunctionObject.h"
#include "FSUtils.h"
#include "IconMenuItem.h"
#include "MimeTypes.h"
#include "PoseView.h"
#include "QueryPoseView.h"
#include "Thread.h"
#include "Tracker.h"
#include "VirtualDirectoryEntryList.h"


namespace BPrivate {

const int32 kMinMenuWidth = 150;

enum nav_flags {
	kVolumesOnly = 1,
	kShowParent = 2
};


bool
SpringLoadedFolderCompareMessages(const BMessage* incoming,
	const BMessage* dragMessage)
{
	if (incoming == NULL || dragMessage == NULL)
		return false;

	bool refsMatch = false;
	for (int32 inIndex = 0; incoming->HasRef("refs", inIndex); inIndex++) {
		entry_ref inRef;
		if (incoming->FindRef("refs", inIndex, &inRef) != B_OK) {
			refsMatch = false;
			break;
		}

		bool inRefMatch = false;
		for (int32 dragIndex = 0; dragMessage->HasRef("refs", dragIndex);
			dragIndex++) {
			entry_ref dragRef;
			if (dragMessage->FindRef("refs", dragIndex, &dragRef) != B_OK) {
				inRefMatch =  false;
				break;
			}
			// if the incoming ref matches any ref in the drag ref
			// then we can try the next incoming ref
			if (inRef == dragRef) {
				inRefMatch = true;
				break;
			}
		}
		refsMatch = inRefMatch;
		if (!inRefMatch)
			break;
	}

	if (refsMatch) {
		// If all the refs match try and see if this is a new drag with
		// the same drag contents.
		refsMatch = false;
		BPoint incomingPoint;
		BPoint dragPoint;
		if (incoming->FindPoint("click_pt", &incomingPoint) == B_OK
			&& dragMessage->FindPoint("click_pt", &dragPoint) == B_OK) {
			refsMatch = (incomingPoint == dragPoint);
		}
	}

	return refsMatch;
}


void
SpringLoadedFolderSetMenuStates(const BMenu* menu,
	const BObjectList<BString>* typeslist)
{
	if (menu == NULL || typeslist == NULL || typeslist->IsEmpty())
		return;

	// If a types list exists iterate through the list and see if each item
	// can support any item in the list and set the enabled state of the item.
	int32 count = menu->CountItems();
	for (int32 index = 0 ; index < count ; index++) {
		ModelMenuItem* item = dynamic_cast<ModelMenuItem*>(menu->ItemAt(index));
		if (item == NULL)
			continue;

		const Model* model = item->TargetModel();
		if (!model)
			continue;

		if (model->IsSymLink()) {
			// find out what the model is, resolve if symlink
			BEntry entry(model->EntryRef(), true);
			if (entry.InitCheck() == B_OK) {
				if (entry.IsDirectory()) {
					// folder? always keep enabled
					item->SetEnabled(true);
				} else {
					// other, check its support
					Model resolvedModel(&entry);
					int32 supported
						= resolvedModel.SupportsMimeType(NULL, typeslist);
					item->SetEnabled(supported != kDoesNotSupportType);
				}
			} else {
				// bad entry ref (bad symlink?), disable
				item->SetEnabled(false);
			}
		} else if (model->IsDirectory() || model->IsRoot() || model->IsVolume())
			// always enabled if a container
			item->SetEnabled(true);
		else if (model->IsFile() || model->IsExecutable()) {
			int32 supported = model->SupportsMimeType(NULL, typeslist);
			item->SetEnabled(supported != kDoesNotSupportType);
		} else
			item->SetEnabled(false);
	}
}


void
SpringLoadedFolderAddUniqueTypeToList(entry_ref* ref,
	BObjectList<BString>* typeslist)
{
	if (ref == NULL || typeslist == NULL)
		return;

	// get the mime type for the current ref
	BNodeInfo nodeinfo;
	BNode node(ref);
	if (node.InitCheck() != B_OK)
		return;

	nodeinfo.SetTo(&node);

	char mimestr[B_MIME_TYPE_LENGTH];
	// add it to the list
	if (nodeinfo.GetType(mimestr) == B_OK && strlen(mimestr) > 0) {
		// If this is a symlink, add symlink to the list (below)
		// resolve the symlink, add the resolved type to the list.
		if (strcmp(B_LINK_MIMETYPE, mimestr) == 0) {
			BEntry entry(ref, true);
			if (entry.InitCheck() == B_OK) {
				entry_ref resolvedRef;
				if (entry.GetRef(&resolvedRef) == B_OK)
					SpringLoadedFolderAddUniqueTypeToList(&resolvedRef,
						typeslist);
			}
		}
		// scan the current list, don't add dups
		bool isUnique = true;
		int32 count = typeslist->CountItems();
		for (int32 index = 0 ; index < count ; index++) {
			if (typeslist->ItemAt(index)->Compare(mimestr) == 0) {
				isUnique = false;
				break;
			}
		}

		if (isUnique)
			typeslist->AddItem(new BString(mimestr));
	}
}


void
SpringLoadedFolderCacheDragData(const BMessage* incoming, BMessage** message,
	BObjectList<BString>** typeslist)
{
	if (incoming == NULL)
		return;

	delete* message;
	delete* typeslist;

	BMessage* localMessage = new BMessage(*incoming);
	BObjectList<BString>* localTypesList = new BObjectList<BString>(10, true);

	for (int32 index = 0; incoming->HasRef("refs", index); index++) {
		entry_ref ref;
		if (incoming->FindRef("refs", index, &ref) != B_OK)
			continue;

		SpringLoadedFolderAddUniqueTypeToList(&ref, localTypesList);
	}

	*message = localMessage;
	*typeslist = localTypesList;
}

}


//	#pragma mark - BNavMenu


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NavMenu"


BNavMenu::BNavMenu(const char* title, uint32 message, const BHandler* target,
	BWindow* parentWindow, const BObjectList<BString>* list)
	:
	BSlowMenu(title),
	fMessage(message),
	fMessenger(target, target->Looper()),
	fParentWindow(parentWindow),
	fFlags(0),
	fItemList(NULL),
	fContainer(NULL),
	fIteratingDesktop(false),
	fTypesList(new BObjectList<BString>(10, true))
{
	if (list != NULL)
		*fTypesList = *list;

	InitIconPreloader();

	SetFont(be_plain_font);

	// add the parent window to the invocation message so that it
	// can be closed if option modifier held down during invocation
	BContainerWindow* originatingWindow =
		dynamic_cast<BContainerWindow*>(fParentWindow);
	if (originatingWindow != NULL) {
		fMessage.AddData("nodeRefsToClose", B_RAW_TYPE,
			originatingWindow->TargetModel()->NodeRef(), sizeof(node_ref));
	}

	// too long to have triggers
	SetTriggersEnabled(false);
}


BNavMenu::BNavMenu(const char* title, uint32 message,
	const BMessenger& messenger, BWindow* parentWindow,
	const BObjectList<BString>* list)
	:
	BSlowMenu(title),
	fMessage(message),
	fMessenger(messenger),
	fParentWindow(parentWindow),
	fFlags(0),
	fItemList(NULL),
	fContainer(NULL),
	fIteratingDesktop(false),
	fTypesList(new BObjectList<BString>(10, true))
{
	if (list != NULL)
		*fTypesList = *list;

	InitIconPreloader();

	SetFont(be_plain_font);

	// add the parent window to the invocation message so that it
	// can be closed if option modifier held down during invocation
	BContainerWindow* originatingWindow =
		dynamic_cast<BContainerWindow*>(fParentWindow);
	if (originatingWindow != NULL) {
		fMessage.AddData("nodeRefsToClose", B_RAW_TYPE,
			originatingWindow->TargetModel()->NodeRef(), sizeof (node_ref));
	}

	// too long to have triggers
	SetTriggersEnabled(false);
}


BNavMenu::~BNavMenu()
{
	delete fTypesList;
}


void
BNavMenu::AttachedToWindow()
{
	BSlowMenu::AttachedToWindow();

	SpringLoadedFolderSetMenuStates(this, fTypesList);
		// If dragging, (fTypesList != NULL) set the menu items enabled state
		// relative to the ability to handle an item in the drag message.
	ResetTargets();
		// allow an opportunity to reset the target for each of the items
}


void
BNavMenu::DetachedFromWindow()
{
}


void
BNavMenu::ResetTargets()
{
	SetTargetForItems(Target());
}


void
BNavMenu::ForceRebuild()
{
	ClearMenuBuildingState();
	fMenuBuilt = false;
}


bool
BNavMenu::NeedsToRebuild() const
{
	return !fMenuBuilt;
}


void
BNavMenu::SetNavDir(const entry_ref* ref)
{
	ForceRebuild();
		// reset the slow menu building mechanism so we can add more stuff

	fNavDir = *ref;
}


void
BNavMenu::ClearMenuBuildingState()
{
	delete fContainer;
	fContainer = NULL;

	// item list is non-owning, need to delete the items because
	// they didn't get added to the menu
	if (fItemList != NULL) {
		int32 count = fItemList->CountItems();
		for (int32 index = count - 1; index >= 0; index--)
			delete RemoveItem(index);

		delete fItemList;
		fItemList = NULL;
	}
}


bool
BNavMenu::StartBuildingItemList()
{
	BEntry entry;

	if (fNavDir.device < 0 || entry.SetTo(&fNavDir, true) != B_OK
		|| !entry.Exists()) {
		return false;
	}

	fItemList = new BObjectList<BMenuItem>(50);

	fIteratingDesktop = false;

	BDirectory parent;
	status_t status = entry.GetParent(&parent);

	// if ref is the root item then build list of volume root dirs
	fFlags = uint8((fFlags & ~kVolumesOnly)
		| (status == B_ENTRY_NOT_FOUND ? kVolumesOnly : 0));
	if (fFlags & kVolumesOnly)
		return true;

	Model startModel(&entry, true);
	if (startModel.InitCheck() != B_OK || !startModel.IsContainer())
		return false;

	if (startModel.IsQuery())
		fContainer = new QueryEntryListCollection(&startModel);
	else if (startModel.IsVirtualDirectory())
		fContainer = new VirtualDirectoryEntryList(&startModel);
	else if (startModel.IsDesktop()) {
		fIteratingDesktop = true;
		fContainer = DesktopPoseView::InitDesktopDirentIterator(
			0, 	startModel.EntryRef());
		AddRootItemsIfNeeded();
		AddTrashItem();
	} else if (startModel.IsTrash()) {
		// the trash window needs to display a union of all the
		// trash folders from all the mounted volumes
		BVolumeRoster volRoster;
		volRoster.Rewind();
		BVolume volume;
		fContainer = new EntryIteratorList();

		while (volRoster.GetNextVolume(&volume) == B_OK) {
			if (volume.IsReadOnly() || !volume.IsPersistent())
				continue;

			BDirectory trashDir;

			if (FSGetTrashDir(&trashDir, volume.Device()) == B_OK) {
				EntryIteratorList* iteratorList
					= dynamic_cast<EntryIteratorList*>(fContainer);

				ASSERT(iteratorList != NULL);

				if (iteratorList != NULL)
					iteratorList->AddItem(new DirectoryEntryList(trashDir));
			}
		}
	} else {
		BDirectory* directory = dynamic_cast<BDirectory*>(startModel.Node());

		ASSERT(directory != NULL);

		if (directory != NULL)
			fContainer = new DirectoryEntryList(*directory);
	}

	if (fContainer == NULL || fContainer->InitCheck() != B_OK)
		return false;

	fContainer->Rewind();

	return true;
}


void
BNavMenu::AddRootItemsIfNeeded()
{
	BVolumeRoster roster;
	roster.Rewind();
	BVolume volume;
	while (roster.GetNextVolume(&volume) == B_OK) {
		BDirectory root;
		BEntry entry;
		if (!volume.IsPersistent()
			|| volume.GetRootDirectory(&root) != B_OK
			|| root.GetEntry(&entry) != B_OK) {
			continue;
		}

		Model model(&entry);
		AddOneItem(&model);
	}
}


void
BNavMenu::AddTrashItem()
{
	BPath path;
	if (find_directory(B_TRASH_DIRECTORY, &path) == B_OK) {
		BEntry entry(path.Path());
		Model model(&entry);
		AddOneItem(&model);
	}
}


bool
BNavMenu::AddNextItem()
{
	if ((fFlags & kVolumesOnly) != 0) {
		BuildVolumeMenu();
		return false;
	}

	BEntry entry;
	if (fContainer->GetNextEntry(&entry) != B_OK) {
		// we're finished
		return false;
	}

	if (TrackerSettings().HideDotFiles()) {
		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) == B_OK && name[0] == '.')
			return true;
	}

	Model model(&entry, true);
	if (model.InitCheck() != B_OK) {
//		PRINT(("not showing hidden item %s, wouldn't open\n", model->Name()));
		return true;
	}

	QueryEntryListCollection* queryContainer
		= dynamic_cast<QueryEntryListCollection*>(fContainer);
	if (queryContainer != NULL && !queryContainer->ShowResultsFromTrash()
		&& FSInTrashDir(model.EntryRef())) {
		// query entry is in trash and shall not be shown
		return true;
	}

	ssize_t size = -1;
	PoseInfo poseInfo;
	if (model.Node() != NULL) {
		size = model.Node()->ReadAttr(kAttrPoseInfo, B_RAW_TYPE, 0,
			&poseInfo, sizeof(poseInfo));
	}

	model.CloseNode();

	// item might be in invisible
	if (size == sizeof(poseInfo)
			&& !BPoseView::PoseVisible(&model, &poseInfo)) {
		return true;
	}

	AddOneItem(&model);

	return true;
}


void
BNavMenu::AddOneItem(Model* model)
{
	BMenuItem* item = NewModelItem(model, &fMessage, fMessenger, false,
		dynamic_cast<BContainerWindow*>(fParentWindow),
		fTypesList, &fTrackingHook);

	if (item != NULL)
		fItemList->AddItem(item);
}


ModelMenuItem*
BNavMenu::NewModelItem(Model* model, const BMessage* invokeMessage,
	const BMessenger& target, bool suppressFolderHierarchy,
	BContainerWindow* parentWindow, const BObjectList<BString>* typeslist,
	TrackingHookData* hook)
{
	if (model->InitCheck() != B_OK)
		return 0;

	entry_ref ref;
	bool isContainer = false;
	if (model->IsSymLink()) {
		Model* newResolvedModel = 0;
		Model* result = model->LinkTo();

		if (result == NULL) {
			newResolvedModel = new Model(model->EntryRef(), true, true);

			if (newResolvedModel->InitCheck() != B_OK) {
				// broken link, still can show though, bail
				delete newResolvedModel;
				result = NULL;
			} else
				result = newResolvedModel;
		}

		if (result != NULL) {
			BModelOpener opener(result);
				// open the model, if it ain't open already

			PoseInfo poseInfo;
			ssize_t size = -1;

			if (result->Node() != NULL) {
				size = result->Node()->ReadAttr(kAttrPoseInfo, B_RAW_TYPE, 0,
					&poseInfo, sizeof(poseInfo));
			}

			result->CloseNode();

			if (size == sizeof(poseInfo) && !BPoseView::PoseVisible(result,
				&poseInfo)) {
				// link target does not want to be visible
				delete newResolvedModel;
				return NULL;
			}

			ref = *result->EntryRef();
			isContainer = result->IsContainer();
		}

		model->SetLinkTo(result);
	} else {
		ref = *model->EntryRef();
		isContainer = model->IsContainer();
	}

	BMessage* message = new BMessage(*invokeMessage);
	message->AddRef("refs", model->EntryRef());

	// truncate name if necessary
	BString truncatedString(model->Name());
	be_plain_font->TruncateString(&truncatedString, B_TRUNCATE_END,
		GetMaxMenuWidth());

	ModelMenuItem* item = NULL;
	if (!isContainer || suppressFolderHierarchy) {
		item = new ModelMenuItem(model, truncatedString.String(), message);
		if (invokeMessage->what != B_REFS_RECEIVED)
			item->SetEnabled(false);
			// the above is broken for FavoritesMenu::AddNextItem, which uses a
			// workaround - should fix this
	} else {
		BNavMenu* menu = new BNavMenu(truncatedString.String(),
			invokeMessage->what, target, parentWindow, typeslist);
		menu->SetNavDir(&ref);
		if (hook != NULL) {
			menu->InitTrackingHook(hook->fTrackingHook, &(hook->fTarget),
				hook->fDragMessage);
		}

		item = new ModelMenuItem(model, menu);
		item->SetMessage(message);
	}

	return item;
}


void
BNavMenu::BuildVolumeMenu()
{
	BVolumeRoster roster;
	BVolume volume;

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

			ASSERT(menu->Name() != NULL);

			ModelMenuItem* item = new ModelMenuItem(model, menu);
			BMessage* message = new BMessage(fMessage);

			message->AddRef("refs", model->EntryRef());

			item->SetMessage(message);
			fItemList->AddItem(item);
			ASSERT(item->Label() != NULL);
		}
	}
}


int
BNavMenu::CompareFolderNamesFirstOne(const BMenuItem* i1, const BMenuItem* i2)
{
	ThrowOnAssert(i1 != NULL && i2 != NULL);

	const ModelMenuItem* item1 = dynamic_cast<const ModelMenuItem*>(i1);
	const ModelMenuItem* item2 = dynamic_cast<const ModelMenuItem*>(i2);

	if (item1 != NULL && item2 != NULL) {
		return item1->TargetModel()->CompareFolderNamesFirst(
			item2->TargetModel());
	}

	return strcasecmp(i1->Label(), i2->Label());
}


int
BNavMenu::CompareOne(const BMenuItem* i1, const BMenuItem* i2)
{
	ThrowOnAssert(i1 != NULL && i2 != NULL);

	return strcasecmp(i1->Label(), i2->Label());
}


void
BNavMenu::DoneBuildingItemList()
{
	// add sorted items to menu
	if (TrackerSettings().SortFolderNamesFirst())
		fItemList->SortItems(CompareFolderNamesFirstOne);
	else
		fItemList->SortItems(CompareOne);

	// if the parent link should be shown, it will be the first
	// entry in the menu - but don't add the item if we're already
	// at the file system's root
	if ((fFlags & kShowParent) != 0) {
		BDirectory directory(&fNavDir);
		BEntry entry(&fNavDir);
		if (!directory.IsRootDirectory()
			&& entry.GetParent(&entry) == B_OK) {
			Model model(&entry, true);
			BLooper* looper;
			AddNavParentDir(&model, fMessage.what,
				fMessenger.Target(&looper));
		}
	}

	int32 count = fItemList->CountItems();
	for (int32 index = 0; index < count; index++)
		AddItem(fItemList->ItemAt(index));

	fItemList->MakeEmpty();

	if (count == 0) {
		BMenuItem* item = new BMenuItem(B_TRANSLATE("Empty folder"), 0);
		item->SetEnabled(false);
		AddItem(item);
	}

	SetTargetForItems(fMessenger);
}


int32
BNavMenu::GetMaxMenuWidth(void)
{
	return std::max((int32)(BScreen().Frame().Width() / 4), kMinMenuWidth);
}


void
BNavMenu::AddNavDir(const Model* model, uint32 what, BHandler* target,
	bool populateSubmenu)
{
	BMessage* message = new BMessage((uint32)what);
	message->AddRef("refs", model->EntryRef());
	ModelMenuItem* item = NULL;

	if (populateSubmenu) {
		BNavMenu* navMenu = new BNavMenu(model->Name(), what, target);
		navMenu->SetNavDir(model->EntryRef());
		navMenu->InitTrackingHook(fTrackingHook.fTrackingHook,
			&(fTrackingHook.fTarget), fTrackingHook.fDragMessage);
		item = new ModelMenuItem(model, navMenu);
		item->SetMessage(message);
	} else
		item = new ModelMenuItem(model, model->Name(), message);

	AddItem(item);
}


void
BNavMenu::AddNavParentDir(const char* name,const Model* model,
	uint32 what, BHandler* target)
{
	BNavMenu* menu = new BNavMenu(name, what, target);
	menu->SetNavDir(model->EntryRef());
	menu->SetShowParent(true);
	menu->InitTrackingHook(fTrackingHook.fTrackingHook,
		&(fTrackingHook.fTarget), fTrackingHook.fDragMessage);

	BMenuItem* item = new SpecialModelMenuItem(model, menu);
	BMessage* message = new BMessage(what);
	message->AddRef("refs", model->EntryRef());
	item->SetMessage(message);

	AddItem(item);
}


void
BNavMenu::AddNavParentDir(const Model* model, uint32 what, BHandler* target)
{
	AddNavParentDir(B_TRANSLATE("parent folder"),model, what, target);
}


void
BNavMenu::SetShowParent(bool show)
{
	fFlags = uint8((fFlags & ~kShowParent) | (show ? kShowParent : 0));
}


void
BNavMenu::SetTypesList(const BObjectList<BString>* list)
{
	if (list != NULL)
		*fTypesList = *list;
	else
		fTypesList->MakeEmpty();
}


const BObjectList<BString>*
BNavMenu::TypesList() const
{
	return fTypesList;
}


void
BNavMenu::SetTarget(const BMessenger& messenger)
{
	fMessenger = messenger;
}


BMessenger
BNavMenu::Target()
{
	return fMessenger;
}


TrackingHookData*
BNavMenu::InitTrackingHook(bool (*hook)(BMenu*, void*),
	const BMessenger* target, const BMessage* dragMessage)
{
	fTrackingHook.fTrackingHook = hook;
	if (target != NULL)
		fTrackingHook.fTarget = *target;

	fTrackingHook.fDragMessage = dragMessage;
	SetTrackingHookDeep(this, hook, &fTrackingHook);

	return &fTrackingHook;
}


void
BNavMenu::SetTrackingHookDeep(BMenu* menu, bool (*func)(BMenu*, void*),
	void* state)
{
	menu->SetTrackingHook(func, state);
	int32 count = menu->CountItems();
	for (int32 index = 0 ; index < count; index++) {
		BMenuItem* item = menu->ItemAt(index);
		if (item == NULL)
			continue;

		BMenu* submenu = item->Submenu();
		if (submenu != NULL)
			SetTrackingHookDeep(submenu, func, state);
	}
}
