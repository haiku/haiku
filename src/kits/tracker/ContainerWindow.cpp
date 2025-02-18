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


#include "ContainerWindow.h"

#include <Alert.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <GridView.h>
#include <GroupLayout.h>
#include <Keymap.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <UnicodeChar.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <WindowPrivate.h>

#include <fs_attr.h>
#include <image.h>
#include <strings.h>
#include <stdlib.h>

#include "Attributes.h"
#include "AutoDeleter.h"
#include "AutoLock.h"
#include "BackgroundImage.h"
#include "Commands.h"
#include "CountView.h"
#include "DeskWindow.h"
#include "DraggableContainerIcon.h"
#include "FSClipboard.h"
#include "FSUndoRedo.h"
#include "FSUtils.h"
#include "FavoritesMenu.h"
#include "FindPanel.h"
#include "IconMenuItem.h"
#include "LiveMenu.h"
#include "MimeTypes.h"
#include "Model.h"
#include "MountMenu.h"
#include "NavMenu.h"
#include "Navigator.h"
#include "OpenWithWindow.h"
#include "PoseView.h"
#include "QueryContainerWindow.h"
#include "SelectionWindow.h"
#include "Shortcuts.h"
#include "TemplatesMenu.h"
#include "Thread.h"
#include "TitleView.h"
#include "Tracker.h"
#include "TrackerSettings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ContainerWindow"


#ifdef _IMPEXP_BE
_IMPEXP_BE
#endif
void do_minimize_team(BRect zoomRect, team_id team, bool zoom);


struct AddOneAddOnParams {
	BObjectList<BMenuItem>* primaryList;
	BObjectList<BMenuItem>* secondaryList;
};

struct StaggerOneParams {
	bool rectFromParent;
};


BRect BContainerWindow::sNewWindRect;
static int32 sWindowStaggerBy;

LockingList<AddOnShortcut, true>* BContainerWindow::fAddOnsList
	= new LockingList<struct AddOnShortcut, true>(10);


namespace BPrivate {


int
CompareContainerWindowNodeRef(const BContainerWindow* item1, const BContainerWindow* item2)
{
	const node_ref* ref1 = item1->TargetModel()->NodeRef();
	const node_ref* ref2 = item2->TargetModel()->NodeRef();
	if (*ref1 < *ref2)
		return -1;
	else if (*ref1 == *ref2)
		return 0;
	else
		return 1;
}


filter_result
ActivateWindowFilter(BMessage*, BHandler** target, BMessageFilter*)
{
	BView* view = dynamic_cast<BView*>(*target);

	// activate the window if no PoseView or DraggableContainerIcon had been
	// pressed (those will activate the window themselves, if necessary)
	if (view != NULL
		&& dynamic_cast<BPoseView*>(view) == NULL
		&& dynamic_cast<DraggableContainerIcon*>(view) == NULL
		&& view->Window() != NULL) {
		view->Window()->Activate(true);
	}

	return B_DISPATCH_MESSAGE;
}


}	// namespace BPrivate


static int32
AddOnMenuGenerate(const entry_ref* addOnRef, BMenu* menu, BContainerWindow* window)
{
	BEntry entry(addOnRef);
	BPath path;
	status_t result = entry.InitCheck();
	if (result != B_OK)
		return result;

	result = entry.GetPath(&path);
	if (result != B_OK)
		return result;

	image_id addOnImage = load_add_on(path.Path());
	if (addOnImage < 0)
		return addOnImage;

	void (*populateMenu)(BMessage*, BMenu*, BHandler*);
	result = get_image_symbol(addOnImage, "populate_menu", 2, (void**)&populateMenu);
	if (result < 0) {
		PRINT(("Couldn't find populate_menu\n"));
		unload_add_on(addOnImage);
		return result;
	}

	BMessage* message = window->AddOnMessage(B_TRACKER_ADDON_MESSAGE);
	message->AddRef("addon_ref", addOnRef);

	// call add-on code
	(*populateMenu)(message, menu, window->PoseView());

	unload_add_on(addOnImage);
	return B_OK;
}


static status_t
RunAddOnMessageThread(BMessage *message, void *)
{
	entry_ref addOnRef;
	BEntry entry;
	BPath path;
	status_t result = message->FindRef("addon_ref", &addOnRef);
	image_id addOnImage;

	if (result != B_OK)
		goto end;

	entry = BEntry(&addOnRef);
	result = entry.InitCheck();
	if (result != B_OK)
		goto end;

	result = entry.GetPath(&path);
	if (result != B_OK)
		goto end;

	addOnImage = load_add_on(path.Path());
	if (addOnImage < 0) {
		result = addOnImage;
		goto end;
	}
	void (*messageReceived)(BMessage*);
	result = get_image_symbol(addOnImage, "message_received", 2,
		(void**)&messageReceived);

	if (result < 0) {
		PRINT(("Couldn't find message_received\n"));
		unload_add_on(addOnImage);
		goto end;
	}
	// call add-on code
	(*messageReceived)(message);
	unload_add_on(addOnImage);
	return B_OK;

end:
	BString buffer(B_TRANSLATE("Error %error loading add-On %name."));
	buffer.ReplaceFirst("%error", strerror(result));
	buffer.ReplaceFirst("%name", addOnRef.name);

	BAlert* alert = new BAlert("", buffer.String(), B_TRANSLATE("Cancel"),
		0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();

	return result;
}


static void
AddOneAddOn(const Model* model, const char* name, uint32 shortcut,
	uint32 modifiers, bool primary, void* context,
	BContainerWindow* window, BMenu* menu)
{
	AddOneAddOnParams* params = (AddOneAddOnParams*)context;

	BMessage* message = new BMessage(kLoadAddOn);
	message->AddRef("refs", model->EntryRef());

	ModelMenuItem* item;
	try {
		item = new ModelMenuItem(model, name, message, (char)shortcut, modifiers);
	} catch (...) {
		delete message;
		return;
	}

	const entry_ref* addOnRef = model->EntryRef();
	AddOnMenuGenerate(addOnRef, menu, window);

	if (primary)
		params->primaryList->AddItem(item);
	else
		params->secondaryList->AddItem(item);
}


static int32
AddOnThread(BMessage* refsMessage, entry_ref addOnRef, entry_ref directoryRef)
{
	ObjectDeleter<BMessage> _(refsMessage);

	BEntry entry(&addOnRef);
	BPath path;
	status_t result = entry.InitCheck();
	if (result == B_OK)
		result = entry.GetPath(&path);

	if (result == B_OK) {
		image_id addOnImage = load_add_on(path.Path());
		if (addOnImage >= 0) {
			void (*processRefs)(entry_ref, BMessage*, void*);
			result = get_image_symbol(addOnImage, "process_refs", 2,
				(void**)&processRefs);

			if (result >= 0) {
				// call add-on code
				(*processRefs)(directoryRef, refsMessage, NULL);

				unload_add_on(addOnImage);
				return B_OK;
			} else
				PRINT(("couldn't find process_refs\n"));

			unload_add_on(addOnImage);
		} else
			result = addOnImage;
	}

	BString buffer(B_TRANSLATE("Error %error loading Add-On %name."));
	buffer.ReplaceFirst("%error", strerror(result));
	buffer.ReplaceFirst("%name", addOnRef.name);

	BAlert* alert = new BAlert("", buffer.String(), B_TRANSLATE("Cancel"),
		0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();

	return result;
}


static bool
NodeHasSavedState(const BNode* node)
{
	attr_info info;
	return node->GetAttrInfo(kAttrWindowFrame, &info) == B_OK;
}


static bool
OffsetFrameOne(const char* DEBUG_ONLY(name), uint32, off_t, void* castToRect,
	void* castToParams)
{
	ASSERT(strcmp(name, kAttrWindowFrame) == 0);
	StaggerOneParams* params = (StaggerOneParams*)castToParams;

	if (!params->rectFromParent)
		return false;

	if (!castToRect)
		return false;

	((BRect*)castToRect)->OffsetBy(sWindowStaggerBy, sWindowStaggerBy);

	return true;
}


static void
AddMimeTypeString(BStringList& list, Model* model)
{
	if (model == NULL)
		return;

	const char* modelMimeType = model->MimeType();
	if (modelMimeType == NULL || *modelMimeType == '\0')
		return;

	BString type = BString(modelMimeType);
	if (list.HasString(type, true))
		return;

	list.Add(type);
}


//	#pragma mark - BContainerWindow


BContainerWindow::BContainerWindow(LockingList<BWindow>* list, uint32 openFlags, window_look look,
	window_feel feel, uint32 windowFlags, uint32 workspace, bool useLayout)
	:
	BWindow(InitialWindowRect(feel), "TrackerWindow", look, feel, windowFlags, workspace),
	fWindowList(list),
	fOpenFlags(openFlags),
	fUsesLayout(useLayout),
	fMenuContainer(NULL),
	fPoseContainer(NULL),
	fBorderedView(NULL),
	fVScrollBarContainer(NULL),
	fCountContainer(NULL),
	fShortcuts(NULL),
	fContextMenu(NULL),
	fPoseContextMenu(NULL),
	fWindowContextMenu(NULL),
	fDropContextMenu(NULL),
	fVolumeContextMenu(NULL),
	fTrashContextMenu(NULL),
	fDragContextMenu(NULL),
	fMoveToItem(NULL),
	fCopyToItem(NULL),
	fCreateLinkItem(NULL),
	fOpenWithItem(NULL),
	fEditQueryItem(NULL),
	fMountItem(NULL),
	fNavigationItem(NULL),
	fNewTemplatesItem(NULL),
	fMenuBar(NULL),
	fDraggableIcon(NULL),
	fNavigator(NULL),
	fPoseView(NULL),
	fAttrMenu(NULL),
	fWindowMenu(NULL),
	fFileMenu(NULL),
	fArrangeByItem(NULL),
	fSelectionWindow(NULL),
	fTaskLoop(NULL),
	fStateNeedsSaving(false),
	fBackgroundImage(NULL),
	fLastMenusBeginningTime(0),
	fSavedZoomRect(0, 0, -1, -1),
	fSaveStateIsEnabled(true),
	fIsWatchingPath(false)
{
	InitIconPreloader();

	if (list != NULL) {
		ASSERT(list->IsLocked());
		list->AddItem(this);
	}

	if (fUsesLayout) {
		SetFlags(Flags() | B_AUTO_UPDATE_SIZE_LIMITS);

		fRootLayout = new BGroupLayout(B_VERTICAL, 0);
		fRootLayout->SetInsets(0);
		SetLayout(fRootLayout);
		fRootLayout->Owner()->AdoptSystemColors();

		fMenuContainer = new BGroupView(B_HORIZONTAL, 0);
		fRootLayout->AddView(fMenuContainer);

		fPoseContainer = new BGridView(0.0, 0.0);
		fRootLayout->AddView(fPoseContainer);

		fBorderedView = new BorderedView;
		fPoseContainer->GridLayout()->AddView(fBorderedView, 0, 1);

		fCountContainer = new BGroupView(B_HORIZONTAL, 0);
		fPoseContainer->GridLayout()->AddView(fCountContainer, 0, 2);
	}

	AddCommonFilter(new BMessageFilter(B_MOUSE_DOWN, ActivateWindowFilter));

	Run();

	// watch out for settings changes
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker != NULL && tracker->Lock()) {
		tracker->StartWatching(this, kWindowsShowFullPathChanged);
		tracker->StartWatching(this, kSingleWindowBrowseChanged);
		tracker->StartWatching(this, kShowNavigatorChanged);
		tracker->Unlock();
	}

	// ToDo: remove me once we have undo/redo menu items
	// (that is, move them to AddShortcuts())
	AddShortcut('Z', B_COMMAND_KEY, new BMessage(B_UNDO), this);
	AddShortcut('Z', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(B_REDO), this);
}


BContainerWindow::~BContainerWindow()
{
	ASSERT(IsLocked());

	// stop the watchers
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker != NULL && tracker->Lock()) {
		tracker->StopWatching(this, kWindowsShowFullPathChanged);
		tracker->StopWatching(this, kSingleWindowBrowseChanged);
		tracker->StopWatching(this, kShowNavigatorChanged);
		tracker->Unlock();
	}

	delete fTaskLoop;
	delete fBackgroundImage;
	delete fShortcuts;

	if (fSelectionWindow != NULL && fSelectionWindow->Lock())
		fSelectionWindow->Quit();
}


BRect
BContainerWindow::InitialWindowRect(window_feel feel)
{
	if (!sNewWindRect.IsValid()) {
		const float labelSpacing = be_control_look->DefaultLabelSpacing();
		// approximately (85, 50, 548, 280) with default spacing
		sNewWindRect = BRect(labelSpacing * 14, labelSpacing * 8,
			labelSpacing * 91, labelSpacing * 46);
		sWindowStaggerBy = (int32)(labelSpacing * 3.0f);
	}

	if (feel != kDesktopWindowFeel)
		return sNewWindRect;

	// do not offset desktop window
	BRect result = sNewWindRect;
	result.OffsetTo(0, 0);
	return result;
}


void
BContainerWindow::Minimize(bool minimize)
{
	if (minimize && (modifiers() & B_OPTION_KEY) != 0)
		do_minimize_team(BRect(0, 0, 0, 0), be_app->Team(), true);
	else
		_inherited::Minimize(minimize);
}


bool
BContainerWindow::QuitRequested()
{
	// this is a response to the DeskBar sending us a B_QUIT, when it really
	// means to say close all your windows. It might be better to have it
	// send a kCloseAllWindows message and have windowless apps stay running,
	// which is what we will do for the Tracker
	if (CurrentMessage() != NULL
		&& ((CurrentMessage()->FindInt32("modifiers") & B_CONTROL_KEY)) != 0) {
		be_app->PostMessage(kCloseAllWindows);
	}

	Hide();
		// this will close the window instantly, even if
		// the file system is very busy right now
	return true;
}


void
BContainerWindow::Quit()
{
	// delete detached menus in reverse chronological order
	// we're quitting, don't bother setting NULL

	if (fCreateLinkItem != NULL && fCreateLinkItem->Menu() == NULL)
		delete fCreateLinkItem;
	if (fCopyToItem != NULL && fCopyToItem->Menu() == NULL)
		delete fCopyToItem;
	if (fMoveToItem != NULL && fMoveToItem->Menu() == NULL)
		delete fMoveToItem;

	if (fMountItem != NULL && fMountItem->Menu() == NULL)
		delete fMountItem;

	if (fOpenWithItem != NULL && fOpenWithItem->Menu() == NULL)
		delete fOpenWithItem;
	if (fEditQueryItem != NULL && fEditQueryItem->Menu() == NULL)
		delete fEditQueryItem;

	if (fNewTemplatesItem != NULL && fNewTemplatesItem->Menu() == NULL)
		delete fNewTemplatesItem;

	if (fNavigationItem != NULL && fNavigationItem->Menu() == NULL)
		delete fNavigationItem;

	if (fAttrMenu != NULL && fAttrMenu->Supermenu() == NULL)
		delete fAttrMenu;

	if (fArrangeByItem != NULL && fArrangeByItem->Menu() == NULL)
		delete fArrangeByItem;

	delete fPoseContextMenu;
	delete fWindowContextMenu;
	delete fDropContextMenu;
	delete fVolumeContextMenu;
	delete fDragContextMenu;
	delete fTrashContextMenu;

	int32 windowCount = 0;

	// This is a deadlock code sequence - need to change this
	// to acquire the window list while this container window is unlocked
	if (fWindowList != NULL) {
		AutoLock<LockingList<BWindow> > lock(fWindowList);
		if (lock.IsLocked()) {
			fWindowList->RemoveItem(this, false);
			windowCount = fWindowList->CountItems();
		}
	}

	if (StateNeedsSaving())
		SaveState();

	if (fWindowList != NULL && windowCount == 0)
		be_app->PostMessage(B_QUIT_REQUESTED);

	_inherited::Quit();
}


BPoseView*
BContainerWindow::NewPoseView(Model* model, uint32 viewMode)
{
	return new BPoseView(model, viewMode);
}


void
BContainerWindow::CreatePoseView(Model* model)
{
	fPoseView = NewPoseView(model, kListMode);
	fBorderedView->GroupLayout()->AddView(fPoseView);
	fBorderedView->GroupLayout()->SetInsets(1, 0, 1, 1);
	fBorderedView->EnableBorderHighlight(false);

	TrackerSettings settings;
	if (settings.SingleWindowBrowse() && model->IsDirectory() && !PoseView()->IsFilePanel()) {
		fNavigator = new BNavigator(model);
		fPoseContainer->GridLayout()->AddView(fNavigator, 0, 0, 2);
		if (!settings.ShowNavigator())
			fNavigator->Hide();
	}

	SetPathWatchingEnabled(settings.ShowNavigator()
		|| settings.ShowFullPathInTitleBar());
}


