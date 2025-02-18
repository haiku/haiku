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
#ifndef _CONTAINER_WINDOW_H
#define _CONTAINER_WINDOW_H


#include <GroupView.h>
#include <MimeType.h>
#include <StringList.h>
#include <Window.h>

#include "LockingList.h"
#include "NavMenu.h"
#include "TaskLoop.h"


class BGridView;
class BGroupLayout;
class BGroupView;
class BPopUpMenu;
class BMenuBar;

namespace BPrivate {

class BNavigator;
class BPoseView;
class DraggableContainerIcon;
class ModelMenuItem;
class AttributeStreamNode;
class BackgroundImage;
class Model;
class ModelNodeLazyOpener;
class BorderedView;
class SelectionWindow;
class TShortcuts;
class TemplatesMenu;


#define kDefaultFolderTemplate "DefaultFolderTemplate"


extern const char* kAddOnsMenuName;


enum {
	// flags that describe opening of the window
	kRestoreWorkspace	= 0x1,
	kIsHidden			= 0x2,
		// set when opening a window during initial Tracker start
	kRestoreDecor		= 0x4
};


struct AddOnShortcut {
	Model*	model;
	char	key;
	char	defaultKey;
	uint32	modifiers;
};


class BContainerWindow : public BWindow {
public:
	BContainerWindow(LockingList<BWindow>* windowList, uint32 openFlags,
		window_look look = B_DOCUMENT_WINDOW_LOOK,
		window_feel feel = B_NORMAL_WINDOW_FEEL,
		uint32 windowFlags = B_WILL_ACCEPT_FIRST_CLICK | B_NO_WORKSPACE_ACTIVATION,
		uint32 workspace = B_CURRENT_WORKSPACE, bool useLayout = true);

	virtual ~BContainerWindow();

	virtual void Init(const BMessage* message = NULL);
	virtual void InitLayout();

	static BRect InitialWindowRect(window_feel);

	virtual void Minimize(bool minimize);
	virtual void Quit();
	virtual bool QuitRequested();

	virtual void CreatePoseView(Model*);

	virtual void ShowContextMenu(BPoint, const entry_ref*);
	virtual uint32 ShowDropContextMenu(BPoint, BPoseView* source = NULL);
	virtual void MenusBeginning();
	virtual void MenusEnded();
	virtual void MessageReceived(BMessage*);
	virtual void FrameResized(float, float);
	virtual void FrameMoved(BPoint);
	virtual void Zoom(BPoint, float, float);
	virtual void WorkspacesChanged(uint32, uint32);

	// virtuals that control setup of window
	virtual bool ShouldAddMenus() const;
	virtual bool ShouldAddScrollBars() const;

	virtual void CheckScreenIntersect();

	virtual bool IsShowing(const node_ref*) const;
	virtual bool IsShowing(const entry_ref*) const;

	void ResizeToFit();

	Model* TargetModel() const;
	BPoseView* PoseView() const;
	TShortcuts* Shortcuts() const;
	BNavigator* Navigator() const;

	virtual void SelectionChanged();
	virtual void ViewModeChanged(uint32 oldMode, uint32 newMode);

	virtual void RestoreState();
	virtual void RestoreState(const BMessage &);
	void RestoreStateCommon();
	virtual void SaveState(bool hide = true);
	virtual void SaveState(BMessage &) const;
	virtual void SwitchDirectory(const entry_ref* ref);
	virtual void OpenParent();
	void UpdateTitle();

	bool StateNeedsSaving() const;
	bool SaveStateIsEnabled() const;
	void SetSaveStateEnabled(bool);

	void UpdateBackgroundImage();

	static status_t GetLayoutState(BNode*, BMessage*);
	static status_t SetLayoutState(BNode*, const BMessage*);
		// calls for inheriting window size, attribute layout, etc.
		// deprecated

	void AddMimeTypesToMenu();
	void AddMimeTypesToMenu(BMenu*);

	BMenuItem* NewArrangeByMenu();
	virtual void SetupArrangeByMenu(BMenu*);
	void MarkArrangeByMenu(BMenu*);

	BMenuItem* NewAttributeMenuItem(const char* label, const char* name,
		int32 type, float width, int32 align, bool editable,
		bool statField);
	BMenuItem* NewAttributeMenuItem(const char* label, const char* name,
		int32 type, const char* displayAs, float width, int32 align,
		bool editable, bool statField);

	void NewAttributesMenu();
	virtual void NewAttributesMenu(BMenu*);
	void MarkAttributesMenu();
	virtual void MarkAttributesMenu(BMenu*);
	void HideAttributesMenu();
	void ShowAttributesMenu();

