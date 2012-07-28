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


#include "RecentItems.h"

#include <Roster.h>

#include "Attributes.h"
#include "IconMenuItem.h"
#include "Model.h"
#include "NavMenu.h"
#include "PoseView.h"
#include "SlowMenu.h"
#include "Tracker.h"
#include "Utilities.h"


class RecentItemsMenu : public BSlowMenu {
public:
	RecentItemsMenu(const char* title, BMessage* openMessage,
		BHandler* itemTarget, int32 maxItems)
		:	BSlowMenu(title),
			fTargetMesage(openMessage),
			fItemTarget(itemTarget),
			fMaxCount(maxItems)
		{}
	virtual ~RecentItemsMenu();

	virtual bool StartBuildingItemList();
	virtual bool AddNextItem();
	virtual void DoneBuildingItemList() {}
	virtual void ClearMenuBuildingState();

protected:
	virtual const BMessage* FileMessage()
		{ return fTargetMesage; }
	virtual const BMessage* ContainerMessage()
		{ return fTargetMesage; }

	BRecentItemsList* fTterator;
	BMessage* fTargetMesage;
	BHandler* fItemTarget;
	int32 fCount;
	int32 fSanityCount;
	int32 fMaxCount;
};


class RecentFilesMenu : public RecentItemsMenu {
public:
	RecentFilesMenu(const char* title, BMessage* openFileMessage,
		BMessage* openFolderMessage, BHandler* target,
		int32 maxItems, bool navMenuFolders, const char* ofType,
		const char* openedByAppSig);

	RecentFilesMenu(const char* title, BMessage* openFileMessage,
		BMessage* openFolderMessage, BHandler* target,
		int32 maxItems, bool navMenuFolders, const char* ofTypeList[],
		int32 ofTypeListCount, const char* openedByAppSig);

	virtual ~RecentFilesMenu();

protected:
	virtual const BMessage* ContainerMessage()
		{ return openFolderMessage; }

private:
	BMessage* openFolderMessage;
};


class RecentFoldersMenu : public RecentItemsMenu {
public:
	RecentFoldersMenu(const char* title, BMessage* openMessage,
		BHandler* target, int32 maxItems, bool navMenuFolders,
		const char* openedByAppSig);
};


class RecentAppsMenu : public RecentItemsMenu {
public:
	RecentAppsMenu(const char* title, BMessage* openMessage,
		BHandler* target, int32 maxItems);
};


// #pragma mark -


RecentItemsMenu::~RecentItemsMenu()
{
	delete fTterator;
	delete fTargetMesage;
}


bool
RecentItemsMenu::AddNextItem()
{
	BMenuItem* item = fTterator->GetNextMenuItem(FileMessage(),
		ContainerMessage(), fItemTarget);

	if (item) {
		AddItem(item);
		fCount++;
	}
	fSanityCount++;

	return fCount < fMaxCount - 1 && (fSanityCount < fMaxCount + 20);
		// fSanityCount is a hacky way of dealing with a lot of stale
		// recent apps
}


bool
RecentItemsMenu::StartBuildingItemList()
{
	// remove any preexisting items
	int32 itemCount = CountItems();
	while (itemCount--)
		delete RemoveItem((int32)0);

	fCount = 0;
	fSanityCount = 0;
	fTterator->Rewind();
	return true;
}


void
RecentItemsMenu::ClearMenuBuildingState()
{
	fMenuBuilt = false;
		// force rebuilding each time
	fTterator->Rewind();
}


// #pragma mark -


RecentFilesMenu::RecentFilesMenu(const char* title, BMessage* openFileMessage,
	BMessage* openFolderMessage, BHandler* target, int32 maxItems,
	bool navMenuFolders, const char* ofType, const char* openedByAppSig)
	:
	RecentItemsMenu(title, openFileMessage, target, maxItems),
	openFolderMessage(openFolderMessage)
{
	fTterator = new BRecentFilesList(maxItems + 10, navMenuFolders,
		ofType, openedByAppSig);
}