void
BContainerWindow::AddContextMenus()
{
	// create context sensitive menus
	fPoseContextMenu = new TLivePosePopUpMenu("PoseContext", this, false, false);
	AddPoseContextMenu(fPoseContextMenu);

	fVolumeContextMenu = new BPopUpMenu("VolumeContext", false, false);
	AddVolumeContextMenu(fVolumeContextMenu);

	fWindowContextMenu = new TLiveWindowPopUpMenu("WindowContext", this, false, false);
	AddWindowContextMenu(fWindowContextMenu);

	fDropContextMenu = new BPopUpMenu("DropContext", false, false);
	AddDropContextMenu(fDropContextMenu);

	fDragContextMenu = new BPopUpNavMenu("DragContext");
		// will get added and built dynamically in ShowContextMenu

	fTrashContextMenu = new BPopUpMenu("TrashContext", false, false);
	AddTrashContextMenu(fTrashContextMenu);
}


void
BContainerWindow::DetachSubmenus()
{
	// detach menus in reverse chronological order
	if (fCreateLinkItem != NULL && fCreateLinkItem->Menu() != NULL)
		fCreateLinkItem->Menu()->RemoveItem(fCreateLinkItem);
	if (fCopyToItem != NULL && fCopyToItem->Menu() != NULL)
		fCopyToItem->Menu()->RemoveItem(fCopyToItem);
	if (fMoveToItem != NULL && fMoveToItem->Menu() != NULL)
		fMoveToItem->Menu()->RemoveItem(fMoveToItem);

	if (fMountItem != NULL && fMountItem->Menu() != NULL)
		fMountItem = DetachMountMenu();

	if (fOpenWithItem != NULL && fOpenWithItem->Menu() != NULL)
		fOpenWithItem->Menu()->RemoveItem(fOpenWithItem);
	if (fEditQueryItem != NULL && fEditQueryItem->Menu() != NULL)
		fEditQueryItem->Menu()->RemoveItem(fEditQueryItem);

	if (fNewTemplatesItem != NULL && fNewTemplatesItem->Menu() != NULL)
		fNewTemplatesItem->Menu()->RemoveItem(fNewTemplatesItem);

	if (fNavigationItem != NULL && fNavigationItem->Menu() != NULL) {
		// delete separator first
		delete fNavigationItem->Menu()->RemoveItem(
			fNavigationItem->Menu()->IndexOf(fNavigationItem) + 1);
		fNavigationItem->Menu()->RemoveItem(fNavigationItem);
	}
}


void
BContainerWindow::RepopulateMenus()
{
	DetachSubmenus();

	if (fMenuBar != NULL) {
		if (fFileMenu != NULL) {
			fMenuBar->RemoveItem(fFileMenu);
			delete fFileMenu;
		}

		if (fWindowMenu != NULL) {
			fMenuBar->RemoveItem(fWindowMenu);
			delete fWindowMenu;
		}

		if (fAttrMenu != NULL) {
			fMenuBar->RemoveItem(fAttrMenu);
			delete fAttrMenu;
		}

		if (ShouldAddMenus()) {
			AddMenus();
			if (PoseView()->ViewMode() == kListMode)
				fMenuBar->AddItem(fAttrMenu, 2);
		}
	}

	delete fPoseContextMenu;
	fPoseContextMenu = new TLivePosePopUpMenu("PoseContext", this, false, false);
	AddPoseContextMenu(fPoseContextMenu);

	delete fWindowContextMenu;
	fWindowContextMenu = new TLiveWindowPopUpMenu("WindowContext", this, false, false);
	AddWindowContextMenu(fWindowContextMenu);
}


void
BContainerWindow::Init(const BMessage* message)
{
	// pose view is expected to be setup at this point
	if (PoseView() == NULL)
		return;

	// deal with new unconfigured folders
	if (NeedsDefaultStateSetup())
		SetupDefaultState();

	if (ShouldAddScrollBars())
		PoseView()->AddScrollBars();

	fShortcuts = new TShortcuts(this);

	fEditQueryItem = Shortcuts()->EditQueryItem();

	const char* name = Shortcuts()->MoveToLabel();
	fMoveToItem = Shortcuts()->MoveToItem(new BNavMenu(name, kMoveSelectionTo, this));
	name = Shortcuts()->CopyToLabel();
	fCopyToItem = Shortcuts()->CopyToItem(new BNavMenu(name, kCopySelectionTo, this));
	name = Shortcuts()->CreateLinkLabel();
	fCreateLinkItem = Shortcuts()->CreateLinkItem(new BNavMenu(name, kCreateLink, this));

	name = Shortcuts()->NewTemplatesLabel();
	fNewTemplatesItem = Shortcuts()->NewTemplatesItem(new TemplatesMenu(PoseView(), name));

	TrackerSettings settings;

	if (ShouldAddMenus()) {
		fMenuBar = new BMenuBar("MenuBar");
		fMenuContainer->GroupLayout()->AddView(fMenuBar);
		AddMenus();

		if (!TargetModel()->IsRoot() && !TargetModel()->IsTrash())
			_AddFolderIcon();
	} else {
		// add equivalents of the menu shortcuts to the menuless
		// desktop window
		AddShortcuts();
	}

	AddContextMenus();
	AddShortcut(B_DELETE, B_NO_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kDeleteSelection),
		PoseView());
	AddShortcut('K', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kCleanupAll), PoseView());
	AddShortcut('Q', B_COMMAND_KEY | B_OPTION_KEY | B_SHIFT_KEY | B_CONTROL_KEY,
		new BMessage(kQuitTracker));

	AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY, new BMessage(kOpenSelection), PoseView());

	SetSingleWindowBrowseShortcuts(settings.SingleWindowBrowse());

#if DEBUG
	// add some debugging shortcuts
	AddShortcut('D', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage('dbug'),
		PoseView());
	AddShortcut('C', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage('dpcc'),
		PoseView());
	AddShortcut('F', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage('dpfl'),
		PoseView());
	AddShortcut('F', B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY,
		new BMessage('dpfL'), PoseView());
#endif

	BKeymap keymap;
	if (keymap.SetToCurrent() == B_OK) {
		BStringList unmodified(3);
		if (keymap.GetModifiedCharacters("+", B_SHIFT_KEY, 0, unmodified)
				== B_OK) {
			int32 count = unmodified.CountStrings();
			for (int32 i = 0; i < count; i++) {
				uint32 key = BUnicodeChar::FromUTF8(unmodified.StringAt(i));
				if (!HasShortcut(key, 0)) {
					// Add semantic zoom in shortcut, bug #6692
					BMessage* increaseSize = new BMessage(kIconMode);
					increaseSize->AddInt32("scale", 1);
					AddShortcut(key, B_COMMAND_KEY, increaseSize, PoseView());
				}
			}
		}
		unmodified.MakeEmpty();
	}

	if (message != NULL)
		RestoreState(*message);
	else
		RestoreState();

	bool isListMode = PoseView()->ViewMode() == kListMode;
	if (ShouldAddMenus() && isListMode) {
		// for now only show attributes in list view
		// eventually enable attribute menu to allow users to select
		// using different attributes as titles in icon view modes
		ShowAttributesMenu();
	}

	// load Tracker add-on menus into main menu bar
	if (ShouldAddMenus() && ShouldHaveAddOnMenus())
		BuildAddOnMenus(fMenuBar);

	CheckScreenIntersect();

	if (fBackgroundImage != NULL && !PoseView()->IsDesktopView() && !isListMode)
		fBackgroundImage->Show(PoseView(), current_workspace());

	Show();

	// done showing, turn the B_NO_WORKSPACE_ACTIVATION flag off;
	// it was on to prevent workspace jerking during boot
	SetFlags(Flags() & ~B_NO_WORKSPACE_ACTIVATION);
}


void
BContainerWindow::InitLayout()
{
	fBorderedView->GroupLayout()->AddView(0, PoseView()->TitleView());

	fCountContainer->GroupLayout()->AddView(PoseView()->CountView(), 0.25f);

	bool forFilePanel = PoseView()->IsFilePanel();
	if (!forFilePanel) {
		// Eliminate the extra borders
		fPoseContainer->GridLayout()->SetInsets(-1, 0, -1, -1);
		fCountContainer->GroupLayout()->SetInsets(0, -1, 0, 0);
	}

	if (PoseView()->VScrollBar() != NULL) {
		fVScrollBarContainer = new BGroupView(B_VERTICAL, 0);
		fVScrollBarContainer->GroupLayout()->AddView(PoseView()->VScrollBar());
		fVScrollBarContainer->GroupLayout()->SetInsets(-1, forFilePanel ? 0 : -1,
			0, 0);
		fPoseContainer->GridLayout()->AddView(fVScrollBarContainer, 1, 1);
	}
	if (PoseView()->HScrollBar() != NULL) {
		BGroupView* hScrollBarContainer = new BGroupView(B_VERTICAL, 0);
		hScrollBarContainer->GroupLayout()->AddView(PoseView()->HScrollBar());
		hScrollBarContainer->GroupLayout()->SetInsets(0, -1, 0,
			forFilePanel ? 0 : -1);
		fCountContainer->GroupLayout()->AddView(hScrollBarContainer);

		BSize size = PoseView()->HScrollBar()->MinSize();
		if (forFilePanel) {
			// Count view height is 1px smaller than scroll bar because it has
			// no upper border.
			size.height -= 1;
		}
		PoseView()->CountView()->SetExplicitMinSize(size);
	}
}


void
BContainerWindow::RestoreState()
{
	UpdateTitle();

	WindowStateNodeOpener opener(this, false);
	RestoreWindowState(opener.StreamNode());
	PoseView()->Init(opener.StreamNode());

	RestoreStateCommon();
}


void
BContainerWindow::RestoreState(const BMessage &message)
{
	UpdateTitle();

	RestoreWindowState(message);
	PoseView()->Init(message);

	RestoreStateCommon();
}


void
BContainerWindow::RestoreStateCommon()
{
	if (fUsesLayout)
		InitLayout();

	if (BootedInSafeMode())
		// don't pick up backgrounds in safe mode
		return;

	bool isDesktop = PoseView()->IsDesktopView();

	WindowStateNodeOpener opener(this, false);
	if (!TargetModel()->IsRoot() && opener.Node() != NULL) {
		// don't pick up background image for root disks
		// to do this, would have to have a unique attribute for the
		// disks window that doesn't collide with the desktop
		// for R4 this was not done to make things simpler
		// the default image will still work though
		fBackgroundImage = BackgroundImage::GetBackgroundImage(opener.Node(), isDesktop);
			// look for background image info in the window's node
	}

	BNode defaultingNode;
	if (fBackgroundImage == NULL && !isDesktop
		&& DefaultStateSourceNode(kDefaultFolderTemplate, &defaultingNode)) {
		// look for background image info in the source for defaults
		fBackgroundImage = BackgroundImage::GetBackgroundImage(&defaultingNode, isDesktop);
	}
}


void
BContainerWindow::OpenParent()
{
	BEntry entry(TargetModel()->EntryRef());
	if (entry.InitCheck() != B_OK)
		return;

	BEntry parentEntry;
	if (FSGetParentVirtualDirectoryAware(entry, parentEntry) != B_OK)
		return;

	entry_ref setToRef;
	parentEntry.GetRef(&setToRef);
	const entry_ref* parent = &setToRef;

	// need to send switch message for spatial mode
	BMessage message(kSwitchDirectory);
	message.AddRef("refs", parent);
	MessageReceived(&message);
}


void
BContainerWindow::SwitchDirectory(const entry_ref* ref)
{
	BEntry entry;
	if (entry.SetTo(ref, true) != B_OK || entry.InitCheck() != B_OK)
		return;

	if (StateNeedsSaving())
		SaveState(false);

	bool wasInTrash = TargetModel()->IsTrash() || TargetModel()->InTrash();
	bool wasRoot = TargetModel()->IsRoot();
	bool wasVolume = TargetModel()->IsVolume();

	// Switch dir and apply new state
	WindowStateNodeOpener opener(this, false);
	opener.SetTo(&entry, false);

	// Update pose view and set directory type
	PoseView()->SwitchDir(ref, opener.StreamNode());

	if (wasInTrash ^ (TargetModel()->IsTrash() || TargetModel()->InTrash())
		|| wasRoot != TargetModel()->IsRoot() || wasVolume != TargetModel()->IsVolume()) {
		RepopulateMenus();
	}

	// skip the rest on file panel
	if (PoseView()->IsFilePanel())
		return;

	// Tracker add-on menus may have changed
	RebuildAddOnMenus(fMenuBar);

	TrackerSettings settings;
	if (settings.ShowNavigator() || settings.ShowFullPathInTitleBar())
		SetPathWatchingEnabled(true);

	SetSingleWindowBrowseShortcuts(settings.SingleWindowBrowse());

	// Update draggable folder icon
	if (fMenuBar != NULL) {
		if (!TargetModel()->IsRoot() && !TargetModel()->IsTrash()) {
			// Folder icon should be visible, but in single
			// window navigation, it might not be.
			if (fDraggableIcon != NULL) {
				IconCache::sIconCache->IconChanged(TargetModel());
				if (fDraggableIcon->IsHidden())
					fDraggableIcon->Show();
				fDraggableIcon->Invalidate();
			} else {
				// draggable icon visible
				_AddFolderIcon();
			}
		} else if (fDraggableIcon != NULL) {
			// hide for Root or Trash
			fDraggableIcon->Hide();
		}
	}

	// Update window title
	UpdateTitle();
}


void
BContainerWindow::UpdateTitle()
{
	// set title to full path, if necessary
	if (TrackerSettings().ShowFullPathInTitleBar()) {
		// use the Entry's full path
		BPath path;
		TargetModel()->GetPath(&path);
		SetTitle(path.Path());
	} else {
		// use the default look
		SetTitle(TargetModel()->Name());
	}

	if (Navigator() != NULL)
		Navigator()->UpdateLocation(TargetModel(), kActionUpdatePath);
}


void
BContainerWindow::UpdateBackgroundImage()
{
	if (BootedInSafeMode())
		return;

	WindowStateNodeOpener opener(this, false);

	if (!TargetModel()->IsRoot() && opener.Node() != NULL) {
		fBackgroundImage = BackgroundImage::Refresh(fBackgroundImage, opener.Node(),
			TargetModel()->IsDesktop(), PoseView());
	}

		// look for background image info in the window's node
	BNode defaultingNode;
	if (!fBackgroundImage && !TargetModel()->IsDesktop()
		&& DefaultStateSourceNode(kDefaultFolderTemplate, &defaultingNode)) {
		// look for background image info in the source for defaults
		fBackgroundImage = BackgroundImage::Refresh(fBackgroundImage, &defaultingNode,
			TargetModel()->IsDesktop(), PoseView());
	}
}


void
BContainerWindow::FrameResized(float, float)
{
	if (PoseView() != NULL && !TargetModel()->IsDesktop()) {
		BRect extent = PoseView()->Extent();
		float offsetX = extent.left - PoseView()->Bounds().left;
		float offsetY = extent.top - PoseView()->Bounds().top;

		// scroll when the size augmented, there is a negative offset
		// and we have resized over the bottom right corner of the extent
		BPoint scroll(B_ORIGIN);
		if (offsetX < 0 && PoseView()->Bounds().right > extent.right
			&& Bounds().Width() > fPreviousBounds.Width()) {
			scroll.x = std::max(fPreviousBounds.Width() - Bounds().Width(),
				offsetX);
		}

		if (offsetY < 0 && PoseView()->Bounds().bottom > extent.bottom
			&& Bounds().Height() > fPreviousBounds.Height()) {
			scroll.y = std::max(fPreviousBounds.Height() - Bounds().Height(),
				offsetY);
		}

		if (scroll != B_ORIGIN)
			PoseView()->ScrollBy(scroll.x, scroll.y);

		PoseView()->UpdateScrollRange();
		PoseView()->ResetPosePlacementHint();
	}

	fPreviousBounds = Bounds();
	if (IsActive())
		fStateNeedsSaving = true;
}


void
BContainerWindow::FrameMoved(BPoint)
{
	if (IsActive())
		fStateNeedsSaving = true;
}


void
BContainerWindow::WorkspacesChanged(uint32, uint32)
{
	if (IsActive())
		fStateNeedsSaving = true;
}


void
BContainerWindow::ViewModeChanged(uint32 oldMode, uint32 newMode)
{
	if (fBackgroundImage == NULL)
		return;

	if (newMode == kListMode)
		fBackgroundImage->Remove();
	else if (oldMode == kListMode)
		fBackgroundImage->Show(PoseView(), current_workspace());
}


