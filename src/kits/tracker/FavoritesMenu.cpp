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

#include "FavoritesMenu.h"

#include <compat/sys/stat.h>

#include <Application.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <FilePanel.h>
#include <Locale.h>
#include <Message.h>
#include <Path.h>
#include <Query.h>
#include <Roster.h>

#include <functional>
#include <algorithm>

#include "IconMenuItem.h"
#include "NavMenu.h"
#include "PoseView.h"
#include "QueryPoseView.h"
#include "Tracker.h"
#include "Utilities.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FavoritesMenu"

FavoritesMenu::FavoritesMenu(const char* title, BMessage* openFolderMessage,
	BMessage* openFileMessage, const BMessenger &target,
	bool isSavePanel, BRefFilter* filter)
	:	BSlowMenu(title),
		fOpenFolderMessage(openFolderMessage),
		fOpenFileMessage(openFileMessage),
		fTarget(target),
		fContainer(NULL),
		fInitialItemCount(0),
		fIsSavePanel(isSavePanel),
		fRefFilter(filter)
{
}


FavoritesMenu::~FavoritesMenu()
{
	delete fOpenFolderMessage;
	delete fOpenFileMessage;
	delete fContainer;
}


void
FavoritesMenu::SetRefFilter(BRefFilter* filter)
{
	fRefFilter = filter;
}


bool
FavoritesMenu::StartBuildingItemList()
{
	// initialize the menu building state

	if (!fInitialItemCount)
		fInitialItemCount = CountItems();
	else {
		// strip the old items so we can add new fresh ones
		int32 count = CountItems() - fInitialItemCount;
		// keep the items that were added by the FavoritesMenu creator
		while (count--)
			delete RemoveItem(fInitialItemCount);
	}

	fUniqueRefCheck.clear();
	fState = kStart;
	return true;
}


bool
FavoritesMenu::AddNextItem()
{
	// run the next chunk of code for a given item adding state

	if (fState == kStart) {
		fState = kAddingFavorites;
		fSectionItemCount = 0;
		fAddedSeparatorForSection = false;
		// set up adding the GoTo menu items

		try {
			BPath path;
			ThrowOnError(find_directory(B_USER_SETTINGS_DIRECTORY,
				&path, true));
			path.Append(kGoDirectory);
			mkdir(path.Path(), 0777);

			BEntry entry(path.Path());
			Model startModel(&entry, true);
			ThrowOnInitCheckError(&startModel);

			if (!startModel.IsContainer())
				throw B_ERROR;

			if (startModel.IsQuery())
				fContainer = new QueryEntryListCollection(&startModel);
			else
				fContainer = new DirectoryEntryList(*dynamic_cast<BDirectory*>
					(startModel.Node()));

			ThrowOnInitCheckError(fContainer);
			ThrowOnError( fContainer->Rewind() );

		} catch (...) {
			delete fContainer;
			fContainer = NULL;
		}
	}


	if (fState == kAddingFavorites) {
		entry_ref ref;
		if (fContainer
			&& fContainer->GetNextRef(&ref) == B_OK) {
			Model model(&ref, true);
			if (model.InitCheck() != B_OK)
				return true;

			if (!ShouldShowModel(&model))
				return true;

			BMenuItem* item = BNavMenu::NewModelItem(&model,
				model.IsDirectory() ? fOpenFolderMessage : fOpenFileMessage,
				fTarget);
				
			if (item == NULL)
				return true;

			item->SetLabel(ref.name);
				// this is the name of the link in the Go dir

			if (!fAddedSeparatorForSection) {
				fAddedSeparatorForSection = true;
				AddItem(new TitledSeparatorItem(B_TRANSLATE("Favorites")));
			}
			fUniqueRefCheck.push_back(*model.EntryRef());
			AddItem(item);
			fSectionItemCount++;
			return true;
		}

		// done with favorites, set up for adding recent files
		fState = kAddingFiles;

		fAddedSeparatorForSection = false;

		app_info info;
		be_app->GetAppInfo(&info);
		fItems.MakeEmpty();

		int32 apps, docs, folders;
		TrackerSettings().RecentCounts(&apps, &docs, &folders);

		BRoster().GetRecentDocuments(&fItems, docs, NULL, info.signature);
		fIndex = 0;
		fSectionItemCount = 0;
	}

	if (fState == kAddingFiles) {
		//	if this is a Save panel, not an Open panel
		//	then don't add the recent documents
		if (!fIsSavePanel) {
			for (;;) {
				entry_ref ref;
				if (fItems.FindRef("refs", fIndex++, &ref) != B_OK)
					break;

				Model model(&ref, true);
				if (model.InitCheck() != B_OK)
					return true;

				if (!ShouldShowModel(&model))
					return true;

				BMenuItem* item = BNavMenu::NewModelItem(&model,
					fOpenFileMessage, fTarget);
				if (item) {
					if (!fAddedSeparatorForSection) {
						fAddedSeparatorForSection = true;
						AddItem(new TitledSeparatorItem(
							B_TRANSLATE("Recent documents")));
					}
					AddItem(item);
					fSectionItemCount++;
					return true;
				}
			}
		}

		// done with recent files, set up for adding recent folders
		fState = kAddingFolders;

		fAddedSeparatorForSection = false;

		app_info info;
		be_app->GetAppInfo(&info);
		fItems.MakeEmpty();

		int32 apps, docs, folders;
		TrackerSettings().RecentCounts(&apps, &docs, &folders);

		BRoster().GetRecentFolders(&fItems, folders, info.signature);
		fIndex = 0;
	}

	if (fState == kAddingFolders) {
		for (;;) {
			entry_ref ref;
			if (fItems.FindRef("refs", fIndex++, &ref) != B_OK)
				break;

			// don't add folders that are already in the GoTo section
			if (find_if(fUniqueRefCheck.begin(), fUniqueRefCheck.end(),
				bind2nd(std::equal_to<entry_ref>(), ref))
					!= fUniqueRefCheck.end()) {
				continue;
			}

			Model model(&ref, true);
			if (model.InitCheck() != B_OK)
				return true;

			if (!ShouldShowModel(&model))
				return true;

			BMenuItem* item = BNavMenu::NewModelItem(&model,
				fOpenFolderMessage, fTarget, true);
			if (item) {
				if (!fAddedSeparatorForSection) {
					fAddedSeparatorForSection = true;
					AddItem(new TitledSeparatorItem(
						B_TRANSLATE("Recent folders")));
				}
				AddItem(item);
				item->SetEnabled(true);
					// BNavMenu::NewModelItem returns a disabled item here -
					// need to fix this in BNavMenu::NewModelItem
				return true;
			}
		}
	}
	return false;
}