RecentFilesMenu::RecentFilesMenu(const char* title, BMessage* openFileMessage,
	BMessage* openFolderMessage, BHandler* target, int32 maxItems,
	bool navMenuFolders, const char* ofTypeList[], int32 ofTypeListCount,
	const char* openedByAppSig)
	:
	RecentItemsMenu(title, openFileMessage, target, maxItems),
	openFolderMessage(openFolderMessage)
{
	fTterator = new BRecentFilesList(maxItems + 10, navMenuFolders,
		ofTypeList, ofTypeListCount, openedByAppSig);
}


RecentFilesMenu::~RecentFilesMenu()
{
	delete openFolderMessage;
}


// #pragma mark -


RecentFoldersMenu::RecentFoldersMenu(const char* title, BMessage* openMessage,
	BHandler* target, int32 maxItems, bool navMenuFolders,
	const char* openedByAppSig)
	:
	RecentItemsMenu(title, openMessage, target, maxItems)
{
	fTterator = new BRecentFoldersList(maxItems + 10, navMenuFolders,
		openedByAppSig);
}


// #pragma mark -


RecentAppsMenu::RecentAppsMenu(const char* title, BMessage* openMessage,
	BHandler* target, int32 maxItems)
	:	RecentItemsMenu(title, openMessage, target, maxItems)
{
	fTterator = new BRecentAppsList(maxItems);
}


// #pragma mark -


BRecentItemsList::BRecentItemsList(int32 maxItems, bool navMenuFolders)
	:
	fMaxItems(maxItems),
	fNavMenuFolders(navMenuFolders)
{
	InitIconPreloader();
		// need the icon cache
	Rewind();
}


void
BRecentItemsList::Rewind()
{
	fIndex = 0;
	fItems.MakeEmpty();
}