void
BContainerWindow::CheckScreenIntersect()
{
	BScreen screen(this);
	BRect screenFrame(screen.Frame());
	BRect frame(Frame());

	if (sNewWindRect.bottom > screenFrame.bottom)
		sNewWindRect.OffsetTo(85, 50);

	if (sNewWindRect.right > screenFrame.right)
		sNewWindRect.OffsetTo(85, 50);

	if (!frame.Intersects(screenFrame))
		MoveTo(sNewWindRect.LeftTop());
}


void
BContainerWindow::SaveState(bool hide)
{
	if (SaveStateIsEnabled()) {
		WindowStateNodeOpener opener(this, true);
		if (opener.StreamNode() != NULL)
			SaveWindowState(opener.StreamNode());

		if (hide)
			Hide();

		if (opener.StreamNode())
			PoseView()->SaveState(opener.StreamNode());

		fStateNeedsSaving = false;
	}
}


void
BContainerWindow::SaveState(BMessage& message) const
{
	if (SaveStateIsEnabled()) {
		SaveWindowState(message);
		PoseView()->SaveState(message);
	}
}


bool
BContainerWindow::StateNeedsSaving() const
{
	return PoseView() != NULL && (fStateNeedsSaving || PoseView()->StateNeedsSaving());
}


status_t
BContainerWindow::GetLayoutState(BNode* node, BMessage* message)
{
	if (node == NULL || message == NULL)
		return B_BAD_VALUE;

	status_t result = node->InitCheck();
	if (result != B_OK)
		return result;

	// ToDo: get rid of this, use AttrStream instead
	node->RewindAttrs();
	char attrName[256];
	while (node->GetNextAttrName(attrName) == B_OK) {
		attr_info info;
		if (node->GetAttrInfo(attrName, &info) != B_OK)
			continue;

		// filter out attributes that are not related to window position
		// and column resizing
		// more can be added as needed
		if (strcmp(attrName, kAttrWindowFrame) != 0
			&& strcmp(attrName, kAttrColumns) != 0
			&& strcmp(attrName, kAttrViewState) != 0
			&& strcmp(attrName, kAttrColumnsForeign) != 0
			&& strcmp(attrName, kAttrViewStateForeign) != 0) {
			continue;
		}

		char* buffer = new char[info.size];
		if (node->ReadAttr(attrName, info.type, 0, buffer,
				(size_t)info.size) == info.size) {
			message->AddData(attrName, info.type, buffer, (ssize_t)info.size);
		}
		delete[] buffer;
	}

	return B_OK;
}


status_t
BContainerWindow::SetLayoutState(BNode* node, const BMessage* message)
{
	status_t result = node->InitCheck();
	if (result != B_OK)
		return result;

	for (int32 globalIndex = 0; ;) {
#if B_BEOS_VERSION_DANO
		const char* name;
#else
		char* name;
#endif
		type_code type;
		int32 count;
		status_t result = message->GetInfo(B_ANY_TYPE, globalIndex, &name,
			&type, &count);
		if (result != B_OK)
			break;

		for (int32 index = 0; index < count; index++) {
			const void* buffer;
			ssize_t size;
			result = message->FindData(name, type, index, &buffer, &size);
			if (result != B_OK) {
				PRINT(("error reading %s \n", name));
				return result;
			}

			if (node->WriteAttr(name, type, 0, buffer,
					(size_t)size) != size) {
				PRINT(("error writing %s \n", name));
				return result;
			}
			globalIndex++;
		}
	}

	return B_OK;
}


bool
BContainerWindow::ShouldAddMenus() const
{
	return true;
}


bool
BContainerWindow::ShouldAddScrollBars() const
{
	return true;
}


Model*
BContainerWindow::TargetModel() const
{
	return PoseView()->TargetModel();
}


void
BContainerWindow::SelectionChanged()
{
}


void
BContainerWindow::Zoom(BPoint, float, float)
{
	BRect oldZoomRect(fSavedZoomRect);
	fSavedZoomRect = Frame();
	ResizeToFit();

	if (fSavedZoomRect == Frame() && oldZoomRect.IsValid())
		ResizeTo(oldZoomRect.Width(), oldZoomRect.Height());
}


void
BContainerWindow::ResizeToFit()
{
	BScreen screen(this);
	BRect screenFrame(screen.Frame());

	screenFrame.InsetBy(5, 5);
	BMessage decoratorSettings;
	GetDecoratorSettings(&decoratorSettings);

	float tabHeight = 15;
	BRect tabRect;
	if (decoratorSettings.FindRect("tab frame", &tabRect) == B_OK)
		tabHeight = tabRect.Height();
	screenFrame.top += tabHeight;

	BRect frame(Frame());

	float widthDiff = frame.Width() - PoseView()->Frame().Width();
	float heightDiff = frame.Height() - PoseView()->Frame().Height();

	// move frame left top on screen
	BPoint leftTop(frame.LeftTop());
	leftTop.ConstrainTo(screenFrame);
	frame.OffsetTo(leftTop);

	// resize to extent size
	BRect extent(PoseView()->Extent());
	frame.right = frame.left + extent.Width() + widthDiff;
	frame.bottom = frame.top + extent.Height() + heightDiff;

	// make sure entire window fits on screen
	frame = frame & screenFrame;

	ResizeTo(frame.Width(), frame.Height());
	MoveTo(frame.LeftTop());
	PoseView()->DisableScrollBars();

	// scroll if there is an offset
	PoseView()->ScrollBy(
		extent.left - PoseView()->Bounds().left,
		extent.top - PoseView()->Bounds().top);

	PoseView()->UpdateScrollRange();
	PoseView()->EnableScrollBars();
}


void
BContainerWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_CUT:
		case B_COPY:
		case B_PASTE:
		case B_SELECT_ALL:
		{
			BView* view = CurrentFocus();
			if (dynamic_cast<BTextView*>(view) == NULL) {
				// The selected item is not a BTextView, so forward the
				// message to the PoseView.
				if (PoseView() != NULL)
					PostMessage(message, PoseView());
			} else {
				// Since we catch the generic clipboard shortcuts in a way that
				// means the BTextView will never get them, we must
				// manually forward them ourselves.
				//
				// However, we have to take care to not forward the custom
				// clipboard messages, else we would wind up in infinite
				// recursion.
				PostMessage(message, view);
			}
			break;
		}

		case kCutMoreSelectionToClipboard:
		case kCopyMoreSelectionToClipboard:
		case kPasteLinksFromClipboard:
			if (PoseView() != NULL)
				PostMessage(message, PoseView());
			break;

		case B_UNDO: {
			BView* view = CurrentFocus();
			if (dynamic_cast<BTextView*>(view) == NULL) {
				FSUndo();
			} else {
				view->MessageReceived(message);
			}
			break;
		}

		case B_REDO: {
			BView* view = CurrentFocus();
			if (dynamic_cast<BTextView*>(view) == NULL) {
				FSRedo();
			} else {
				view->MessageReceived(message);
			}
			break;
		}

		case kOpenParentDir:
			OpenParent();
			break;

		case kNewFolder:
			PostMessage(message, PoseView());
			break;

		case kNewTemplateSubmenu:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK)
				_NewTemplateSubmenu(ref);
			break;
		}

		case kRestoreState:
			if (message->HasMessage("state")) {
				BMessage state;
				message->FindMessage("state", &state);
				Init(&state);
			} else
				Init();
			break;

		case kResizeToFit:
			ResizeToFit();
			break;

		case kLoadAddOn:
			LoadAddOn(message);
			break;

		case kCopySelectionTo:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			BRoster().AddToRecentFolders(&ref);

			Model model(&ref);
			if (model.InitCheck() != B_OK)
				break;

			if (*model.NodeRef() == *TargetModel()->NodeRef())
				PoseView()->DuplicateSelection();
			else
				PoseView()->MoveSelectionInto(&model, this, true);
			break;
		}

		case kMoveSelectionTo:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			BRoster().AddToRecentFolders(&ref);

			Model model(&ref);
			if (model.InitCheck() != B_OK)
				break;

			PoseView()->MoveSelectionInto(&model, this, false, true);
			break;
		}

		case kCreateLink:
		case kCreateRelativeLink:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BRoster().AddToRecentFolders(&ref);

				Model model(&ref);
				if (model.InitCheck() != B_OK)
					break;

				PoseView()->MoveSelectionInto(&model, this, false, false,
					message->what == kCreateLink,
					message->what == kCreateRelativeLink);
			} else if (!TargetModel()->IsQuery() && !TargetModel()->IsVirtualDirectory()) {
				// no destination specified, create link in same dir as item
				PoseView()->MoveSelectionInto(TargetModel(), this, false, false,
					message->what == kCreateLink,
					message->what == kCreateRelativeLink);
			}
			break;
		}

		case kShowSelectionWindow:
			ShowSelectionWindow();
			break;

		case kSelectMatchingEntries:
			PoseView()->SelectMatchingEntries(message);
			break;

		case kFindButton:
			(new FindWindow())->Show();
			break;

		case kQuitTracker:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case kRestoreBackgroundImage:
			UpdateBackgroundImage();
			break;

		case kSwitchDirectory:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			if (!PoseView()->IsFilePanel() && !TrackerSettings().SingleWindowBrowse()) {
				message->what = B_REFS_RECEIVED;
				const node_ref* nodeRef = TargetModel()->NodeRef();

				// add information about the child, so that we can select it in the parent view
				message->AddData("nodeRefToSelect", B_RAW_TYPE, nodeRef, sizeof(node_ref));

				if ((modifiers() & B_OPTION_KEY) != 0) {
					// if option down, add instructions to close the parent
					message->AddData("nodeRefsToClose", B_RAW_TYPE, nodeRef, sizeof(node_ref));
				}

				be_app->PostMessage(message);
			} else {
				SwitchDirectory(&ref);

				if (Navigator() != NULL) {
					// update Navigation bar
					int32 action = message->GetInt32("action", kActionSet);
					Navigator()->UpdateLocation(TargetModel(), action);
				}
			}
			break;
		}

		case B_REFS_RECEIVED:
		{
			if (PoseView() == NULL || !PoseView()->IsDragging())
				break;

			// ref in this message is the target, the end point of the drag
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				PoseView()->SetWaitingForRefs(false);
				BEntry entry(&ref, true);
				// don't copy to printers dir
				if (entry.InitCheck() == B_OK && entry.IsDirectory() && !FSIsPrintersDir(&entry)) {
					Model target(&entry, true, false);
					BPoint where;
					uint32 buttons;
					PoseView()->GetMouse(&where, &buttons, true);
					PoseView()->HandleDropCommon(PoseView()->DragMessage(), &target, NULL,
						PoseView(), where);
				}
			}

			PoseView()->DragStop();
			break;
		}

		case B_TRACKER_ADDON_MESSAGE:
		{
			_PassMessageToAddOn(message);
			break;
		}

		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 observerWhat;
			if (message->FindInt32("be:observe_change_what", &observerWhat)
					== B_OK) {
				TrackerSettings settings;
				switch (observerWhat) {
					case kWindowsShowFullPathChanged:
						UpdateTitle();
						if (!IsPathWatchingEnabled()
							&& settings.ShowFullPathInTitleBar()) {
							SetPathWatchingEnabled(true);
						}
						if (IsPathWatchingEnabled()
							&& !(settings.ShowNavigator() || settings.ShowFullPathInTitleBar())) {
							SetPathWatchingEnabled(false);
						}
						break;

					case kSingleWindowBrowseChanged:
						if (PoseView()->IsFilePanel() || PoseView()->IsDesktopView()) {
							SetSingleWindowBrowseShortcuts(settings.SingleWindowBrowse());
							break;
						}

						if (settings.SingleWindowBrowse() && Navigator() == NULL
							&& TargetModel()->IsDirectory()) {
							fNavigator = new BNavigator(TargetModel());
							fPoseContainer->GridLayout()->AddView(fNavigator, 0, 0, 2);
							fNavigator->Hide();
							SetPathWatchingEnabled(settings.ShowNavigator()
								|| settings.ShowFullPathInTitleBar());
						}

						if (!settings.SingleWindowBrowse() && fWindowList != NULL) {
							// close duplicate windows
							int32 windowCount = fWindowList->CountItems();
							BObjectList<BContainerWindow> containerList(windowCount);
							for (int32 index = 0; index < windowCount; index++) {
								BContainerWindow* window
									= dynamic_cast<BContainerWindow*>(fWindowList->ItemAt(index));
								if (window != NULL && window->TargetModel() != NULL
									&& window->TargetModel()->NodeRef() != NULL) {
									containerList.AddItem(window);
								}
							}

							windowCount = containerList.CountItems();
								// get the window count again as it may have changed
							if (windowCount > 1) {
								// sort containerList by node ref
								containerList.SortItems(CompareContainerWindowNodeRef);

								// go backwards from second to last item to first
								for (int32 index = windowCount - 2; index >= 0; --index) {
									BContainerWindow* window = containerList.ItemAt(index);
									BContainerWindow* second = containerList.ItemAt(index + 1);
									if (window == NULL || second == NULL)
										continue;

									const node_ref* windowRef = window->TargetModel()->NodeRef();
									const node_ref* secondRef = second->TargetModel()->NodeRef();
									if (*windowRef == *secondRef) {
										// duplicate windows found, close second window
										// use BMessenger::SendMessage(), safer than PostMessage()
										BMessenger(second).SendMessage(B_QUIT_REQUESTED);
									}
								}
							}
						}

						if (!settings.SingleWindowBrowse() && TargetModel()->IsDesktop()) {
							// close the "Desktop" window, but not the Desktop
							this->Quit();
						}

						SetSingleWindowBrowseShortcuts(settings.SingleWindowBrowse());
						break;

					case kShowNavigatorChanged:
						ShowNavigator(settings.ShowNavigator());
						if (!IsPathWatchingEnabled() && settings.ShowNavigator())
							SetPathWatchingEnabled(true);
						if (IsPathWatchingEnabled()
							&& !(settings.ShowNavigator() || settings.ShowFullPathInTitleBar())) {
							SetPathWatchingEnabled(false);
						}
						SetSingleWindowBrowseShortcuts(settings.SingleWindowBrowse());
						break;

					default:
						_inherited::MessageReceived(message);
						break;
				}
			}
			break;
		}

		case B_NODE_MONITOR:
			UpdateTitle();
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


bool
BContainerWindow::IsShowing(const node_ref* node) const
{
	return PoseView()->Represents(node);
}


bool
BContainerWindow::IsShowing(const entry_ref* entry) const
{
	return PoseView()->Represents(entry);
}


void
BContainerWindow::AddMenus()
{
	fFileMenu = new TLiveFileMenu(B_TRANSLATE("File"), this);
	AddFileMenu(fFileMenu);
	fMenuBar->AddItem(fFileMenu, 0);

	fWindowMenu = new TLiveWindowMenu(B_TRANSLATE("Window"), this);
	fMenuBar->AddItem(fWindowMenu, 1);
	AddWindowMenu(fWindowMenu);

	// create Attributes menu, add it later
	fAttrMenu = new BMenu(B_TRANSLATE("Attributes"));
	NewAttributesMenu(fAttrMenu);

	// create "Arrange By >" menu, add it later
	fArrangeByItem = NewArrangeByMenu();
}


void
BContainerWindow::AddFileMenu(BMenu* menu)
{
	if (TargetModel()->IsTrash()) {
		// add as first item in menu
		menu->AddItem(Shortcuts()->EmptyTrashItem());
		menu->AddItem(new BSeparatorItem());
	} else if (TargetModel()->IsPrintersDir()) {
		// add as first item in menu
		menu->AddItem(Shortcuts()->AddPrinterItem());
		menu->AddItem(new BSeparatorItem());
	}

	menu->AddItem(Shortcuts()->FindItem());
	if (ShouldHaveNewFolderItem())
		menu->AddItem(Shortcuts()->NewFolderItem());
	menu->AddSeparatorItem();

	menu->AddItem(Shortcuts()->OpenItem());
	// "Edit query" and "Open with..." inserted here,
	// see UpdateMenu(), SetupEditQueryItem() and SetupOpenWithMenu()
	menu->AddItem(Shortcuts()->GetInfoItem());
	menu->AddItem(Shortcuts()->EditNameItem());

	if (TargetModel()->IsPrintersDir()) {
		menu->AddItem(Shortcuts()->MakeActivePrinterItem());
		return; // we're done with printers directory
	}

	// "Mount >" menu and "Unmount" are inserted here,
	// see UpdateMenu() and SetupMountMenu()

	if (!TargetModel()->IsRoot()) {
		if (TargetModel()->IsTrash()) {
			menu->AddItem(Shortcuts()->DeleteItem());
			menu->AddItem(Shortcuts()->RestoreItem());
		} else {
			menu->AddItem(Shortcuts()->DuplicateItem());
			menu->AddItem(Shortcuts()->MoveToTrashItem());
		}
	}

	menu->AddSeparatorItem();

	// The "Move To", "Copy To", "Create Link" menus are inserted here,
	// have a look at UpdateMenu() and SetupMoveCopyMenus().

	if (!TargetModel()->IsRoot() && !TargetModel()->IsTrash() && !TargetModel()->InTrash()) {
		menu->AddItem(Shortcuts()->CutItem());
		menu->AddItem(Shortcuts()->CopyItem());
		menu->AddItem(Shortcuts()->PasteItem());
		menu->AddSeparatorItem();
	}

	if (!TargetModel()->IsRoot())
		menu->AddItem(Shortcuts()->IdentifyItem());
	if (ShouldHaveAddOnMenus())
		menu->AddItem(new BMenuItem(new BMenu(Shortcuts()->AddOnsLabel())));
}


