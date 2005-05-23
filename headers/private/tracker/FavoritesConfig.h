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

#ifndef FAVORITES_CONFIG_H
#define FAVORITES_CONFIG_H

#include <Control.h>
#include <Window.h>

#include "Model.h"

class BBox;
class BButton;
class BCheckBox;
class BFilePanel;
class BMenuField;
class BPopUpMenu;
class BTextControl;
class BMessageRunner;

namespace BPrivate {

const uint32 kConfigShow = 'show';
const uint32 kConfigClose = 'canc';

//
//	your app will get one of these messages when
//	the count has changed
//	FindInt32("count", &count)
//
const uint32 kUpdateAppsCount = 'upac';
const uint32 kUpdateDocsCount = 'updc';
const uint32 kUpdateFolderCount = 'upfl';

class NameItemPanel : public BWindow {
public:
	NameItemPanel(BWindow *parent, const char *initialText);
	virtual ~NameItemPanel();
	
	virtual void MessageReceived(BMessage *); 
	
private:
	void AddParts(const char *initialText); 
	
	BWindow *fParent;
	BBox *fBG;
	BTextControl *fNameFld;
	BButton *fCancelBtn;
	BButton *fDoneBtn;
};

class TDraggableIconButton : public BControl {
public:
	TDraggableIconButton(BRect , const char *label, BMessage *,
		uint32 resizemask, uint32 flags);
	virtual ~TDraggableIconButton();
	
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void Draw(BRect);
	virtual void MouseDown(BPoint);
	virtual void MouseUp(BPoint);
	virtual void MouseMoved(BPoint , uint32 , const BMessage *);

	virtual void GetPreferredSize(float *, float *);
	virtual void ResizeToPreferred();

private:
	bool fDragging;
	BRect fInitialClickRect;
	BBitmap *fIcon;
	BRect fIconRect;
	BRect fLabelRect;
};

class TScrollerButton : public BControl {
public:
	TScrollerButton(BRect , BMessage *, bool direction);
	
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void Draw(BRect);
	virtual void MouseDown(BPoint);
	virtual void MouseUp(BPoint);
	virtual void MouseMoved(BPoint , uint32 , const BMessage *);

private:
	bool fDirection;
	rgb_color fSelectedColor;
	BRect fHiliteFrame;
	BMessageRunner *fTicker;
};

class TContentsMenu : public BControl {
public:
	TContentsMenu(BRect , BMessage *singleClick, BMessage *doubleClick,
		int32 visibleItemCount, const entry_ref *startRef);
	virtual ~TContentsMenu();
	
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void Draw(BRect );
	virtual void KeyDown(const char *bytes, int32 numBytes);
	virtual void MessageReceived(BMessage *);
	virtual void MouseDown(BPoint );
	virtual void MouseUp(BPoint );
	virtual void MouseMoved(BPoint , uint32 , const BMessage *);
	
	virtual void GetPreferredSize(float *, float *);
	virtual void ResizeToPreferred();
	
	void StartTracking(BPoint);
	void StopTracking();
	
#ifdef ITEM_EDIT
	void BeginItemEdit(BPoint);
	void StopItemEdit();
#endif
	
	void SetStartRef(const entry_ref *);
	void FillMenu(const entry_ref *);
	void EmptyMenu();
	
	bool ItemFrame(int32 index, BRect *iconFrame, BRect *textFrame, BRect *itemFrame) const;
	void InvalidateItem(int32 index);
	void InvalidateAbsoluteItem(int32 index);
	int32 ItemAt(BPoint, BRect *textFrame, BRect *itemFrame, BRect *iconFrame);
	const Model *ItemAt(int32) const;

	void SelectItemAt(BPoint where);
	void Select(const entry_ref *);

	void OpenItem(int32);
	int32 ItemCount() const;
	void RemoveItem(int32);
	void AddTempItem(BPoint where);

private:
	void UpdateScrollers();
	void Scroll(bool);
	
	BMessage *fDoubleClickMessage;

	int32 fVisibleItemCount;
	entry_ref fStartRef;
	
	BFont *fMenuFont;
	float fFontHeight;
	float fItemHeight;
	rgb_color fHiliteColor;
	
	BRect fInitialClickRect;
	bigtime_t fInitialClickTime;
#ifdef ITEM_EDIT
	BPoint fInitialClickLoc;
	bool fEditingItem;
	BTextView *fEditingFld;
	int32 fItemIndex;
#endif		
	int32 fFirstItem;
	BObjectList<Model> *fContentsList;
	
	BBitmap *fSmallGroupIcon;
	BBitmap *fSymlinkIcon;
	
	TScrollerButton *fUpBtn;
	TScrollerButton *fDownBtn;
};


//	pass -1 to max apps/docs/folders to not have it show
class TFavoritesConfigWindow : public BWindow {
public:
	TFavoritesConfigWindow(BRect frame, const char *title,
		bool modal, uint32 filePanelNodeFlavors,
		BMessenger parent, const entry_ref *startRef,
		int32 maxApps = -1, int32 maxDocs = -1, int32 maxFolders = -1);
	~TFavoritesConfigWindow();
						
	void MessageReceived(BMessage *);
	bool QuitRequested();
	
	void AddNewGroup();
	void AddRefs(BMessage *);
	
private:
	void AddParts(int32 maxApps, int32 maxDocs, int32 maxFolders);
	void AddBeMenuPane(int32 maxApps, int32 maxDocs, int32 maxFolders);
	
	void OpenGroup(const entry_ref *);
	void ShowGroup(const entry_ref *);
	
	void PromptForAdd();
	
	void UpdateButtons();
	
	void UpdateFoldersCount(int32 = -1, bool notifyTracker = true);
	void UpdateDocsCount(int32 = -1, bool notifyTracker = true);
	void UpdateAppsCount(int32 = -1, bool notifyTracker = true);
	
	static void BuildCommon(BRect *frame, int32 count, const char *string, uint32 btnWhat,
		uint32 fldWhat, BCheckBox **button, BTextControl **field, BBox *parent);

	static void AddNewGroup(entry_ref *, entry_ref *);
	static void AddSymLink(const entry_ref *, const entry_ref *);

	uint32 fFilePanelNodeFlavors;
	BMessenger fParent;
	entry_ref fCurrentRef;
	BFilePanel *fAddPanel;
	
	//	Favorites Menu Config controls
	BBox *fBeMenuPaneBG;
	
	BPopUpMenu *fGroupMenu;
	BMenuField *fGroupBtn;
	
	BPopUpMenu *fSortMenu;
	BMenuField *fSortBtn;
	
	TDraggableIconButton *fNewGroupBtn;
	
	BCheckBox *fRecentAppsBtn;
	BTextControl *fRecentAppsFld;
	
	BCheckBox *fRecentFoldersBtn;
	BTextControl *fRecentFoldersFld;
	
	BCheckBox *fRecentDocsBtn;
	BTextControl *fRecentDocsFld;
	
	TContentsMenu *fMenuThing;
	
	BButton *fAddBtn;
	BButton *fRemoveBtn;
	BButton *fEditBtn;
	BButton *fOpenBtn;
};

} // namespace BPrivate

using namespace BPrivate;

#endif