BMenuItem*
BRecentItemsList::GetNextMenuItem(const BMessage* fileOpenInvokeMessage,
	const BMessage* containerOpenInvokeMessage, BHandler* target,
	entry_ref* currentItemRef)
{
	entry_ref ref;
	if (GetNextRef(&ref) != B_OK)
		return NULL;

	Model model(&ref, true);
	if (model.InitCheck() != B_OK)
		return NULL;

	bool container = false;
	if (model.IsSymLink()) {

		Model* newResolvedModel = NULL;
		Model* result = model.LinkTo();

		if (!result) {
			newResolvedModel = new Model(model.EntryRef(), true, true);

			if (newResolvedModel->InitCheck() != B_OK) {
				// broken link, still can show though, bail
				delete newResolvedModel;
				result = NULL;
			} else
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
		model.SetLinkTo(result);
	} else {
		ref = *model.EntryRef();
		container = model.IsContainer();
	}

	// if user asked for it, return the current item ref
	if (currentItemRef)
		*currentItemRef = ref;

	BMessage* message;
	if (container && containerOpenInvokeMessage)
		message = new BMessage(*containerOpenInvokeMessage);
	else if (!container && fileOpenInvokeMessage)
		message = new BMessage(*fileOpenInvokeMessage);
	else
		message = new BMessage(B_REFS_RECEIVED);

	message->AddRef("refs", model.EntryRef());

	// Truncate the name if necessary
	BString truncatedString(model.Name());
	be_plain_font->TruncateString(&truncatedString, B_TRUNCATE_END,
		BNavMenu::GetMaxMenuWidth());

	ModelMenuItem* item = NULL;
	if (!container || !fNavMenuFolders)
		item = new ModelMenuItem(&model, truncatedString.String(), message);
	else {
		// add another nav menu item if it's a directory
		BNavMenu* menu = new BNavMenu(truncatedString.String(), message->what,
			target, 0);

		menu->SetNavDir(&ref);
		item = new ModelMenuItem(&model, menu);
		item->SetMessage(message);
	}

	if (item && target)
		item->SetTarget(target);

	return item;
}


status_t
BRecentItemsList::GetNextRef(entry_ref* result)
{
	return fItems.FindRef("refs", fIndex++, result);
}


// #pragma mark -


BRecentFilesList::BRecentFilesList(int32 maxItems, bool navMenuFolders,
	const char* ofType, const char* openedByAppSig)
	:
	BRecentItemsList(maxItems, navMenuFolders),
	fType(ofType),
	fTypes(NULL),
	fTypeCount(0),
	fAppSig(openedByAppSig)
{
}


BRecentFilesList::BRecentFilesList(int32 maxItems, bool navMenuFolders,
	const char* ofTypeList[], int32 ofTypeListCount, const char* openedByAppSig)
	:
	BRecentItemsList(maxItems, navMenuFolders),
	fType(NULL),
	fTypes(NULL),
	fTypeCount(ofTypeListCount),
	fAppSig(openedByAppSig)
{
	if (fTypeCount) {
		fTypes = new char *[ofTypeListCount];
		for (int32 index = 0; index < ofTypeListCount; index++)
			fTypes[index] = strdup(ofTypeList[index]);
	}
}


BRecentFilesList::~BRecentFilesList()
{
	if (fTypeCount) {
		for (int32 index = 0; index < fTypeCount; index++)
			free(fTypes[index]);
		delete [] fTypes;
	}
}


status_t
BRecentFilesList::GetNextRef(entry_ref* ref)
{
	if (fIndex == 0) {
		// Lazy roster Get
		if (fTypes)
			BRoster().GetRecentDocuments(&fItems, fMaxItems,
				const_cast<const char**>(fTypes),
				fTypeCount, fAppSig.Length() ? fAppSig.String() : NULL);
		else
			BRoster().GetRecentDocuments(&fItems, fMaxItems,
				fType.Length() ? fType.String() : NULL,
				fAppSig.Length() ? fAppSig.String() : NULL);

	}
	return BRecentItemsList::GetNextRef(ref);
}


BMenu*
BRecentFilesList::NewFileListMenu(const char* title,
	BMessage* openFileMessage, BMessage* openFolderMessage,
	BHandler* target, int32 maxItems, bool navMenuFolders, const char* ofType,
	const char* openedByAppSig)
{
	return new RecentFilesMenu(title, openFileMessage,
		openFolderMessage, target, maxItems, navMenuFolders, ofType, openedByAppSig);
}


BMenu*
BRecentFilesList::NewFileListMenu(const char* title,
	BMessage* openFileMessage, BMessage* openFolderMessage,
	BHandler* target, int32 maxItems, bool navMenuFolders, const char* ofTypeList[],
	int32 ofTypeListCount, const char* openedByAppSig)
{
	return new RecentFilesMenu(title, openFileMessage,
		openFolderMessage, target, maxItems, navMenuFolders, ofTypeList,
		ofTypeListCount, openedByAppSig);
}


// #pragma mark -


BMenu*
BRecentFoldersList::NewFolderListMenu(const char* title,
	BMessage* openMessage, BHandler* target, int32 maxItems,
	bool navMenuFolders, const char* openedByAppSig)
{
	return new RecentFoldersMenu(title, openMessage, target, maxItems,
		navMenuFolders, openedByAppSig);
}


BRecentFoldersList::BRecentFoldersList(int32 maxItems, bool navMenuFolders,
	const char* openedByAppSig)
	:
	BRecentItemsList(maxItems, navMenuFolders),
	fAppSig(openedByAppSig)
{
}


status_t
BRecentFoldersList::GetNextRef(entry_ref* ref)
{
	if (fIndex == 0) {
		// Lazy roster Get
		BRoster().GetRecentFolders(&fItems, fMaxItems,
			fAppSig.Length() ? fAppSig.String() : NULL);

	}
	return BRecentItemsList::GetNextRef(ref);
}


// #pragma mark -


BRecentAppsList::BRecentAppsList(int32 maxItems)
	:
	BRecentItemsList(maxItems, false)
{
}


status_t
BRecentAppsList::GetNextRef(entry_ref* ref)
{
	if (fIndex == 0) {
		// Lazy roster Get
		BRoster().GetRecentApps(&fItems, fMaxItems);
	}
	return BRecentItemsList::GetNextRef(ref);
}


BMenu*
BRecentAppsList::NewAppListMenu(const char* title, BMessage* openMessage,
	 BHandler* target, int32 maxItems)
{
	return new RecentAppsMenu(title, openMessage, target, maxItems);
}