void
BContainerWindow::AddWindowMenu(BMenu* menu)
{
	AddIconSizeMenu(menu);

	BMenuItem* item = new BMenuItem(B_TRANSLATE("List view"), new BMessage(kListMode), '3');
	item->SetTarget(PoseView());
	menu->AddItem(item);
	menu->AddSeparatorItem();

	menu->AddItem(Shortcuts()->ResizeToFitItem());
	// "Arrange by >" menu inserted here,
	// see UpdateMenu() and SetupArrangeByMenu()
	menu->AddItem(Shortcuts()->SelectItem());
	menu->AddItem(Shortcuts()->SelectAllItem());
	menu->AddItem(Shortcuts()->InvertSelectionItem());
	menu->AddItem(Shortcuts()->OpenParentItem());
	menu->AddItem(Shortcuts()->CloseItem());
	menu->AddItem(Shortcuts()->CloseAllInWorkspaceItem());
	menu->AddSeparatorItem();

	item = new BMenuItem("Preferences" B_UTF8_ELLIPSIS, new BMessage(kShowSettingsWindow), ',');
	item->SetTarget(be_app);
	menu->AddItem(item);
}


void
BContainerWindow::AddIconSizeMenu(BMenu* menu)
{
	if (menu == NULL)
		return;

	BMenuItem* item;
	BMessage* message;

	BMenu* iconSizeMenu = new BMenu(B_TRANSLATE("Icon view"));
	iconSizeMenu->SetRadioMode(true);

	static const uint32 kIconSizes[] = { 32, 40, 48, 64, 96, 128 };

	BString label;
	const char* format;
	const char* comment = "The '×' is the Unicode multiplication sign U+00D7";
	uint32 iconSize;
	for (uint32 i = 0; i < sizeof(kIconSizes) / sizeof(uint32); ++i) {
		iconSize = kIconSizes[i];
		message = new BMessage(kIconMode);
		message->AddInt32("size", iconSize);
		format = B_TRANSLATE_COMMENT("%" B_PRId32 " × %" B_PRId32, comment);
		label.SetToFormat(format, iconSize, iconSize);
		item = new BMenuItem(label, message);
		item->SetTarget(PoseView());
		iconSizeMenu->AddItem(item);
	}

	iconSizeMenu->AddSeparatorItem();

	message = new BMessage(kIconMode);
	message->AddInt32("scale", 0);
	item = new BMenuItem(B_TRANSLATE("Decrease size"), message, '-');
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("scale", 1);
	item = new BMenuItem(B_TRANSLATE("Increase size"), message, '+');
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	// A sub menu where the super item can be invoked.
	menu->AddItem(iconSizeMenu);
	BMenuItem* iconSizeSuperItem = iconSizeMenu->Superitem();
	if (iconSizeSuperItem != NULL) {
		iconSizeSuperItem->SetShortcut('1', B_COMMAND_KEY);
		iconSizeSuperItem->SetMessage(new BMessage(kIconMode));
		iconSizeSuperItem->SetTarget(PoseView());
	}

	item = new BMenuItem(B_TRANSLATE("Mini icon view"), new BMessage(kMiniIconMode), '2');
	item->SetTarget(PoseView());
	menu->AddItem(item);
}


void
BContainerWindow::AddShortcuts()
{
	// add equivalents of the menu shortcuts to the menuless desktop window
	ASSERT(!TargetModel()->IsTrash());
	ASSERT(!PoseView()->IsFilePanel());
	ASSERT(!TargetModel()->IsQuery());
	ASSERT(!TargetModel()->IsVirtualDirectory());

	AddShortcut('X', B_COMMAND_KEY | B_SHIFT_KEY,
		new BMessage(kCutMoreSelectionToClipboard), this);
	AddShortcut('C', B_COMMAND_KEY | B_SHIFT_KEY,
		new BMessage(kCopyMoreSelectionToClipboard), this);
	AddShortcut('F', B_COMMAND_KEY, new BMessage(kFindButton), PoseView());
	AddShortcut('N', B_COMMAND_KEY, new BMessage(kNewFolder), PoseView());
	AddShortcut('O', B_COMMAND_KEY, new BMessage(kOpenSelection), PoseView());
	AddShortcut('I', B_COMMAND_KEY, new BMessage(kGetInfo), PoseView());
	AddShortcut('E', B_COMMAND_KEY, new BMessage(kEditName), PoseView());
	AddShortcut('D', B_COMMAND_KEY, new BMessage(kDuplicateSelection), PoseView());
	AddShortcut(B_DELETE, B_NO_COMMAND_KEY, new BMessage(kMoveSelectionToTrash), PoseView());
	AddShortcut('K', B_COMMAND_KEY, new BMessage(kCleanup), PoseView());
	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_SELECT_ALL), PoseView());
	AddShortcut('S', B_COMMAND_KEY, new BMessage(kInvertSelection), PoseView());
	AddShortcut('A', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kShowSelectionWindow), PoseView());
	AddShortcut('G', B_COMMAND_KEY, new BMessage(kEditQuery), PoseView());
		// it is ok to add a global Edit query shortcut here, PoseView will
		// filter out cases where selected pose is not a query
	AddShortcut('U', B_COMMAND_KEY, new BMessage(kUnmountVolume), PoseView());
	AddShortcut(B_UP_ARROW, B_COMMAND_KEY, new BMessage(kOpenParentDir), PoseView());
	AddShortcut('O', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage(kOpenSelectionWith), PoseView());

	BMessage* decreaseSize = new BMessage(kIconMode);
	decreaseSize->AddInt32("scale", 0);
	AddShortcut('-', B_COMMAND_KEY, decreaseSize, PoseView());

	BMessage* increaseSize = new BMessage(kIconMode);
	increaseSize->AddInt32("scale", 1);
	AddShortcut('+', B_COMMAND_KEY, increaseSize, PoseView());
}


void
BContainerWindow::MenusBeginning()
{
	if (fMenuBar == NULL)
		return;

	if (CurrentMessage() != NULL && CurrentMessage()->what == B_MOUSE_DOWN) {
		// don't commit active pose if only a keyboard shortcut is
		// invoked - this would prevent Cut/Copy/Paste from working
		PoseView()->CommitActivePose();
	}

	if (fFileMenu != NULL)
		UpdateMenu(fFileMenu, kFileMenuContext);

	if (fWindowMenu != NULL)
		UpdateMenu(fWindowMenu, kWindowMenuContext);

	if (system_time() - fLastMenusBeginningTime > 50000) {
		// Tracker add-on menus may have changed
		RebuildAddOnMenus(fMenuBar);
	}

	// prevent Add-ons from being rebuilt too fast
	fLastMenusBeginningTime = system_time();
}


void
BContainerWindow::MenusEnded()
{
	// when we're done we want to clear nav menus for next time
	DeleteSubmenu(fCreateLinkItem);
	DeleteSubmenu(fCopyToItem);
	DeleteSubmenu(fMoveToItem);
	DeleteSubmenu(fOpenWithItem);
	DeleteSubmenu(fNavigationItem);
	DeleteSubmenu(fMountItem);
}


void
BContainerWindow::SetupNavigationMenu(BMenu* parent, const entry_ref* ref)
{
	ASSERT(parent != NULL);

	// start by removing nav item (and separator) from old menu
	if (fNavigationItem != NULL && fNavigationItem->Menu() != NULL) {
		// delete separator first
		delete fNavigationItem->Menu()->RemoveItem(
			fNavigationItem->Menu()->IndexOf(fNavigationItem) + 1);
		fNavigationItem->Menu()->RemoveItem(fNavigationItem);
	}

	// if we weren't passed a ref then we're navigating this window
	if (ref == NULL)
		ref = TargetModel()->EntryRef();

	// bail out if we shouldn't have a navigation item
	if (!ShouldHaveNavigationMenu(ref))
		return;

	BEntry entry;
	if (entry.SetTo(ref) != B_OK)
		return;

	// only navigate directories and queries (check for symlink here)
	Model model(&entry);
	entry_ref resolvedRef;

	if (model.InitCheck() != B_OK)
		return;
	else if (!model.IsContainer() && !model.IsSymLink())
		return;

	if (model.IsSymLink()) {
		if (entry.SetTo(model.EntryRef(), true) != B_OK)
			return;

		Model resolvedModel(&entry);
		if (resolvedModel.InitCheck() != B_OK || !resolvedModel.IsContainer())
			return;

		entry.GetRef(&resolvedRef);
		ref = &resolvedRef;
	}

	// always build a fresh navigation menu
	delete fNavigationItem;
	fNavigationItem
		= new ModelMenuItem(&model, new BNavMenu(model.Name(), B_REFS_RECEIVED, be_app, this));

	// setup a navigation menu item which will dynamically load items
	// as menu items are traversed
	BNavMenu* navMenu = dynamic_cast<BNavMenu*>(fNavigationItem->Submenu());
	navMenu->SetNavDir(ref);
	fNavigationItem->SetLabel(model.Name());
	fNavigationItem->SetEntry(&entry);

	parent->AddItem(fNavigationItem, 0);
	parent->AddItem(new BSeparatorItem(), 1);

	BMessage* message = new BMessage(B_REFS_RECEIVED);
	message->AddRef("refs", ref);
	fNavigationItem->SetMessage(message);
	fNavigationItem->SetTarget(be_app);

	if (!PoseView()->IsDragging())
		parent->SetTrackingHook(NULL, NULL);
}


void
BContainerWindow::SetupEditQueryItem(BMenu* parent)
{
	SetupEditQueryItem(parent, TargetModel()->EntryRef());
}


void
BContainerWindow::SetupEditQueryItem(BMenu* parent, const entry_ref* ref)
{
	ASSERT(parent != NULL);

	// start by removing "Edit query" from old menu
	if (fEditQueryItem != NULL && fEditQueryItem->Menu() != NULL)
		fEditQueryItem->Menu()->RemoveItem(fEditQueryItem);

	// if ref unset assume window ref
	if (ref == NULL)
		ref = TargetModel()->EntryRef();

	ASSERT(ref);

	// bail out if "Open" item or index not found
	BMenuItem* openItem = parent->FindItem(kOpenSelection);
	int32 openIndex = parent->IndexOf(openItem);
	if (openItem == NULL || openIndex == B_ERROR)
		return;

	// add "Edit query" item between "Open" and "Open with..."
	parent->AddItem(fEditQueryItem, openIndex + 1);

	Shortcuts()->UpdateEditQueryItem(fEditQueryItem);
}


void
BContainerWindow::SetupOpenWithMenu(BMenu* parent)
{
	SetupOpenWithMenu(parent, TargetModel()->EntryRef());
}


void
BContainerWindow::SetupOpenWithMenu(BMenu* parent, const entry_ref* ref)
{
	ASSERT(parent != NULL);

	// start by removing "Open with..." item from old menu
	if (fOpenWithItem != NULL && fOpenWithItem->Menu() != NULL)
		fOpenWithItem->Menu()->RemoveItem(fOpenWithItem);

	// ToDo:
	// check if only item in selection list is the root
	// and do not add if true

	// if ref unset assume window ref
	if (ref == NULL)
		ref = TargetModel()->EntryRef();

	ASSERT(ref != NULL);

	// bail out if we shouldn't have an "Open with..." menu
	if (!ShouldHaveOpenWithMenu(ref))
		return;

	// bail out if "Open" item not found
	BMenuItem* openItem = parent->FindItem(kOpenSelection);
	if (openItem == NULL)
		return;

	// bail out if index of "Open" item not found
	int32 openIndex = parent->IndexOf(openItem);
	if (openIndex == B_ERROR)
		return;

	// build a list of all refs to open
	BMessage message(B_REFS_RECEIVED);
	BPose* pose;
	int32 selectCount = PoseView()->CountSelected();
	for (int32 index = 0; index < selectCount; index++) {
		pose = PoseView()->SelectionList()->ItemAt(index);
		message.AddRef("refs", pose->TargetModel()->EntryRef());
	}

	// add Tracker token so that refs received recipients can script us
	message.AddMessenger("TrackerViewToken", BMessenger(PoseView()));

	// always build a fresh "Open with..." menu
	delete fOpenWithItem;
	fOpenWithItem = Shortcuts()->OpenWithItem(
		new OpenWithMenu(Shortcuts()->OpenWithLabel(), &message, this, be_app));

	parent->AddItem(fOpenWithItem, openIndex + 1);
	Shortcuts()->UpdateOpenWithItem(fOpenWithItem);
}


void
BContainerWindow::SetupNewTemplatesMenu(BMenu* parent, MenuContext context)
{
	ASSERT(parent != NULL);

	// start by removing "New >" item from the old menu
	if (fNewTemplatesItem != NULL && fNewTemplatesItem->Menu() != NULL)
		fNewTemplatesItem->Menu()->RemoveItem(fNewTemplatesItem);

	// no "New folder" item or menu for this window type, bail
	if (!ShouldHaveNewFolderItem())
		return;

	// file panel does not have a menu, only "New folder"
	BMenuItem* newFolderItem = parent->FindItem(kNewFolder);
	if (PoseView()->IsFilePanel())
		return Shortcuts()->UpdateNewFolderItem(newFolderItem);

	// we should have a "New >" menu at this point
	TemplatesMenu* newTemplatesMenu = (TemplatesMenu*)fNewTemplatesItem->Submenu();
	ASSERT(newTemplatesMenu != NULL);

	// update templates menu state
	newTemplatesMenu->UpdateMenuState();

	// no templates found, update "New folder" instead, bail
	if (newTemplatesMenu->CountTemplates() == 0)
		return Shortcuts()->UpdateNewFolderItem(newFolderItem);

	int32 newFolderIndex = parent->IndexOf(newFolderItem);
	if (newFolderItem != NULL && newFolderIndex != B_ERROR) {
		// replace "New folder" with "New >" menu
		parent->RemoveItem(newFolderItem);
		parent->AddItem(fNewTemplatesItem, newFolderIndex);
		delete newFolderItem;
	} else {
		// we already have a "New >" menu
		if (context == kWindowPopUpContext)
			parent->AddItem(fNewTemplatesItem, 2);
		else if (context == kFileMenuContext)
			parent->AddItem(fNewTemplatesItem, 1);
	}

	// update "New >" menu status
	Shortcuts()->UpdateNewTemplatesItem(fNewTemplatesItem);
}


void
BContainerWindow::SetupMountMenu(BMenu* parent, MenuContext context)
{
	SetupMountMenu(parent, context, TargetModel()->EntryRef());
}


void
BContainerWindow::SetupMountMenu(BMenu* parent, MenuContext context, const entry_ref* ref)
{
	ASSERT(parent != NULL);

	// Remove "Mount", "Unmount" and separator item from the old menu
	if (fMountItem != NULL && fMountItem->Menu() != NULL)
		fMountItem = DetachMountMenu();

	if (ref == NULL)
		ref = TargetModel()->EntryRef();

	Model model(ref);

	// bail out if not Desktop, root or volume
	if (!(model.IsDesktop() || model.IsRoot() || model.IsVolume()))
		return;

	// Insert "Mount >" menu after "Select all" on Desktop,
	// or after "Edit name" on volume/root menu.
	// Add-ons and custom Tracker add-ons gets added after this.
	int32 mountIndex = parent->CountItems() - 1;
		// fall back to last item in the menu if all else fails
	if (model.IsDesktop()) {
		BMenuItem* selectAll = parent->FindItem(B_SELECT_ALL);
		if (selectAll != NULL)
			mountIndex = parent->IndexOf(selectAll) + 2;
				// skip separator
	} else if (model.IsRoot() || model.IsVolume()) {
		BMenuItem* editName = parent->FindItem(kEditName);
		if (editName != NULL)
			mountIndex = parent->IndexOf(editName) + 2;
				// skip separator
	}

	delete fMountItem;
	fMountItem = Shortcuts()->MountItem(new MountMenu(Shortcuts()->MountLabel()));
	parent->AddItem(fMountItem, mountIndex);

	if (model.IsDesktop()
		|| (model.IsRoot() && (context == kWindowPopUpContext || context == kPosePopUpContext))) {
		// No "Unmount", add separator item only
		parent->AddItem(new BSeparatorItem(), mountIndex + 1);
	} else {
		// Add "Unmount" and separator
		BMenuItem* unmountItem = Shortcuts()->UnmountItem();
		parent->AddItem(unmountItem, mountIndex + 1);
		Shortcuts()->UpdateUnmountItem(unmountItem);
		parent->AddItem(new BSeparatorItem(), mountIndex + 2);
	}
}