	PiggybackTaskLoop* DelayedTaskLoop();
		// use for RunLater queueing
	void PulseTaskLoop();
		// called by some view that has pulse, either BackgroundView
		// or BPoseView

	static bool DefaultStateSourceNode(const char* name, BNode* result,
		bool createNew = false, bool createFolder = true);

	BMessage* AddOnMessage(int32);
	BPopUpMenu* ContextMenu();

	void ShowSelectionWindow();

	void ShowNavigator(bool);
	void SetSingleWindowBrowseShortcuts(bool);

	void SetPathWatchingEnabled(bool);
	bool IsPathWatchingEnabled(void) const;

protected:
	enum MenuContext {
		kFileMenuContext,
		kWindowMenuContext,
		kPosePopUpContext,
		kWindowPopUpContext
	};

protected:
	virtual BPoseView* NewPoseView(Model*, uint32);
		// instantiate a different flavor of BPoseView for different
		// ContainerWindows

	virtual void RestoreWindowState(AttributeStreamNode*);
	virtual void RestoreWindowState(const BMessage &);
	virtual void SaveWindowState(AttributeStreamNode*);
	virtual void SaveWindowState(BMessage&) const;

	virtual bool NeedsDefaultStateSetup();
	virtual void SetupDefaultState();
		// these two virtuals control setting up a new folder that
		// does not have any state settings yet with the default

	virtual void AddMenus();
	virtual void AddShortcuts();
		// add equivalents of the menu shortcuts to the menuless
		// desktop window
	virtual void AddFileMenu(BMenu* menu);
	virtual void AddWindowMenu(BMenu* menu);
	virtual void AddIconSizeMenu(BMenu* menu);

	virtual void AddContextMenus();
	virtual void AddPoseContextMenu(BMenu*);
	virtual void AddWindowContextMenu(BMenu*);
	virtual void AddVolumeContextMenu(BMenu*);
	virtual void AddDropContextMenu(BMenu*);
	virtual void AddTrashContextMenu(BMenu*);

	virtual void DetachSubmenus();
	virtual void RepopulateMenus();

	virtual void SetupNavigationMenu(BMenu*, const entry_ref*);
	virtual void SetupMoveCopyMenus(BMenu*, const entry_ref*);
	virtual void PopulateMoveCopyNavMenu(BNavMenu*, uint32,
		const entry_ref*, bool);

	virtual void SetupOpenWithMenu(BMenu*);
	virtual void SetupOpenWithMenu(BMenu*, const entry_ref* ref);
	virtual void SetupNewTemplatesMenu(BMenu*, MenuContext context);
	virtual void SetupEditQueryItem(BMenu*);
	virtual void SetupEditQueryItem(BMenu*, const entry_ref* ref);
	virtual void SetupDiskMenu(BMenu*);
	virtual void SetupMountMenu(BMenu*, MenuContext context);
	virtual void SetupMountMenu(BMenu*, MenuContext context, const entry_ref* ref);
	BMenuItem* DetachMountMenu();

	virtual void BuildAddOnMenus(BMenuBar*);
	virtual void RebuildAddOnMenus(BMenuBar*);
	virtual void BuildAddOnsMenu(BMenu*);
	void BuildMimeTypeList(BStringList& mimeTypes);

	virtual void UpdateMenu(BMenu* menu, MenuContext context,
		const entry_ref* ref = NULL);
	virtual void UpdateFileMenu(BMenu* menu);
	virtual void UpdatePoseContextMenu(BMenu* menu, const entry_ref* ref);
	virtual void UpdateFileMenuOrPoseContextMenu(BMenu* menu, MenuContext context,
		const entry_ref* ref = NULL);
	virtual void UpdateWindowMenu(BMenu* menu);
	virtual void UpdateWindowContextMenu(BMenu* menu);
	virtual void UpdateWindowMenuOrWindowContextMenu(BMenu* menu, MenuContext context);

	BMenu* AddMimeMenu(const BMimeType& mimeType, bool isSuperType,
		BMenu* menu, int32 start);

	BHandler* ResolveSpecifier(BMessage*, int32, BMessage*, int32,
		const char*);

	void LoadAddOn(BMessage*);
	void EachAddOn(void (*)(const Model*, const char*, uint32 shortcut,
			uint32 modifiers, bool primary, void*, BContainerWindow*, BMenu*),
		void*, BStringList&, BMenu*);

protected:
	LockingList<BWindow>* fWindowList;
	uint32 fOpenFlags;
	bool fUsesLayout;

