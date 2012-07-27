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
#ifndef SLOW_CONTEXT_POPUP_H
#define SLOW_CONTEXT_POPUP_H


#include <PopUpMenu.h>
#include "NavMenu.h"


namespace BPrivate {

class BSlowContextMenu : public BPopUpMenu {
public:
				BSlowContextMenu(const char* title);
	virtual		~BSlowContextMenu();

	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();

	void SetNavDir(const entry_ref*);
	
	void ClearMenu();
	
	void ForceRebuild();
	bool NeedsToRebuild() const;
		// will cause menu to get rebuilt next time it is shown

	void SetTarget(const BMessenger &);
	const BMessenger Target() const;

	void SetTypesList(const BObjectList<BString>* list);
	const BObjectList<BString>* TypesList() const;

	static ModelMenuItem* NewModelItem(Model*, const BMessage*, const BMessenger&,
		bool suppressFolderHierarchy = false, BContainerWindow* = NULL,
		const BObjectList<BString>* typeslist = NULL,
		TrackingHookData* hook = NULL);

	TrackingHookData* InitTrackingHook(bool (*)(BMenu*, void*),
		const BMessenger* target, const BMessage* dragMessage);

	const bool IsShowing() const;

protected:
	virtual bool AddDynamicItem(add_state state);
	virtual bool StartBuildingItemList();
	virtual bool AddNextItem();
	virtual void DoneBuildingItemList();
	virtual void ClearMenuBuildingState();

	void BuildVolumeMenu();

	void AddOneItem(Model*);
	void AddRootItemsIfNeeded();
	void AddTrashItem();
	static void SetTrackingHookDeep(BMenu*, bool (*)(BMenu*, void*), void*);

	bool fMenuBuilt;

private:
	entry_ref fNavDir;
	BMessage fMessage;
	BMessenger fMessenger;
	BWindow* fParentWindow;

	// menu building state
	bool fVolsOnly;
	BObjectList<BMenuItem>* fItemList;
	EntryListBase* fContainer;
	bool fIteratingDesktop;

	const BObjectList<BString>* fTypesList;

	TrackingHookData fTrackingHook;
	bool fIsShowing;
		// see note in AttachedToWindow
};


} // namespace BPrivate

using namespace BPrivate;


inline const BObjectList<BString>*
BSlowContextMenu::TypesList() const
{
	return fTypesList;
}


inline const BMessenger
BSlowContextMenu::Target() const
{
	return fMessenger;
}


inline const bool
BSlowContextMenu::IsShowing() const
{
	return fIsShowing;
}

#endif	// SLOW_CONTEXT_POPUP_H