BMenuItem*
BContainerWindow::DetachMountMenu()
{
	BMenu* menu = fMountItem->Menu();
	int32 mountIndex = menu->IndexOf(fMountItem);
	if (mountIndex == B_ERROR)
		return NULL;

	// remove items in reverse chronological order

	// we may not have "Unmount", look for item with U shortcut
	if (menu->ItemAt(mountIndex + 1) != NULL && menu->ItemAt(mountIndex + 1)->Shortcut() == 'U') {
		// remove and delete separator
		BMenuItem* separator = menu->RemoveItem(mountIndex + 2);
		if (dynamic_cast<BSeparatorItem*>(separator) != NULL)
			delete separator;
		// remove and delete "Unmount"
		delete menu->RemoveItem(mountIndex + 1);
	} else {
		// remove and delete separator
		BMenuItem* separator = menu->RemoveItem(mountIndex + 1);
		if (dynamic_cast<BSeparatorItem*>(separator) != NULL)
			delete separator;
	}

	// remove "Mount >" menu item from parent and return it
	return menu->RemoveItem(mountIndex);
}


void
BContainerWindow::PopulateMoveCopyNavMenu(BNavMenu* navMenu, uint32 what, const entry_ref* ref,
	bool addLocalOnly)
{
	BVolume volume;
	BVolumeRoster volumeRoster;
	BDirectory directory;
	BEntry entry;
	BPath path;
	Model model;
	dev_t device = ref->device;

	int32 volumeCount = 0;

	navMenu->RemoveItems(0, navMenu->CountItems(), true);

	// count persistent writable volumes
	volumeRoster.Rewind();
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		if (!volume.IsReadOnly() && volume.IsPersistent())
			volumeCount++;
	}

	// add the current folder
	if (entry.SetTo(ref) == B_OK
		&& entry.GetParent(&entry) == B_OK
		&& model.SetTo(&entry) == B_OK) {
		BNavMenu* menu = new BNavMenu(B_TRANSLATE("Current folder"), what,
			this);
		menu->SetNavDir(model.EntryRef());
		menu->SetShowParent(true);

		BMenuItem* item = new SpecialModelMenuItem(&model, menu);
		item->SetMessage(new BMessage((uint32)what));

		navMenu->AddItem(item);
	}

	// add the recent folder menu
	// the "Tracker" settings directory is only used to get its icon
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("Tracker");
		if (entry.SetTo(path.Path()) == B_OK
			&& model.SetTo(&entry) == B_OK) {
			BMenu* menu = new RecentsMenu(B_TRANSLATE("Recent folders"),
				kRecentFolders, what, this);

			BMenuItem* item = new SpecialModelMenuItem(&model, menu);
			item->SetMessage(new BMessage((uint32)what));

			navMenu->AddItem(item);
		}
	}

	// add Desktop
	FSGetBootDeskDir(&directory);
	if (directory.InitCheck() == B_OK && directory.GetEntry(&entry) == B_OK
		&& model.SetTo(&entry) == B_OK) {
		navMenu->AddNavDir(&model, what, this, true);
			// ask NavMenu to populate submenu for us
	}

	// add the home dir
	if (find_directory(B_USER_DIRECTORY, &path) == B_OK
		&& entry.SetTo(path.Path()) == B_OK && model.SetTo(&entry) == B_OK) {
		navMenu->AddNavDir(&model, what, this, true);
	}

	navMenu->AddSeparatorItem();

	// either add all mounted volumes (for copy), or all the top-level
	// directories from the same device (for move)
	// ToDo: can be changed if cross-device moves are implemented

	if (addLocalOnly || volumeCount < 2) {
		// add volume this item lives on
		if (volume.SetTo(device) == B_OK
			&& volume.GetRootDirectory(&directory) == B_OK
			&& directory.GetEntry(&entry) == B_OK
			&& model.SetTo(&entry) == B_OK) {
			navMenu->AddNavDir(&model, what, this, false);
				// do not have submenu populated

			navMenu->SetNavDir(model.EntryRef());
		}
	} else {
		// add all persistent writable volumes
		volumeRoster.Rewind();
		while (volumeRoster.GetNextVolume(&volume) == B_OK) {
			if (volume.IsReadOnly() || !volume.IsPersistent())
				continue;

			// add root dir
			if (volume.GetRootDirectory(&directory) == B_OK
				&& directory.GetEntry(&entry) == B_OK
				&& model.SetTo(&entry) == B_OK) {
				navMenu->AddNavDir(&model, what, this, true);
					// ask NavMenu to populate submenu for us
			}
		}
	}
}


void
BContainerWindow::SetupMoveCopyMenus(BMenu* parent, const entry_ref* ref)
{
	// bail out if items are not setup yet, this should never happen
	if (fMoveToItem == NULL || fCopyToItem == NULL || fCreateLinkItem == NULL)
		return;

	// start off by removing the items from the old menu
	if (fMoveToItem->Menu() != NULL)
		fMoveToItem->Menu()->RemoveItem(fMoveToItem);
	if (fCopyToItem->Menu() != NULL)
		fCopyToItem->Menu()->RemoveItem(fCopyToItem);
	if (fCreateLinkItem->Menu() != NULL) {
		// remove and delete separator first
		delete fCreateLinkItem->Menu()->RemoveItem(
			fCreateLinkItem->Menu()->IndexOf(fCreateLinkItem) + 1);
		fCreateLinkItem->Menu()->RemoveItem(fCreateLinkItem);
	}

	// if ref unset assume window ref
	if (ref == NULL)
		ref = TargetModel()->EntryRef();

	// bail out if we shouldn't have "Move to/Copy to/Create Link" menus
	if (!ShouldHaveMoveCopyMenus(ref))
		return;

	// bail out if "Move to Trash" or "Delete" item not found
	BMenuItem* moveToTrashItem
		= Shortcuts()->FindItem(parent, kMoveSelectionToTrash, kDeleteSelection);
	int32 index = parent->IndexOf(moveToTrashItem);
	if (moveToTrashItem == NULL || index == B_ERROR)
		return;

	// skip past "Move to Trash" and separator
	index += 2;

	// re-parent items to this menu since they're shared
	parent->AddItem(fMoveToItem, index++);
	parent->AddItem(fCopyToItem, index++);
	parent->AddItem(fCreateLinkItem, index++);
	parent->AddItem(new BSeparatorItem(), index);

	// Set the "Create Link" item label here so it
	// appears correctly when menus are disabled, too.
	Shortcuts()->UpdateCreateLinkItem(fCreateLinkItem);

	// only enable once the menus are built
	fMoveToItem->SetEnabled(false);
	fCopyToItem->SetEnabled(false);
	fCreateLinkItem->SetEnabled(false);

	// not for root or Trash or trashed items
	Model model(ref);
	if (model.IsRoot() || model.IsTrash() || model.InTrash())
		return;

	// configure "Move to" menu item
	PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu*>(fMoveToItem->Submenu()), kMoveSelectionTo, ref,
		true);

	// configure "Copy to" menu item
	// add all mounted volumes (except the one this item lives on)
	PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu*>(fCopyToItem->Submenu()), kCopySelectionTo, ref,
		false);

	// Set "Create Link" menu item message and
	// add all mounted volumes (except the one this item lives on)
	PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu*>(fCreateLinkItem->Submenu()),
		(modifiers() & B_SHIFT_KEY) != 0 ? kCreateRelativeLink : kCreateLink, ref, false);

	Shortcuts()->UpdateMoveToItem(parent->FindItem(kMoveSelectionTo));
	Shortcuts()->UpdateCopyToItem(parent->FindItem(kCopySelectionTo));
	Shortcuts()->UpdateCreateLinkItem(parent->FindItem(Shortcuts()->CreateLinkCommand()));
}


uint32
BContainerWindow::ShowDropContextMenu(BPoint where, BPoseView* source)
{
	BPoint global(where);

	PoseView()->ConvertToScreen(&global);
	PoseView()->CommitActivePose();

	Shortcuts()->UpdateCreateLinkHereItem(
		fDropContextMenu->FindItem(Shortcuts()->CreateLinkHereCommand()));

	BMenuItem* item;

	int32 itemCount = fDropContextMenu->CountItems();
	for (int32 index = 0; index < itemCount - 2; index++) {
		// separator item and Cancel item are skipped
		item = fDropContextMenu->ItemAt(index);
		if (item == NULL)
			break;

		if (item->Command() == kMoveSelectionTo && source != NULL) {
			// check that both source and destination volumes are not read-only
			item->SetEnabled(PoseView()->TargetVolumeIsReadOnly() == false
				&& source->SelectedVolumeIsReadOnly() == false);
		} else {
			// check that target volume is not read-only
			item->SetEnabled(PoseView()->TargetVolumeIsReadOnly() == false);
		}
	}

	item = fDropContextMenu->Go(global, true, true);
	if (item != NULL)
		return item->Command();

	return 0;
}


void
BContainerWindow::ShowContextMenu(BPoint where, const entry_ref* ref)
{
	ASSERT(IsLocked());
	BPoint global(where);
	PoseView()->ConvertToScreen(&global);
	PoseView()->CommitActivePose();

	if (ref != NULL) {
		// clicked on a pose, show file or volume context menu
		Model model(ref);
		if (model.InitCheck() != B_OK)
			return; // bail out, do not show context menu

		if (PoseView()->IsDragging()) {
			fContextMenu = NULL;

			BEntry entry;
			model.GetEntry(&entry);

			// only show for directories (directory, volume, root)
			//
			// don't show a popup for the trash or printers
			// trash is handled in DeskWindow
			//
			// since this menu is opened asynchronously
			// we need to make sure we don't open it more
			// than once, the IsShowing flag is set in
			// SlowContextPopup::AttachedToWindow and
			// reset in DetachedFromWindow
			// see the notes in SlowContextPopup::AttachedToWindow

			if (!FSIsPrintersDir(&entry) && !fDragContextMenu->IsShowing()) {
				fDragContextMenu->ClearMenu();

				// in case the ref is a symlink, resolve it
				// only pop open for directories
				BEntry resolvedEntry(ref, true);
				if (!resolvedEntry.IsDirectory())
					return;

				entry_ref resolvedRef;
				resolvedEntry.GetRef(&resolvedRef);

				// use the resolved ref for the menu
				fDragContextMenu->SetNavDir(&resolvedRef);
				fDragContextMenu->SetTypesList(PoseView()->CachedTypesList());
				fDragContextMenu->SetTarget(BMessenger(this));
				BPoseView* poseView = PoseView();
				if (poseView != NULL) {
					BMessenger target(poseView);
					fDragContextMenu->InitTrackingHook(&BPoseView::MenuTrackingHook, &target,
						poseView->DragMessage());
				}

				// this is now asynchronous so that we don't
				// deadlock in Window::Quit,
				fDragContextMenu->Go(global);
			}

			return;
		}

		if (model.IsTrash())
			fContextMenu = fTrashContextMenu;
		else if (model.IsVolume() || model.IsRoot())
			fContextMenu = fVolumeContextMenu;
		else
			fContextMenu = fPoseContextMenu;

		// bail out before cleanup if popup window is already open
		if (fContextMenu->Window() != NULL)
			return;

		// clean up items from last context menu
		MenusEnded();

		// setup nav menu
		SetupNavigationMenu(fContextMenu, ref);

		// update the rest
		UpdateMenu(fContextMenu, kPosePopUpContext, ref);
	} else if (fWindowContextMenu != NULL) {
		// clicked on a window, show window context menu
		fContextMenu = fWindowContextMenu;

		// bail out before cleanup if popup window is already open
		if (fContextMenu->Window() != NULL)
			return;

		// clean up items from last context menu
		MenusEnded();

		// setup nav menu
		SetupNavigationMenu(fContextMenu, TargetModel()->EntryRef());

		// update the rest
		UpdateMenu(fContextMenu, kWindowPopUpContext);
	}

	// synchronous Go()
	fContextMenu->Go(global, true, true, true);
	fContextMenu = NULL;
}


void
BContainerWindow::AddPoseContextMenu(BMenu* menu)
{
	menu->AddItem(Shortcuts()->OpenItem());
	// "Edit query" and "Open with..." inserted here,
	// see UpdateMenu(), SetupEditQueryItem() and SetupOpenWithMenu()
	menu->AddItem(Shortcuts()->GetInfoItem());
	menu->AddItem(Shortcuts()->EditNameItem());

	if (TargetModel()->InTrash()) {
		menu->AddItem(Shortcuts()->DeleteItem());
		menu->AddItem(Shortcuts()->RestoreItem());
	} else if (!TargetModel()->InTrash()) {
		menu->AddItem(Shortcuts()->DuplicateItem());
		menu->AddItem(Shortcuts()->MoveToTrashItem());
	}
	menu->AddSeparatorItem();

	// The "Move To", "Copy To", "Create Link" menus are inserted here,
	// have a look at UpdateMenu() and SetupMoveCopyMenus().

	if (!TargetModel()->IsPrintersDir() && !TargetModel()->IsRoot() && !TargetModel()->IsTrash()
		&& !TargetModel()->InTrash()) {
		menu->AddItem(Shortcuts()->CutItem());
		menu->AddItem(Shortcuts()->CopyItem());
		menu->AddItem(Shortcuts()->PasteItem());
		menu->AddSeparatorItem();
	}

	menu->AddItem(Shortcuts()->IdentifyItem());
	if (ShouldHaveAddOnMenus())
		menu->AddItem(new BMenuItem(new BMenu(Shortcuts()->AddOnsLabel())));
}


void
BContainerWindow::AddVolumeContextMenu(BMenu* menu)
{
	menu->AddItem(Shortcuts()->OpenItem());
	menu->AddItem(Shortcuts()->GetInfoItem());
	menu->AddItem(Shortcuts()->EditNameItem());
	menu->AddSeparatorItem();

	// "Mount >", "Unmount" and a separator are inserted here,
	// see UpdateMenu() and SetupMountMenu()

	if (ShouldHaveAddOnMenus())
		menu->AddItem(new BMenuItem(new BMenu(Shortcuts()->AddOnsLabel())));
}


void
BContainerWindow::AddWindowContextMenu(BMenu* menu)
{
	// create context sensitive menu for empty area of window
	// since we check view mode before display, this should be a radio
	// mode menu

	if (TargetModel()->IsTrash()) {
		menu->AddItem(Shortcuts()->EmptyTrashItem());
		menu->AddSeparatorItem();
	}

	if (ShouldHaveNewFolderItem()) {
		menu->AddItem(Shortcuts()->NewFolderItem());
		menu->AddSeparatorItem();
	}

	if (TargetModel()->IsDesktop()) {
		AddIconSizeMenu(menu);
		menu->AddSeparatorItem();
	}

	if (!(TargetModel()->IsPrintersDir() || TargetModel()->IsVolume() || TargetModel()->IsRoot()
			|| TargetModel()->IsTrash() || TargetModel()->InTrash())) {
		menu->AddItem(Shortcuts()->PasteItem());
		menu->AddSeparatorItem();
	}

	if (TargetModel()->IsDesktop()) // "Clean up" on Desktop
		menu->AddItem(Shortcuts()->CleanupItem());
	// else "Arrange by >" menu inserted here,
	// see UpdateMenu() and SetupArrangeByMenu()
	menu->AddItem(Shortcuts()->SelectItem());
	menu->AddItem(Shortcuts()->SelectAllItem());
	if (!TargetModel()->IsDesktop())
		menu->AddItem(Shortcuts()->OpenParentItem());
	menu->AddSeparatorItem();

	// "Mount >" menu and "Unmount" are inserted here,
	// see UpdateMenu() and SetupMountMenu().

	if (ShouldHaveAddOnMenus())
		menu->AddItem(new BMenuItem(new BMenu(Shortcuts()->AddOnsLabel())));

#if DEBUG
	menu->AddSeparatorItem();
	BMenuItem* testing = new BMenuItem("Test icon cache",
		new BMessage(kTestIconCache));
	menu->AddItem(testing);
	testing->SetTarget(PoseView());
#endif
}


void
BContainerWindow::AddDropContextMenu(BMenu* menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Move here"), new BMessage(kMoveSelectionTo)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Copy here"), new BMessage(kCopySelectionTo)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Create link here"), new BMessage(kCreateLink)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Cancel"), new BMessage(kCancelButton)));
}


