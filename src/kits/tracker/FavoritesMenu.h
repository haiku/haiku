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
#ifndef __FAVORITES_MENU__
#define __FAVORITES_MENU__


#include <vector>

#include "NavMenu.h"
#include "ObjectList.h"


class BRefFilter;

namespace BPrivate {

class EntryListBase;

#define kGoDirectory "Tracker/Go"


class FavoritesMenu : public BSlowMenu {
	// FavoritesMenu is used in the FilePanel -
	// displays recent files, recent folders and favorites items
	public:
		FavoritesMenu(const char* title, BMessage* openFolderMessage,
			BMessage* openFileMessage, const BMessenger &,
			bool isSavePanel, BRefFilter* filter = NULL);
		virtual ~FavoritesMenu();
		
		void SetRefFilter(BRefFilter* filter);

	private:
		// override the necessary SlowMenu hooks
		virtual bool StartBuildingItemList();
		virtual bool AddNextItem();
		virtual void DoneBuildingItemList();
		virtual void ClearMenuBuildingState();
				
		bool ShouldShowModel(const Model* model);
		
		BMessage* fOpenFolderMessage;
		BMessage* fOpenFileMessage;
		BMessenger fTarget;

		enum State {
			kStart,
			kAddingFavorites,
			kAddingFiles,
			kAddingFolders,
			kDone
		};

		State fState;

		int32 fIndex;
		int32 fSectionItemCount;
		bool fAddedSeparatorForSection;
			// keeps track whether a separator will be needed before the
			// next inserted item
		BMessage fItems;

		EntryListBase* fContainer;
		BObjectList<BMenuItem>* fItemList;
		int32 fInitialItemCount;
		std::vector<entry_ref> fUniqueRefCheck;
		bool fIsSavePanel;
		BRefFilter* fRefFilter;

		typedef BSlowMenu _inherited;
};


enum recent_type {
	kRecentDocuments = 0,
	kRecentApplications = 1,
	kRecentFolders = 2
};


class RecentsMenu : public BNavMenu {
	public:
		RecentsMenu(const char* name, int32 which, uint32 what,
			BHandler* target);

		void			DetachedFromWindow();

		int32			RecentsCount();

	private:
		virtual	bool	StartBuildingItemList();
		virtual	bool	AddNextItem();
				bool	AddRecents(int32 count);
		virtual	void	DoneBuildingItemList();
		virtual	void	ClearMenuBuildingState();

	private:
		int32			fWhich;
		int32			fRecentsCount;

		int32 			fItemIndex;
		BMessage		fRecentList;
};

} // namespace BPrivate

using namespace BPrivate;

#endif