	bool ShouldHaveNavigationMenu(const entry_ref* = NULL);
	bool ShouldHaveOpenWithMenu(const entry_ref* = NULL);
	bool ShouldHaveEditQueryItem(const entry_ref* = NULL);
	bool ShouldHaveMoveCopyMenus(const entry_ref* = NULL);
	bool ShouldHaveNewFolderItem();
	bool ShouldHaveAddOnMenus();

	BGroupLayout* fRootLayout;
	BGroupView* fMenuContainer;
	BGridView* fPoseContainer;
	BorderedView* fBorderedView;
	BGroupView* fVScrollBarContainer;
	BGroupView* fCountContainer;

	TShortcuts*	fShortcuts;
	BPopUpMenu* fContextMenu;
	BPopUpMenu* fPoseContextMenu;
	BPopUpMenu* fWindowContextMenu;
	BPopUpMenu* fDropContextMenu;
	BPopUpMenu* fVolumeContextMenu;
	BPopUpMenu* fTrashContextMenu;
	BPopUpNavMenu* fDragContextMenu;
	BMenuItem* fMoveToItem;
	BMenuItem* fCopyToItem;
	BMenuItem* fCreateLinkItem;
	BMenuItem* fOpenWithItem;
	BMenuItem* fEditQueryItem;
	BMenuItem* fMountItem;
	ModelMenuItem* fNavigationItem;
	BMenuItem* fNewTemplatesItem;
	BMenuBar* fMenuBar;
	DraggableContainerIcon* fDraggableIcon;
	BNavigator* fNavigator;
	BPoseView* fPoseView;
	BMenu* fAttrMenu;
	BMenu* fWindowMenu;
	BMenu* fFileMenu;
	BMenuItem* fArrangeByItem;

	SelectionWindow* fSelectionWindow;

	PiggybackTaskLoop* fTaskLoop;

	bool fStateNeedsSaving;

	BackgroundImage* fBackgroundImage;

	static LockingList<struct AddOnShortcut, true>* fAddOnsList;

private:
	bigtime_t fLastMenusBeginningTime;

	BRect fSavedZoomRect;
	BRect fPreviousBounds;

	static BRect sNewWindRect;

	bool fSaveStateIsEnabled;
	bool fIsWatchingPath;

	typedef BWindow _inherited;

	friend int32 show_context_menu(void*);
	friend class BackgroundView;

	void _UpdateSelectionMIMEInfo();
	void _AddFolderIcon();
	void _PassMessageToAddOn(BMessage*);
	void _NewTemplateSubmenu(entry_ref);
};


class WindowStateNodeOpener {
	// this class manages opening and closing the proper node for
	// state restoring / saving; the constructor knows how to decide whether
	// to use a special directory for root, etc.
	// setter calls used when no attributes can be read from a node and
	// defaults are to be substituted
public:
	WindowStateNodeOpener(BContainerWindow* window, bool forWriting);
	virtual ~WindowStateNodeOpener();

	void SetTo(const BDirectory*);
	void SetTo(const BEntry* entry, bool forWriting);
	void SetTo(Model*, bool forWriting);

	AttributeStreamNode* StreamNode() const;
	BNode* Node() const;

private:
	ModelNodeLazyOpener* fModelOpener;
	BNode* fNode;
	AttributeStreamNode* fStreamNode;
};


class BorderedView : public BGroupView {
public:
	BorderedView();

	void PoseViewFocused(bool);
	virtual void Pulse();

	void EnableBorderHighlight(bool);

protected:
	virtual void WindowActivated(bool);

private:
	bool fEnableBorderHighlight;

	typedef BGroupView _inherited;
};


// inlines ---------

inline BNavigator*
BContainerWindow::Navigator() const
{
	return fNavigator;
}


inline BPoseView*
BContainerWindow::PoseView() const
{
	return fPoseView;
}


inline TShortcuts*
BContainerWindow::Shortcuts() const
{
	return fShortcuts;
}


inline void
BContainerWindow::SetupDiskMenu(BMenu*)
{
	// nothing at this level
}


inline BPopUpMenu*
BContainerWindow::ContextMenu()
{
	return fContextMenu;
}


inline bool
BContainerWindow::SaveStateIsEnabled() const
{
	return fSaveStateIsEnabled;
}


inline void
BContainerWindow::SetSaveStateEnabled(bool value)
{
	fSaveStateIsEnabled = value;
}


inline bool
BContainerWindow::IsPathWatchingEnabled() const
{
	return fIsWatchingPath;
}


filter_result ActivateWindowFilter(BMessage* message, BHandler** target,
	BMessageFilter* messageFilter);

} // namespace BPrivate

using namespace BPrivate;


#endif	// _CONTAINER_WINDOW_H