void
BContainerWindow::AddTrashContextMenu(BMenu* menu)
{
	menu->AddItem(Shortcuts()->EmptyTrashItem());
	menu->AddItem(Shortcuts()->OpenItem());
	menu->AddItem(Shortcuts()->GetInfoItem());
}


void
BContainerWindow::EachAddOn(void (*eachAddOn)(const Model*, const char*,
		uint32 shortcut, uint32 modifiers, bool primary, void* context,
		BContainerWindow* window, BMenu* menu),
	void* passThru, BStringList& mimeTypes, BMenu* menu)
{
	AutoLock<LockingList<AddOnShortcut, true> > lock(fAddOnsList);
	if (!lock.IsLocked())
		return;

	for (int i = fAddOnsList->CountItems() - 1; i >= 0; i--) {
		struct AddOnShortcut* item = fAddOnsList->ItemAt(i);
		bool primary = false;

		if (mimeTypes.CountStrings() > 0) {
			BFile file(item->model->EntryRef(), B_READ_ONLY);
			if (file.InitCheck() == B_OK) {
				BAppFileInfo info(&file);
				if (info.InitCheck() == B_OK) {
					bool secondary = true;

					// does this add-on has types set at all?
					BMessage message;
					if (info.GetSupportedTypes(&message) == B_OK) {
						type_code typeCode;
						int32 count;
						if (message.GetInfo("types", &typeCode, &count) == B_OK)
							secondary = false;
					}

					// check all supported types if it has some set
					if (!secondary) {
						for (int32 i = mimeTypes.CountStrings(); !primary && i-- > 0;) {
							BString type = mimeTypes.StringAt(i);
							if (info.IsSupportedType(type.String())) {
								BMimeType mimeType(type.String());
								if (info.Supports(&mimeType))
									primary = true;
								else
									secondary = true;
							}
						}
					}

					if (!secondary && !primary)
						continue;
				}
			}
		}
		((eachAddOn)(item->model, item->model->Name(), item->key,
			item->modifiers, primary, passThru, this, menu));
	}
}


void
BContainerWindow::BuildMimeTypeList(BStringList& mimeTypes)
{
	int32 selectCount = PoseView()->CountSelected();
	if (selectCount <= 0) {
		// just add the type of the current directory
		AddMimeTypeString(mimeTypes, TargetModel());
	} else {
		_UpdateSelectionMIMEInfo();
		for (int32 index = 0; index < selectCount; index++) {
			BPose* pose = PoseView()->SelectionList()->ItemAt(index);
			AddMimeTypeString(mimeTypes, pose->TargetModel());
			// If it's a symlink, resolves it and add the Target's MimeType
			if (pose->TargetModel()->IsSymLink()) {
				Model* resolved = new Model(
					pose->TargetModel()->EntryRef(), true, true);
				if (resolved->InitCheck() == B_OK)
					AddMimeTypeString(mimeTypes, resolved);

				delete resolved;
			}
		}
	}
}


void
BContainerWindow::BuildAddOnMenus(BMenuBar* parent)
{
	BObjectList<BMenuItem> primaryList;
	BObjectList<BMenuItem> secondaryList;
	BStringList mimeTypes(10);

	AddOneAddOnParams params;
	params.primaryList = &primaryList;
	params.secondaryList = &secondaryList;

	EachAddOn(AddOneAddOn, &params, mimeTypes, parent);

	primaryList.SortItems(CompareLabels);
	secondaryList.SortItems(CompareLabels);

	int32 parentCount = parent->CountItems();
	int32 count = primaryList.CountItems();
	for (int32 index = 0; index < count; index++)
		parent->AddItem(primaryList.ItemAt(parentCount + index));
}


void
BContainerWindow::RebuildAddOnMenus(BMenuBar* parent)
{
	if (parent == NULL || !ShouldHaveAddOnMenus())
		return;

	int32 addOnsIndex = (PoseView()->ViewMode() == kListMode) ? 3 : 2;
	if (parent->CountItems() >= addOnsIndex) {
		BMenuItem* addOnsItem;
		while ((addOnsItem = parent->ItemAt(addOnsIndex)) != NULL) {
			parent->RemoveItem(addOnsItem);
			delete addOnsItem;
		}
	}
	if (ShouldAddMenus())
		BuildAddOnMenus(parent);
}


void
BContainerWindow::BuildAddOnsMenu(BMenu* parent)
{
	BMenuItem* addOnsItem = parent->FindItem(Shortcuts()->AddOnsLabel());
	if (parent->IndexOf(addOnsItem) == 0) {
		// the folder of the context menu seems to be named "Add-Ons"
		// so we just take the last menu item, which is correct if not
		// build with debug option
#if DEBUG
		addOnsItem = parent->ItemAt(parent->CountItems() - 3);
#else
		addOnsItem = parent->ItemAt(parent->CountItems() - 1);
#endif
	}

	if (addOnsItem == NULL)
		return;

	BMenu* addOnsMenu = addOnsItem->Submenu();
	if (addOnsMenu == NULL)
		return;

	// use a scope block for AutoLock
	{
		BFont font;
		AutoLock<BLooper> _(parent->Looper());
		parent->GetFont(&font);
		addOnsMenu->SetFont(&font);
	}

	// found add-ons menu, empty it first
	BMenuItem* item;
	for (;;) {
		item = addOnsMenu->RemoveItem((int32)0);
		if (item == NULL)
			break;
		delete item;
	}

	BObjectList<BMenuItem> primaryList;
	BObjectList<BMenuItem> secondaryList;
	BStringList mimeTypes(10);
	BuildMimeTypeList(mimeTypes);

	AddOneAddOnParams params;
	params.primaryList = &primaryList;
	params.secondaryList = &secondaryList;

	// build a list of the MIME types of the selected items

	EachAddOn(AddOneAddOn, &params, mimeTypes, parent);

	primaryList.SortItems(CompareLabels);
	secondaryList.SortItems(CompareLabels);

	int32 count = primaryList.CountItems();
	for (int32 index = 0; index < count; index++)
		addOnsMenu->AddItem(primaryList.ItemAt(index));

	if (count > 0)
		addOnsMenu->AddSeparatorItem();

	count = secondaryList.CountItems();
	for (int32 index = 0; index < count; index++)
		addOnsMenu->AddItem(secondaryList.ItemAt(index));

	Shortcuts()->UpdateAddOnsItem(addOnsItem);
}


void
BContainerWindow::UpdateMenu(BMenu* menu, MenuContext context, const entry_ref* ref)
{
	// update shared shortcut item's target and enabled state
	Shortcuts()->Update(menu);

	if (context == kFileMenuContext)
		UpdateFileMenu(menu);
	else if (context == kWindowMenuContext)
		UpdateWindowMenu(menu);
	else if (context == kPosePopUpContext)
		UpdatePoseContextMenu(menu, ref);
	else if (context == kWindowPopUpContext)
		UpdateWindowContextMenu(menu);
}


void
BContainerWindow::UpdateFileMenu(BMenu* menu)
{
	SetupNewTemplatesMenu(menu, kFileMenuContext);

	UpdateFileMenuOrPoseContextMenu(menu, kFileMenuContext);
}


void
BContainerWindow::UpdatePoseContextMenu(BMenu* menu, const entry_ref* ref)
{
	// ref must be set in pose pop up context
	ASSERT(ref != NULL);

	UpdateFileMenuOrPoseContextMenu(menu, kPosePopUpContext, ref);
}


void
BContainerWindow::UpdateFileMenuOrPoseContextMenu(BMenu* menu, MenuContext context,
	const entry_ref* ref)
{
	// ref must be set in pose pop up context
	if (context == kPosePopUpContext)
		ASSERT(ref != NULL);

	// if ref unset assume window ref
	if (ref == NULL)
		ref = TargetModel()->EntryRef();

	// "Open with..." menu inserted after Open
	if (context == kPosePopUpContext) {
		if (ShouldHaveOpenWithMenu(ref))
			SetupOpenWithMenu(menu, ref);
	} else if (context == kFileMenuContext) {
		if (ShouldHaveOpenWithMenu())
			SetupOpenWithMenu(menu);
	}

	// "Mount >" menu and "Unmount" are inserted here
	if (context == kPosePopUpContext) {
		Model model(ref);
		if (model.IsRoot() || model.IsVolume())
			SetupMountMenu(menu, kPosePopUpContext, ref);
	} else if (context == kFileMenuContext) {
		if (TargetModel()->IsRoot())
			SetupMountMenu(menu, kFileMenuContext);
	}

	// "Edit query" inserted before "Open with..."
	if (context == kPosePopUpContext) {
		if (ShouldHaveEditQueryItem(ref))
			SetupEditQueryItem(menu, ref);
	} else {
		if (ShouldHaveEditQueryItem())
			SetupEditQueryItem(menu);
	}

	// "Move To", "Copy To", "Create Link" menus inserted after "Move to Trash"
	if (ShouldHaveMoveCopyMenus(ref))
		SetupMoveCopyMenus(menu, ref);

	if (ShouldHaveAddOnMenus())
		BuildAddOnsMenu(menu);
}


void
BContainerWindow::UpdateWindowMenu(BMenu* menu)
{
	UpdateWindowMenuOrWindowContextMenu(menu, kWindowMenuContext);
}


void
BContainerWindow::UpdateWindowContextMenu(BMenu* menu)
{
	SetupNewTemplatesMenu(menu, kWindowPopUpContext);

	UpdateWindowMenuOrWindowContextMenu(menu, kWindowPopUpContext);

	// "Mount >" menu is inserted at the bottom
	if (TargetModel()->IsDesktop() || TargetModel()->IsRoot())
		SetupMountMenu(menu, kWindowPopUpContext);

	if (ShouldHaveAddOnMenus())
		BuildAddOnsMenu(menu);
}


void
BContainerWindow::UpdateWindowMenuOrWindowContextMenu(BMenu* menu, MenuContext context)
{
	// insert "Arrange by >" menu before "Select ..."
	SetupArrangeByMenu(menu);

	// update icon menus
	uint32 viewMode = PoseView()->ViewMode();

	// update icon size submenu
	BMenuItem* iconModeItem = menu->FindItem(kIconMode);
	if (iconModeItem != NULL) {
		BMenu* iconSizeMenu = iconModeItem->Submenu();
		if (iconSizeMenu != NULL) {
			if (viewMode == kIconMode) {
				// mark the current icon size
				BMenuItem* item;
				BMessage* message;
				int32 iconSize = PoseView()->UnscaledIconSizeInt();
				int32 itemCount = iconSizeMenu->CountItems();
				for (int32 index = 0; index < itemCount; index++) {
					item = iconSizeMenu->ItemAt(index);
					if (item == NULL)
						continue;

					message = item->Message();
					if (message == NULL) {
						item->SetMarked(false);
						continue;
					}

					bool sizeMatches = (iconSize == message->GetInt32("size", -1));
					item->SetMarked(sizeMatches);

					// radio mode
					if (sizeMatches)
						break;
				}
			} else {
				// unmark the marked item
				BMenuItem* marked = iconSizeMenu->FindMarked();
				if (marked != NULL)
					marked->SetMarked(false);
			}
		}
	}

	MarkNamedMenuItem(menu, kIconMode, viewMode == kIconMode);
	MarkNamedMenuItem(menu, kListMode, viewMode == kListMode);
	MarkNamedMenuItem(menu, kMiniIconMode, viewMode == kMiniIconMode);
}


BMessage*
BContainerWindow::AddOnMessage(int32 what)
{
	BMessage* message = new BMessage(what);

	// add selected refs to message
	PoseList* selectionList = PoseView()->SelectionList();

	int32 index = 0;
	BPose* pose;
	while ((pose = selectionList->ItemAt(index++)) != NULL)
		message->AddRef("refs", pose->TargetModel()->EntryRef());

	message->AddRef("dir_ref", TargetModel()->EntryRef());
	message->AddMessenger("TrackerViewToken", BMessenger(PoseView()));

	return message;
}


void
BContainerWindow::LoadAddOn(BMessage* message)
{
	UpdateIfNeeded();

	entry_ref addOnRef;
	status_t result = message->FindRef("refs", &addOnRef);
	if (result != B_OK) {
		BString buffer(B_TRANSLATE("Error %error loading add-On %name."));
		buffer.ReplaceFirst("%error", strerror(result));
		buffer.ReplaceFirst("%name", addOnRef.name);

		BAlert* alert = new BAlert("", buffer.String(), B_TRANSLATE("Cancel"),
			0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return;
	}

	// add selected refs to message
	BMessage* refs = AddOnMessage(B_REFS_RECEIVED);

	LaunchInNewThread("Add-on", B_NORMAL_PRIORITY, &AddOnThread, refs,
		addOnRef, *TargetModel()->EntryRef());
}


bool
BContainerWindow::ShouldHaveNavigationMenu(const entry_ref* ref)
{
	if (ref == NULL) {
		if (PoseView()->CountSelected() > 0)
			ref = PoseView()->SelectionList()->FirstItem()->TargetModel()->EntryRef();
		else
			ref = TargetModel()->EntryRef();
	}

	return !PoseView()->IsFilePanel() && !Model(ref).IsQuery();
}


bool
BContainerWindow::ShouldHaveOpenWithMenu(const entry_ref* ref)
{
	if (PoseView()->IsFilePanel())
		return false;

	if (ref == NULL)
		ref = TargetModel()->EntryRef();

	Model model(ref);
	if (model.IsPrintersDir())
		return false;

	return !(model.InTrash() || model.IsTrash() || model.IsRoot() || model.IsVolume());
}


bool
BContainerWindow::ShouldHaveEditQueryItem(const entry_ref* ref)
{
	if (ref == NULL)
		return FSIsQueriesDir(TargetModel()->EntryRef());

	Model model(ref);
	return model.IsQuery() || model.IsQueryTemplate();
}


bool
BContainerWindow::ShouldHaveMoveCopyMenus(const entry_ref* ref)
{
	if (PoseView()->IsFilePanel())
		return false;

	if (ref == NULL)
		ref = TargetModel()->EntryRef();

	Model model(ref);
	if (model.IsPrintersDir())
		return false;

	return !(model.IsTrash() || model.InTrash());
}


bool
BContainerWindow::ShouldHaveNewFolderItem()
{
	if (TargetModel()->IsPrintersDir())
		return false;

	return !(TargetModel()->IsQuery() || TargetModel()->IsRoot() || TargetModel()->IsTrash()
		|| TargetModel()->InTrash() || TargetModel()->IsVirtualDirectory());
}


bool
BContainerWindow::ShouldHaveAddOnMenus()
{
	return !PoseView()->IsFilePanel();
}


//	#pragma mark - BContainerWindow private methods


void
BContainerWindow::_UpdateSelectionMIMEInfo()
{
	BPose* pose;
	int32 index = 0;
	while ((pose = PoseView()->SelectionList()->ItemAt(index++)) != NULL) {
		BString mimeType(pose->TargetModel()->MimeType());
		if (!mimeType.Length() || mimeType.ICompare(B_FILE_MIMETYPE) == 0) {
			pose->TargetModel()->Mimeset(true);
			if (pose->TargetModel()->IsSymLink()) {
				Model* resolved = new Model(pose->TargetModel()->EntryRef(),
					true, true);
				if (resolved->InitCheck() == B_OK) {
					mimeType.SetTo(resolved->MimeType());
					if (!mimeType.Length()
						|| mimeType.ICompare(B_FILE_MIMETYPE) == 0) {
						resolved->Mimeset(true);
					}
				}
				delete resolved;
			}
		}
	}
}


void
BContainerWindow::_AddFolderIcon()
{
	if (fMenuBar == NULL) {
		// We don't want to add the icon if there's no menubar
		return;
	}

	float baseIconSize = be_control_look->ComposeIconSize(16).Height() + 1,
		iconSize = fMenuBar->Bounds().Height() - 2;
	if (iconSize < baseIconSize)
		iconSize = baseIconSize;

	fDraggableIcon = new(std::nothrow) DraggableContainerIcon(BSize(iconSize - 1, iconSize - 1));
	if (fDraggableIcon != NULL) {
		fMenuContainer->GroupLayout()->AddView(fDraggableIcon);
		fMenuBar->SetBorders(
			BControlLook::B_ALL_BORDERS & ~BControlLook::B_RIGHT_BORDER);
	}
}


void
BContainerWindow::_PassMessageToAddOn(BMessage* message)
{
	LaunchInNewThread("Add-on-Pass-Message", B_NORMAL_PRIORITY,
		&RunAddOnMessageThread, new BMessage(*message), (void*)NULL);
}


void
BContainerWindow::_NewTemplateSubmenu(entry_ref dirRef)
{
	entry_ref submenuRef;
	BPath path(&dirRef);
	path.Append(B_TRANSLATE_COMMENT("New submenu", "Folder name of New-template submenu"));
	get_ref_for_path(path.Path(), &submenuRef);

	if (FSCreateNewFolder(&submenuRef) != B_OK)
		return;

	// kAttrTemplateSubMenu shows the folder to be a submenu
	BNode node(&submenuRef);
	if (node.InitCheck() != B_OK)
		return;
	bool flag = true;
	node.WriteAttr(kAttrTemplateSubMenu, B_BOOL_TYPE, 0, &flag, sizeof(bool));

	// show and select new submenu in Tracker
	BEntry entry(&submenuRef);
	node_ref nref;
	if (entry.GetNodeRef(&nref) != B_OK)
		return;

	BMessage message(B_REFS_RECEIVED);
	message.AddRef("refs", &dirRef);
	message.AddData("nodeRefToSelect", B_RAW_TYPE, (void*)&nref, sizeof(node_ref));
	be_app->PostMessage(&message);
}


BMenuItem*
BContainerWindow::NewAttributeMenuItem(const char* label, const char* name,
	int32 type, float width, int32 align, bool editable, bool statField)
{
	return NewAttributeMenuItem(label, name, type, NULL, width, align,
		editable, statField);
}


BMenuItem*
BContainerWindow::NewAttributeMenuItem(const char* label, const char* name,
	int32 type, const char* displayAs, float width, int32 align,
	bool editable, bool statField)
{
	BMessage* message = new BMessage(kAttributeItem);
	message->AddString("attr_name", name);
	message->AddInt32("attr_type", type);
	message->AddInt32("attr_hash", (int32)AttrHashString(name, (uint32)type));
	message->AddFloat("attr_width", width);
	message->AddInt32("attr_align", align);
	if (displayAs != NULL)
		message->AddString("attr_display_as", displayAs);
	message->AddBool("attr_editable", editable);
	message->AddBool("attr_statfield", statField);

	BMenuItem* menuItem = new BMenuItem(label, message);
	menuItem->SetTarget(PoseView());

	return menuItem;
}


void
BContainerWindow::NewAttributesMenu()
{
	if (fAttrMenu != NULL)
		delete fAttrMenu;

	fAttrMenu = new BMenu(B_TRANSLATE("Attributes"));

	NewAttributesMenu(fAttrMenu);
}


void
BContainerWindow::NewAttributesMenu(BMenu* menu)
{
	ASSERT(PoseView() != NULL);

	// empty menu
	BMenuItem* item;
	while ((item = menu->RemoveItem((int32)0)) != NULL)
		delete item;

	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Copy layout"),
		new BMessage(kCopyAttributes)));
	item->SetTarget(PoseView());
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Paste layout"),
		new BMessage(kPasteAttributes)));
	item->SetTarget(PoseView());
	menu->AddSeparatorItem();

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Name"),
		kAttrStatName, B_STRING_TYPE, 145, B_ALIGN_LEFT, true, true));

	if (gLocalizedNamePreferred) {
		menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Real name"),
			kAttrRealName, B_STRING_TYPE, 145, B_ALIGN_LEFT, true, true));
	}

	menu->AddItem(NewAttributeMenuItem (B_TRANSLATE("Size"), kAttrStatSize,
		B_OFF_T_TYPE, 80, B_ALIGN_RIGHT, false, true));

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Modified"),
		kAttrStatModified, B_TIME_TYPE, 150, B_ALIGN_LEFT, false, true));

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Created"),
		kAttrStatCreated, B_TIME_TYPE, 150, B_ALIGN_LEFT, false, true));

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Kind"),
		kAttrMIMEType, B_MIME_STRING_TYPE, 145, B_ALIGN_LEFT, false, false));

	if (TargetModel()->IsTrash() || TargetModel()->InTrash()) {
		menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Original name"),
			kAttrOriginalPath, B_STRING_TYPE, 225, B_ALIGN_LEFT, false,
			false));
	} else {
		menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Location"), kAttrPath,
			B_STRING_TYPE, 225, B_ALIGN_LEFT, false, false));
	}