void
FavoritesMenu::DoneBuildingItemList()
{
	SetTargetForItems(fTarget);
}


void
FavoritesMenu::ClearMenuBuildingState()
{
	delete fContainer;
	fContainer = NULL;
	fState = kDone;

	// force the menu to get rebuilt each time
	fMenuBuilt = false;
}


bool
FavoritesMenu::ShouldShowModel(const Model* model)
{
	if (fIsSavePanel && model->IsFile())
		return false;

	if (!fRefFilter || model->Node() == NULL)
		return true;
	
	struct stat_beos statBeOS;
	convert_to_stat_beos(model->StatBuf(), &statBeOS);

	return fRefFilter->Filter(model->EntryRef(), model->Node(), &statBeOS,
		model->MimeType());
}


//	#pragma mark -


RecentsMenu::RecentsMenu(const char* name, int32 which, uint32 what,
	BHandler* target)
	: BNavMenu(name, what, target),
	fWhich(which),
	fRecentsCount(0),
	fItemIndex(0)
{
	int32 applications;
	int32 documents;
	int32 folders;
	TrackerSettings().RecentCounts(&applications,&documents,&folders);

	if (fWhich == 0)
		fRecentsCount = documents;
	else if (fWhich == 1)
		fRecentsCount = applications;
	else if (fWhich == 2)
		fRecentsCount = folders;
}


void
RecentsMenu::DetachedFromWindow()
{
	//
	//	BNavMenu::DetachedFromWindow sets the TypesList to NULL
	//
	BMenu::DetachedFromWindow();
}


bool
RecentsMenu::StartBuildingItemList()
{
	int32 count = CountItems()-1;
	for (int32 index = count; index >= 0; index--) {
		BMenuItem* item = ItemAt(index);
		ASSERT(item);

		RemoveItem(index);
		delete item;
	}
	//
	//	!! note: don't call inherited from here
	//	the navref is not set for this menu
	//	but it still needs to be a draggable navmenu
	//	simply return true so that AddNextItem is called
	//
	//	return BNavMenu::StartBuildingItemList();
	return true;
}


bool
RecentsMenu::AddNextItem()
{
	if (fRecentsCount > 0 && AddRecents(fRecentsCount))
		return true;

	fItemIndex = 0;
	return false;
}


bool
RecentsMenu::AddRecents(int32 count)
{
	if (fItemIndex == 0) {
		fRecentList.MakeEmpty();
		BRoster roster;

		switch(fWhich) {
			case 0:
				roster.GetRecentDocuments(&fRecentList, count);
				break;
			case 1:
				roster.GetRecentApps(&fRecentList, count);
				break;
			case 2:
				roster.GetRecentFolders(&fRecentList, count);
				break;
			default:
				return false;
				break;
		}
	}
	for (;;) {
		entry_ref ref;
		if (fRecentList.FindRef("refs", fItemIndex++, &ref) != B_OK)
			break;

		if (ref.name && strlen(ref.name) > 0) {
			Model model(&ref, true);
			ModelMenuItem* item = BNavMenu::NewModelItem(&model,
					new BMessage(fMessage.what),
					Target(), false, NULL, TypesList());

			if (item) {
				AddItem(item);

				//	return true so that we know to reenter this list
				return true;
			}
			return true;
		}
	}

	//
	//	return false if we are done with this list
	//
	return false;
}


void
RecentsMenu::DoneBuildingItemList()
{
	//
	//	!! note: don't call inherited here
	//	the object list is not built
	//	and this list does not need to be sorted
	//	BNavMenu::DoneBuildingItemList();
	//

	if (CountItems() <= 0) {
		BMenuItem* item = new BMenuItem(B_TRANSLATE("<No recent items>"), 0);
		item->SetEnabled(false);
		AddItem(item);
	} else
		SetTargetForItems(Target());
}


void
RecentsMenu::ClearMenuBuildingState()
{
	fMenuBuilt = false;
	BNavMenu::ClearMenuBuildingState();
}