#ifdef OWNER_GROUP_ATTRIBUTES
	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Owner"), kAttrStatOwner,
		B_STRING_TYPE, 60, B_ALIGN_LEFT, false, true));

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Group"), kAttrStatGroup,
		B_STRING_TYPE, 60, B_ALIGN_LEFT, false, true));
#endif

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Permissions"),
		kAttrStatMode, B_STRING_TYPE, 80, B_ALIGN_LEFT, false, true));

	MarkAttributesMenu(menu);
}


void
BContainerWindow::ShowAttributesMenu()
{
	ASSERT(fAttrMenu != NULL);
	fMenuBar->AddItem(fAttrMenu, 2);
}


void
BContainerWindow::HideAttributesMenu()
{
	ASSERT(fAttrMenu != NULL);
	fMenuBar->RemoveItem(fAttrMenu);
}


void
BContainerWindow::MarkAttributesMenu()
{
	MarkAttributesMenu(fAttrMenu);
}


void
BContainerWindow::MarkAttributesMenu(BMenu* menu)
{
	if (menu == NULL)
		return;

	BMenuItem* item;
	BMenu* submenu;
	int32 submenuCount;
	int32 itemCount = menu->CountItems();
	for (int32 index = 0; index < itemCount; index++) {
		item = menu->ItemAt(index);
		int32 attrHash;
		if (item->Message() != NULL) {
			if (item->Message()->FindInt32("attr_hash", &attrHash) == B_OK)
				item->SetMarked(PoseView()->ColumnFor((uint32)attrHash) != 0);
			else
				item->SetMarked(false);
		}

		submenu = item->Submenu();
		if (submenu == NULL)
			continue;

		submenuCount = submenu->CountItems();
		for (int32 subindex = 0; subindex < submenuCount; subindex++) {
			item = submenu->ItemAt(subindex);
			if (item == NULL || item->Message() == NULL)
				continue;
			if (item->Message()->FindInt32("attr_hash", &attrHash) == B_OK)
				item->SetMarked(PoseView()->ColumnFor((uint32)attrHash) != 0);
			else
				item->SetMarked(false);
		}
	}
}


// Adds a menu for a specific MIME type if it doesn't exist already.
// Returns the menu, if it existed or not.
BMenu*
BContainerWindow::AddMimeMenu(const BMimeType& mimeType, bool isSuperType,
	BMenu* menu, int32 start)
{
	AutoLock<BLooper> _(menu->Looper());

	if (!mimeType.IsValid())
		return NULL;

	// Check if we already have an entry for this MIME type in the menu.
	for (int32 i = start; BMenuItem* item = menu->ItemAt(i); i++) {
		BMessage* message = item->Message();
		if (message == NULL)
			continue;

		const char* type;
		if (message->FindString("mimetype", &type) == B_OK
			&& !strcmp(mimeType.Type(), type)) {
			return item->Submenu();
		}
	}

	BMessage attrInfo;
	char description[B_MIME_TYPE_LENGTH];
	const char* label = mimeType.Type();

	if (!mimeType.IsInstalled())
		return NULL;

	// only add things to menu which have "user-visible" data
	if (mimeType.GetAttrInfo(&attrInfo) != B_OK)
		return NULL;

	if (mimeType.GetShortDescription(description) == B_OK && description[0])
		label = description;

	// go through each field in meta mime and add it to a menu
	BMenu* mimeMenu = NULL;
	if (isSuperType) {
		// If it is a supertype, we create the menu anyway as it may have
		// submenus later on.
		mimeMenu = new BMenu(label);
		BFont font;
		menu->GetFont(&font);
		mimeMenu->SetFont(&font);
	}

	int32 index = -1;
	const char* publicName;
	while (attrInfo.FindString("attr:public_name", ++index, &publicName)
			== B_OK) {
		if (!attrInfo.FindBool("attr:viewable", index)) {
			// don't add if attribute not viewable
			continue;
		}

		int32 type;
		int32 align;
		int32 width;
		bool editable;
		const char* attrName;
		if (attrInfo.FindString("attr:name", index, &attrName) != B_OK
			|| attrInfo.FindInt32("attr:type", index, &type) != B_OK
			|| attrInfo.FindBool("attr:editable", index, &editable) != B_OK
			|| attrInfo.FindInt32("attr:width", index, &width) != B_OK
			|| attrInfo.FindInt32("attr:alignment", index, &align) != B_OK)
			continue;

		BString displayAs;
		attrInfo.FindString("attr:display_as", index, &displayAs);

		if (mimeMenu == NULL) {
			// do a lazy allocation of the menu
			mimeMenu = new BMenu(label);
			BFont font;
			menu->GetFont(&font);
			mimeMenu->SetFont(&font);
		}
		mimeMenu->AddItem(NewAttributeMenuItem(publicName, attrName, type,
			displayAs.String(), width, align, editable, false));
	}

	if (mimeMenu == NULL)
		return NULL;

	BMessage* message = new BMessage(kMIMETypeItem);
	message->AddString("mimetype", mimeType.Type());
	menu->AddItem(new IconMenuItem(mimeMenu, message, mimeType.Type()));

	return mimeMenu;
}


BMenuItem*
BContainerWindow::NewArrangeByMenu()
{
	// must have an Attributes menu for "Arrange by >"
	ASSERT(fAttrMenu);

	// create a new "Arrange by >" menu
	TLiveArrangeByMenu* menu = new TLiveArrangeByMenu(Shortcuts()->ArrangeByLabel(), this);

	// add Attributes items to "Arrange by >"
	BMenuItem* item;
	int32 attrCount = fAttrMenu->CountItems();
	for (int32 i = 3; i < attrCount; i++) {
		// skip over "Copy layout", "Paste layout" and separator
		item = fAttrMenu->ItemAt(i);
		if (item == NULL || item->Message() == NULL)
			continue;

		item = new BMenuItem(item->Label(), new BMessage(*item->Message()));
		item->Message()->what = kArrangeBy;
		menu->AddItem(item);
	}
	menu->AddSeparatorItem();

	menu->AddItem(Shortcuts()->ReverseOrderItem());
	menu->AddSeparatorItem();

	menu->AddItem(Shortcuts()->CleanupItem());

	return new BMenuItem(menu);
}


void
BContainerWindow::SetupArrangeByMenu(BMenu* parent)
{
	// first remove "Arrange by >" from the old menu
	if (fArrangeByItem != NULL && fArrangeByItem->Menu() != NULL)
		fArrangeByItem->Menu()->RemoveItem(fArrangeByItem);

	// bail out if no parent menu to add to
	if (parent == NULL)
		return;

	// update "Clean up" on Desktop and bail out
	if (TargetModel()->IsDesktop()) {
		Shortcuts()->UpdateCleanupItem(Shortcuts()->FindItem(parent, kCleanup, kCleanupAll));
		return;
	}

	// bail out if no "Select ..." item found
	int32 selectIndex = parent->IndexOf(parent->FindItem(kShowSelectionWindow));
	if (selectIndex == B_ERROR)
		return;

	// add "Arrange by >" menu before "Select..."
	parent->AddItem(fArrangeByItem, selectIndex);

	// mark items
	uint32 attrHash;
	int32 itemCount = fArrangeByItem->Submenu()->CountItems();
	for (int32 index = 0; index < itemCount; index++) {
		BMenuItem* item = fArrangeByItem->Submenu()->ItemAt(index);
		if (item == NULL || item->Message() == NULL)
			continue;

		if (item->Message()->FindInt32("attr_hash", (int32*)&attrHash) == B_OK)
			item->SetMarked(PoseView()->PrimarySort() == attrHash);
		else if (item->Message()->what == kArrangeReverseOrder)
			item->SetMarked(PoseView()->ReverseSort());
	}

	Shortcuts()->UpdateArrangeByItem(fArrangeByItem);
}


void
BContainerWindow::AddMimeTypesToMenu()
{
	AddMimeTypesToMenu(fAttrMenu);
}


void
BContainerWindow::AddMimeTypesToMenu(BMenu* menu)
{
	if (menu == NULL)
		return;

	// Remove old mime type menus
	int32 start = menu->CountItems();
	while (start > 0 && menu->ItemAt(start - 1)->Submenu() != NULL) {
		delete menu->RemoveItem(start - 1);
		start--;
	}

	// Add a separator item if there is none yet
	if (start > 0
		&& dynamic_cast<BSeparatorItem*>(menu->ItemAt(start - 1)) == NULL)
		menu->AddSeparatorItem();

	// Add MIME type in case we're a default query type window
	BPath path;
	if (TargetModel() != NULL) {
		TargetModel()->GetPath(&path);
		if (path.InitCheck() == B_OK
			&& strstr(path.Path(), "/" kQueryTemplates "/") != NULL) {
			// demangle MIME type name
			BString name(TargetModel()->Name());
			name.ReplaceFirst('_', '/');

			PoseView()->AddMimeType(name.String());
		}
	}

	// Add MIME type menus

	int32 typeCount = PoseView()->CountMimeTypes();

	for (int32 index = 0; index < typeCount; index++) {
		BMimeType mimeType(PoseView()->MimeTypeAt(index));
		if (mimeType.InitCheck() == B_OK) {
			BMimeType superType;
			mimeType.GetSupertype(&superType);
			if (superType.InitCheck() == B_OK) {
				BMenu* superMenu = AddMimeMenu(superType, true, menu, start);
				if (superMenu != NULL) {
					// We have a supertype menu.
					AddMimeMenu(mimeType, false, superMenu, 0);
				}
			}
		}
	}

	// remove empty super menus, promote sub-types if needed

	for (int32 index = 0; index < typeCount; index++) {
		BMimeType mimeType(PoseView()->MimeTypeAt(index));
		BMimeType superType;
		mimeType.GetSupertype(&superType);

		BMenu* superMenu = AddMimeMenu(superType, true, menu, start);
		if (superMenu == NULL)
			continue;

		int32 itemsFound = 0;
		int32 menusFound = 0;
		for (int32 i = 0; BMenuItem* item = superMenu->ItemAt(i); i++) {
			if (item->Submenu() != NULL)
				menusFound++;
			else
				itemsFound++;
		}

		if (itemsFound == 0) {
			if (menusFound != 0) {
				// promote types to the top level
				while (BMenuItem* item = superMenu->RemoveItem((int32)0)) {
					menu->AddItem(item);
				}
			}

			menu->RemoveItem(superMenu->Superitem());
			delete superMenu->Superitem();
		}
	}

	// remove separator if it's the only item in menu
	BMenuItem* separator = menu->ItemAt(menu->CountItems() - 1);
	if (dynamic_cast<BSeparatorItem*>(separator) != NULL) {
		menu->RemoveItem(separator);
		delete separator;
	}

	MarkAttributesMenu(menu);
}


BHandler*
BContainerWindow::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	if (strcmp(property, "Poses") == 0) {
//		PRINT(("BContainerWindow::ResolveSpecifier %s\n", property));
		message->PopSpecifier();
		return PoseView();
	}

	return _inherited::ResolveSpecifier(message, index, specifier,
		form, property);
}


PiggybackTaskLoop*
BContainerWindow::DelayedTaskLoop()
{
	if (!fTaskLoop)
		fTaskLoop = new PiggybackTaskLoop;

	return fTaskLoop;
}


bool
BContainerWindow::NeedsDefaultStateSetup()
{
	if (TargetModel() == NULL)
		return false;

	if (TargetModel()->IsRoot()) {
		// don't try to set up anything if we are root
		return false;
	}

	WindowStateNodeOpener opener(this, false);
	if (opener.StreamNode() == NULL) {
		// can't read state, give up
		return false;
	}

	return !NodeHasSavedState(opener.Node());
}


bool
BContainerWindow::DefaultStateSourceNode(const char* name, BNode* result,
	bool createNew, bool createFolder)
{
	//PRINT(("looking for default state in tracker settings dir\n"));
	BPath settingsPath;
	if (FSFindTrackerSettingsDir(&settingsPath) != B_OK)
		return false;

	BDirectory dir(settingsPath.Path());

	BPath path(settingsPath);
	path.Append(name);
	if (!BEntry(path.Path()).Exists()) {
		if (!createNew)
			return false;

		BPath tmpPath(settingsPath);
		for (;;) {
			// deal with several levels of folders
			const char* nextSlash = strchr(name, '/');
			if (!nextSlash)
				break;

			BString tmp;
			tmp.SetTo(name, nextSlash - name);
			tmpPath.Append(tmp.String());

			mkdir(tmpPath.Path(), 0777);

			name = nextSlash + 1;
			if (!name[0]) {
				// can't deal with a slash at end
				return false;
			}
		}

		if (createFolder) {
			if (mkdir(path.Path(), 0777) < 0)
				return false;
		} else {
			BFile file;
			if (dir.CreateFile(name, &file) != B_OK)
				return false;
		}
	}

	//PRINT(("using default state from %s\n", path.Path()));
	result->SetTo(path.Path());
	return result->InitCheck() == B_OK;
}


void
BContainerWindow::SetupDefaultState()
{
	BNode defaultingNode;
		// this is where we'll ulitimately get the state from
	bool gotDefaultingNode = 0;
	bool shouldStagger = false;

	ASSERT(TargetModel() != NULL);

	PRINT(("folder %s does not have any saved state\n", TargetModel()->Name()));

	WindowStateNodeOpener opener(this, true);
		// this is our destination node, whatever it is for this window
	if (opener.StreamNode() == NULL)
		return;

	if (!TargetModel()->IsRoot()) {
		BDirectory deskDir;
		FSGetDeskDir(&deskDir);

		// try copying state from our parent directory, unless it is the
		// desktop folder
		BEntry entry(TargetModel()->EntryRef());
		BNode parent;
		if (FSGetParentVirtualDirectoryAware(entry, parent) == B_OK
			&& parent != deskDir) {
			PRINT(("looking at parent for state\n"));
			if (NodeHasSavedState(&parent)) {
				PRINT(("got state from parent\n"));
				defaultingNode = parent;
				gotDefaultingNode = true;
				// when getting state from parent, stagger the window
				shouldStagger = true;
			}
		}
	}

	if (!gotDefaultingNode
		// parent didn't have any state, use the template directory from
		// tracker settings folder for what our state should be
		// For simplicity we are not picking up the most recent
		// changes that didn't get committed if home is still open in
		// a window, that's probably not a problem; would be OK if state
		// got committed after every change
		&& !DefaultStateSourceNode(kDefaultFolderTemplate, &defaultingNode,
			true)) {
		return;
	}

	if (TargetModel()->IsDesktop()) {
		// don't copy over the attributes if we are the Desktop
		return;
	}

	// copy over the attributes

	// set up a filter of the attributes we want copied
	const char* allowAttrs[] = {
		kAttrWindowFrame,
		kAttrWindowWorkspace,
		kAttrViewState,
		kAttrViewStateForeign,
		kAttrColumns,
		kAttrColumnsForeign,
		0
	};

	// copy over attributes that apply; transform them properly, stripping
	// parts that do not apply, adding a window stagger, etc.

	StaggerOneParams params;
	params.rectFromParent = shouldStagger;
	SelectiveAttributeTransformer frameOffsetter(kAttrWindowFrame,
		OffsetFrameOne, &params);
	SelectiveAttributeTransformer scrollOriginCleaner(kAttrViewState,
		ClearViewOriginOne, &params);

	// do it
	AttributeStreamMemoryNode memoryNode;
	NamesToAcceptAttrFilter filter(allowAttrs);
	AttributeStreamFileNode fileNode(&defaultingNode);

	*opener.StreamNode() << scrollOriginCleaner << frameOffsetter
		<< memoryNode << filter << fileNode;
}


void
BContainerWindow::RestoreWindowState(AttributeStreamNode* node)
{
	if (node == NULL || TargetModel()->IsDesktop()) {
		// don't restore any window state if we are the Desktop
		return;
	}

	const char* rectAttributeName;
	const char* workspaceAttributeName;
	if (TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}

	BRect frame(Frame());
	if (node->Read(rectAttributeName, 0, B_RECT_TYPE, sizeof(BRect), &frame)
			== sizeof(BRect)) {
		const float scalingFactor = be_plain_font->Size() / 12.0f;
		frame.left *= scalingFactor;
		frame.top *= scalingFactor;
		frame.right *= scalingFactor;
		frame.bottom *= scalingFactor;

		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	} else
		sNewWindRect.OffsetBy(sWindowStaggerBy, sWindowStaggerBy);

	fPreviousBounds = Bounds();

	uint32 workspace;
	if (((fOpenFlags & kRestoreWorkspace) != 0)
		&& node->Read(workspaceAttributeName, 0, B_INT32_TYPE, sizeof(uint32),
			&workspace) == sizeof(uint32))
		SetWorkspaces(workspace);

	if ((fOpenFlags & kIsHidden) != 0)
		Minimize(true);

	// restore window decor settings
	int32 size = node->Contains(kAttrWindowDecor, B_RAW_TYPE);
	if (size > 0) {
		char buffer[size];
		if (((fOpenFlags & kRestoreDecor) != 0)
			&& node->Read(kAttrWindowDecor, 0, B_RAW_TYPE, size, buffer)
				== size) {
			BMessage decorSettings;
			if (decorSettings.Unflatten(buffer) == B_OK)
				SetDecoratorSettings(decorSettings);
		}
	}
}


void
BContainerWindow::RestoreWindowState(const BMessage& message)
{
	if (TargetModel()->IsDesktop()) {
		// don't restore any window state if we are the Desktop
		return;
	}

	const char* rectAttributeName;
	const char* workspaceAttributeName;
	if (TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}

	BRect frame(Frame());
	if (message.FindRect(rectAttributeName, &frame) == B_OK) {
		const float scalingFactor = be_plain_font->Size() / 12.0f;
		frame.left *= scalingFactor;
		frame.top *= scalingFactor;
		frame.right *= scalingFactor;
		frame.bottom *= scalingFactor;

		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	} else
		sNewWindRect.OffsetBy(sWindowStaggerBy, sWindowStaggerBy);

	uint32 workspace;
	if (((fOpenFlags & kRestoreWorkspace) != 0)
		&& message.FindInt32(workspaceAttributeName,
			(int32*)&workspace) == B_OK) {
		SetWorkspaces(workspace);
	}

	if ((fOpenFlags & kIsHidden) != 0)
		Minimize(true);

	// restore window decor settings
	BMessage decorSettings;
	if (((fOpenFlags & kRestoreDecor) != 0)
		&& message.FindMessage(kAttrWindowDecor, &decorSettings) == B_OK) {
		SetDecoratorSettings(decorSettings);
	}

	fStateNeedsSaving = false;
		// Undo the effect of the above MoveTo and ResizeTo calls
}


void
BContainerWindow::SaveWindowState(AttributeStreamNode* node)
{
	if (TargetModel() != NULL && PoseView()->IsDesktopView()) {
		// don't save window state if we are the Desktop
		return;
	}

	ASSERT(node != NULL);

	const char* rectAttributeName;
	const char* workspaceAttributeName;
	if (TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}

	// node is null if it already got deleted
	BRect frame(Frame());
	const float scalingFactor = be_plain_font->Size() / 12.0f;
	frame.left /= scalingFactor;
	frame.top /= scalingFactor;
	frame.right /= scalingFactor;
	frame.bottom /= scalingFactor;
	node->Write(rectAttributeName, 0, B_RECT_TYPE, sizeof(BRect), &frame);

	uint32 workspaces = Workspaces();
	node->Write(workspaceAttributeName, 0, B_INT32_TYPE, sizeof(uint32),
		&workspaces);

	BMessage decorSettings;
	if (GetDecoratorSettings(&decorSettings) == B_OK) {
		int32 size = decorSettings.FlattenedSize();
		char buffer[size];
		if (decorSettings.Flatten(buffer, size) == B_OK) {
			node->Write(kAttrWindowDecor, 0, B_RAW_TYPE, size, buffer);
		}
	}
}


void
BContainerWindow::SaveWindowState(BMessage& message) const
{
	const char* rectAttributeName;
	const char* workspaceAttributeName;

	if (TargetModel() != NULL && TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}

	// node is null if it already got deleted
	BRect frame(Frame());
	const float scalingFactor = be_plain_font->Size() / 12.0f;
	frame.left /= scalingFactor;
	frame.top /= scalingFactor;
	frame.right /= scalingFactor;
	frame.bottom /= scalingFactor;
	message.AddRect(rectAttributeName, frame);
	message.AddInt32(workspaceAttributeName, (int32)Workspaces());

	BMessage decorSettings;
	if (GetDecoratorSettings(&decorSettings) == B_OK) {
		message.AddMessage(kAttrWindowDecor, &decorSettings);
	}
}


void
BContainerWindow::ShowSelectionWindow()
{
	if (fSelectionWindow == NULL) {
		fSelectionWindow = new SelectionWindow(this);
		fSelectionWindow->Show();
	} else if (fSelectionWindow->Lock()) {
		// The window is already there, just bring it close
		fSelectionWindow->MoveCloseToMouse();
		if (fSelectionWindow->IsHidden())
			fSelectionWindow->Show();

		fSelectionWindow->Unlock();
	}
}


void
BContainerWindow::ShowNavigator(bool show)
{
	if (TargetModel()->IsDesktop() || !TargetModel()->IsDirectory() || PoseView()->IsFilePanel())
		return;

	if (show) {
		if (Navigator() && !Navigator()->IsHidden())
			return;

		if (Navigator() == NULL) {
			fNavigator = new BNavigator(TargetModel());
			fPoseContainer->GridLayout()->AddView(fNavigator, 0, 0, 2);
		}

		if (Navigator()->IsHidden())
			Navigator()->Show();

		if (PoseView()->VScrollBar())
			PoseView()->UpdateScrollRange();
	} else {
		if (!Navigator() || Navigator()->IsHidden())
			return;

		if (PoseView()->VScrollBar())
			PoseView()->UpdateScrollRange();

		fNavigator->Hide();
	}
}


void
BContainerWindow::SetSingleWindowBrowseShortcuts(bool enabled)
{
	if (TargetModel()->IsDesktop())
		return;

	if (enabled) {
		if (!Navigator())
			return;

		RemoveShortcut(B_DOWN_ARROW, B_COMMAND_KEY | B_OPTION_KEY);
		RemoveShortcut(B_UP_ARROW, B_COMMAND_KEY);
		RemoveShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY);

		AddShortcut(B_LEFT_ARROW, B_COMMAND_KEY,
			new BMessage(kNavigatorCommandBackward), Navigator());
		AddShortcut(B_RIGHT_ARROW, B_COMMAND_KEY,
			new BMessage(kNavigatorCommandForward), Navigator());
		AddShortcut(B_UP_ARROW, B_COMMAND_KEY,
			new BMessage(kNavigatorCommandUp), Navigator());

		AddShortcut(B_LEFT_ARROW, B_OPTION_KEY | B_COMMAND_KEY,
			new BMessage(kNavigatorCommandBackward), Navigator());
		AddShortcut(B_RIGHT_ARROW, B_OPTION_KEY | B_COMMAND_KEY,
			new BMessage(kNavigatorCommandForward), Navigator());
		AddShortcut(B_UP_ARROW, B_OPTION_KEY | B_COMMAND_KEY,
			new BMessage(kNavigatorCommandUp), Navigator());
		AddShortcut(B_DOWN_ARROW, B_OPTION_KEY | B_COMMAND_KEY,
			new BMessage(kOpenSelection), PoseView());
		AddShortcut('L', B_COMMAND_KEY,
			new BMessage(kNavigatorCommandSetFocus), Navigator());
	} else {
		RemoveShortcut(B_LEFT_ARROW, B_COMMAND_KEY);
		RemoveShortcut(B_RIGHT_ARROW, B_COMMAND_KEY);
		RemoveShortcut(B_UP_ARROW, B_COMMAND_KEY);
			// This is added again, below, with a new meaning.

		RemoveShortcut(B_LEFT_ARROW, B_OPTION_KEY | B_COMMAND_KEY);
		RemoveShortcut(B_RIGHT_ARROW, B_OPTION_KEY | B_COMMAND_KEY);
		RemoveShortcut(B_UP_ARROW, B_OPTION_KEY | B_COMMAND_KEY);
		RemoveShortcut(B_DOWN_ARROW, B_COMMAND_KEY | B_OPTION_KEY);
			// This also changes meaning, added again below.

		AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY | B_OPTION_KEY,
			new BMessage(kOpenSelection), PoseView());
		AddShortcut(B_UP_ARROW, B_COMMAND_KEY,
			new BMessage(kOpenParentDir), PoseView());
			// We change the meaning from kNavigatorCommandUp
			// to kOpenParentDir.
		AddShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY,
			new BMessage(kOpenParentDir), PoseView());
			// command + option results in closing the parent window
		RemoveShortcut('L', B_COMMAND_KEY);
	}
}


void
BContainerWindow::SetPathWatchingEnabled(bool enable)
{
	if (IsPathWatchingEnabled()) {
		stop_watching(this);
		fIsWatchingPath = false;
	}

	if (enable) {
		if (TargetModel() != NULL) {
			BEntry entry;

			TargetModel()->GetEntry(&entry);
			status_t err;
			do {
				err = entry.GetParent(&entry);
				if (err != B_OK)
					break;

				char name[B_FILE_NAME_LENGTH];
				entry.GetName(name);
				if (strcmp(name, "/") == 0)
					break;

				node_ref ref;
				entry.GetNodeRef(&ref);
				watch_node(&ref, B_WATCH_NAME, this);
			} while (err == B_OK);

			fIsWatchingPath = err == B_OK;
		} else
			fIsWatchingPath = false;
	}
}


void
BContainerWindow::PulseTaskLoop()
{
	if (fTaskLoop)
		fTaskLoop->PulseMe();
}


//	#pragma mark - WindowStateNodeOpener


WindowStateNodeOpener::WindowStateNodeOpener(BContainerWindow* window,
	bool forWriting)
	:
	fModelOpener(NULL),
	fNode(NULL),
	fStreamNode(NULL)
{
	if (window->TargetModel() != NULL && window->TargetModel()->IsRoot()) {
		BDirectory dir;
		if (FSGetDeskDir(&dir) == B_OK) {
			fNode = new BDirectory(dir);
			fStreamNode = new AttributeStreamFileNode(fNode);
		}
	} else if (window->TargetModel() != NULL) {
		fModelOpener = new ModelNodeLazyOpener(window->TargetModel(),
			forWriting, false);
		if (fModelOpener->IsOpen(forWriting)) {
			fStreamNode = new AttributeStreamFileNode(
				fModelOpener->TargetModel()->Node());
		}
	}
}

WindowStateNodeOpener::~WindowStateNodeOpener()
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;
}


void
WindowStateNodeOpener::SetTo(const BDirectory* node)
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;

	fModelOpener = NULL;
	fNode = new BDirectory(*node);
	fStreamNode = new AttributeStreamFileNode(fNode);
}


void
WindowStateNodeOpener::SetTo(const BEntry* entry, bool forWriting)
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;

	fModelOpener = NULL;
	fNode = new BFile(entry, (uint32)(forWriting ? O_RDWR : O_RDONLY));
	fStreamNode = new AttributeStreamFileNode(fNode);
}


void
WindowStateNodeOpener::SetTo(Model* model, bool forWriting)
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;

	fNode = NULL;
	fStreamNode = NULL;
	fModelOpener = new ModelNodeLazyOpener(model, forWriting, false);
	if (fModelOpener->IsOpen(forWriting)) {
		fStreamNode = new AttributeStreamFileNode(
			fModelOpener->TargetModel()->Node());
	}
}


AttributeStreamNode*
WindowStateNodeOpener::StreamNode() const
{
	return fStreamNode;
}


BNode*
WindowStateNodeOpener::Node() const
{
	if (!fStreamNode)
		return NULL;

	if (fNode)
		return fNode;

	return fModelOpener->TargetModel()->Node();
}


//	#pragma mark - BorderedView


BorderedView::BorderedView()
	:
	BGroupView(B_VERTICAL, 0),
	fEnableBorderHighlight(true)
{
	GroupLayout()->SetInsets(1);
}


void
BorderedView::WindowActivated(bool active)
{
	BContainerWindow* window = dynamic_cast<BContainerWindow*>(Window());
	if (window == NULL)
		return;

	if (window->PoseView()->IsFocus())
		PoseViewFocused(active); // Update border color
}


void BorderedView::EnableBorderHighlight(bool enable)
{
	fEnableBorderHighlight = enable;
	PoseViewFocused(false);
}


void
BorderedView::PoseViewFocused(bool focused)
{
	BContainerWindow* window = dynamic_cast<BContainerWindow*>(Window());
	if (window == NULL)
		return;

	color_which base = B_DOCUMENT_BACKGROUND_COLOR;
	float tint = B_DARKEN_2_TINT;
	if (focused && window->IsActive() && fEnableBorderHighlight) {
		base = B_KEYBOARD_NAVIGATION_COLOR;
		tint = B_NO_TINT;
	}

	BScrollBar* hScrollBar = window->PoseView()->HScrollBar();
	if (hScrollBar != NULL)
		hScrollBar->SetBorderHighlighted(focused);

	BScrollBar* vScrollBar = window->PoseView()->VScrollBar();
	if (vScrollBar != NULL)
		vScrollBar->SetBorderHighlighted(focused);

	SetViewUIColor(base, tint);
	Invalidate();
}


void
BorderedView::Pulse()
{
	BContainerWindow* window = dynamic_cast<BContainerWindow*>(Window());
	if (window != NULL)
		window->PulseTaskLoop();
}
