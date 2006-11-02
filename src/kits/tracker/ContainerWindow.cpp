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
#include <image.h>

#include <Alert.h>
#include <Application.h>
#include <AppFileInfo.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Roster.h>

#include <fs_attr.h>

#include <memory>

#include "Attributes.h"
#include "AttributeStream.h"
#include "AutoLock.h"
#include "BackgroundImage.h"
#include "Commands.h"
#include "ContainerWindow.h"
#include "DeskWindow.h"
#include "FavoritesMenu.h"
#include "FindPanel.h"
#include "FSClipboard.h"
#include "FSUndoRedo.h"
#include "FSUtils.h"
#include "IconMenuItem.h"
#include "OpenWithWindow.h"
#include "MimeTypes.h"
#include "Model.h"
#include "MountMenu.h"
#include "Navigator.h"
#include "NavMenu.h"
#include "PoseView.h"
#include "SelectionWindow.h"
#include "TitleView.h"
#include "Tracker.h"
#include "TrackerSettings.h"
#include "Thread.h"
#include "TemplatesMenu.h"


const uint32 kRedo = 'REDO';
	// this is the same as B_REDO in Dano/Zeta/OpenBeOS


#if !B_BEOS_VERSION_DANO
_IMPEXP_BE
#endif
void do_minimize_team(BRect zoomRect, team_id team, bool zoom);

// Amount you have to move the mouse before a drag starts
const float kDragSlop = 3.0f;

namespace BPrivate {

const char *kAddOnsMenuName = "Add-Ons";

class DraggableContainerIcon : public BView {
	public:
		DraggableContainerIcon(BRect rect, const char *name, uint32 resizeMask);

		virtual void AttachedToWindow();
		virtual void MouseDown(BPoint where);
		virtual void MouseUp(BPoint where);
		virtual void MouseMoved(BPoint point, uint32 /*transit*/, const BMessage *message);
		virtual void FrameMoved(BPoint newLocation);
		virtual void Draw(BRect updateRect);

	private:
		uint32	fDragButton;
		BPoint	fClickPoint;
		bool	fDragStarted;
};

}	// namespace BPrivate

struct AddOneAddonParams {
	BObjectList<BMenuItem> *primaryList;
	BObjectList<BMenuItem> *secondaryList;
};

struct StaggerOneParams {
	bool rectFromParent;
};

const int32 kContainerWidthMinLimit = 120;
const int32 kContainerWindowHeightLimit = 85;

const int32 kWindowStaggerBy = 17;

BRect BContainerWindow::sNewWindRect(85, 50, 415, 280);


namespace BPrivate {

filter_result
ActivateWindowFilter(BMessage *, BHandler **target, BMessageFilter *)
{
	BView *view = dynamic_cast<BView*>(*target);

	// activate the window if no PoseView or DraggableContainerIcon had been pressed
	// (those will activate the window themselves, if necessary)
	if (view
		&& !dynamic_cast<BPoseView*>(view)
		&& !dynamic_cast<DraggableContainerIcon*>(view)
		&& view->Window())
		view->Window()->Activate(true);

	return B_DISPATCH_MESSAGE;		
}


static void
StripShortcut(const Model *model, char *result, uint32 &shortcut)
{
	strcpy(result, model->Name());

	// check if there is a shortcut
	uint32 length = strlen(result);
	shortcut = '\0';
	if (result[length - 2] == '-') {
		shortcut = result[length - 1];
		result[length - 2] = '\0';
	}
}


static const Model *
MatchOne(const Model *model, void *castToName)
{
	char buffer[B_FILE_NAME_LENGTH];
	uint32 dummy;
	StripShortcut(model, buffer, dummy);

	if (strcmp(buffer, (const char *)castToName) == 0) 
		// found match, bail out
		return model;

	return 0;
}


int
CompareLabels(const BMenuItem *item1, const BMenuItem *item2)
{
	return strcasecmp(item1->Label(), item2->Label());
}

}	// namespace BPrivate


static bool
AddOneAddon(const Model *model, const char *name, uint32 shortcut, bool primary, void *context)
{
	AddOneAddonParams *params = (AddOneAddonParams *)context;

	BMessage *message = new BMessage(kLoadAddOn);
	message->AddRef("refs", model->EntryRef());

	ModelMenuItem *item = new ModelMenuItem(model, name, message,
		(char)shortcut, B_OPTION_KEY);

	if (primary)
		params->primaryList->AddItem(item);
	else
		params->secondaryList->AddItem(item);

	return false;
}


static int32
AddOnThread(BMessage *refsMessage, entry_ref addonRef, entry_ref dirRef)
{
	std::auto_ptr<BMessage> refsMessagePtr(refsMessage);

	BEntry entry(&addonRef);
	BPath path;
	status_t result = entry.InitCheck();
	if (result == B_OK)
		result = entry.GetPath(&path);

	if (result == B_OK) {
		image_id addonImage = load_add_on(path.Path());
		if (addonImage >= 0) {
			void (*processRefs)(entry_ref, BMessage *, void *);
			result = get_image_symbol(addonImage, "process_refs", 2, (void **)&processRefs);

#ifndef __INTEL__
			if (result < 0) {
				PRINT(("trying old legacy ppc signature\n"));
				// try old-style addon signature
				result = get_image_symbol(addonImage,
					"process_refs__F9entry_refP8BMessagePv", 2, (void **)&processRefs);
			}
#endif

			if (result >= 0) {

				// call add-on code
				(*processRefs)(dirRef, refsMessagePtr.get(), 0);
				
				unload_add_on(addonImage);
				return B_OK;
			} else 
				PRINT(("couldn't find process_refs\n"));

			unload_add_on(addonImage);
		} else
			result = addonImage;
	}

	char buffer[1024];
	sprintf(buffer, "Error %s loading Add-On %s.", strerror(result), addonRef.name);

	BAlert *alert = new BAlert("", buffer, "Cancel", 0, 0,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);
	alert->Go();
	
	return result;
}


static bool
NodeHasSavedState(const BNode *node)
{
	attr_info info;
	return node->GetAttrInfo(kAttrWindowFrame, &info) == B_OK;
}


static bool
OffsetFrameOne(const char *DEBUG_ONLY(name), uint32, off_t, void *castToRect,
	void *castToParams)
{
	ASSERT(strcmp(name, kAttrWindowFrame) == 0);
	StaggerOneParams *params = (StaggerOneParams *)castToParams;

	if (!params->rectFromParent)
		return false;
		
	if (!castToRect)
		return false;
	
	((BRect *)castToRect)->OffsetBy(kWindowStaggerBy, kWindowStaggerBy);
	return true;
}


static void
AddMimeTypeString(BObjectList<BString> &list, Model *model)
{
	BString *mimeType = new BString(model->MimeType());
	if (!mimeType->Length() || !mimeType->ICompare(B_FILE_MIMETYPE)) {
		// if model is of unknown type, try mimeseting it first
		model->Mimeset(true);
		mimeType->SetTo(model->MimeType());
	}

	if (mimeType->Length()) {
		// only add the type if it's not already there
		for (int32 i = list.CountItems(); i-- > 0;) {
			BString *string = list.ItemAt(i);
			if (string != NULL && !string->ICompare(*mimeType))
				return;
		}
		list.AddItem(mimeType);
	}
}


//	#pragma mark -


DraggableContainerIcon::DraggableContainerIcon(BRect rect, const char *name,
	uint32 resizeMask)
	: BView(rect, name, resizeMask, B_WILL_DRAW | B_FRAME_EVENTS),
	fDragButton(0),
	fDragStarted(false)
{
}


void
DraggableContainerIcon::AttachedToWindow()
{
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	FrameMoved(BPoint(0, 0));
		// this decides whether to hide the icon or not
}


void
DraggableContainerIcon::MouseDown(BPoint point)
{
	// we only like container windows
	BContainerWindow *window = dynamic_cast<BContainerWindow *>(Window());
	if (window == NULL)
		return;

	// we don't like the Trash icon (because it cannot be moved)
	if (window->IsTrash() || window->IsPrintersDir())
		return;

	uint32 buttons;
	window->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);

	if (IconCache::sIconCache->IconHitTest(point, window->TargetModel(),
			kNormalIcon, B_MINI_ICON)) {
		// The click hit the icon, initiate a drag
		fDragButton = buttons & (B_PRIMARY_MOUSE_BUTTON | B_SECONDARY_MOUSE_BUTTON);
		fDragStarted = false;
		fClickPoint = point;
	} else
		fDragButton = 0;

	if (!fDragButton)
		Window()->Activate(true);
}


void
DraggableContainerIcon::MouseUp(BPoint /*point*/)
{
	if (!fDragStarted)
		Window()->Activate(true);

	fDragButton = 0;
	fDragStarted = false;
}


void
DraggableContainerIcon::MouseMoved(BPoint point, uint32 /*transit*/,
	const BMessage */*message*/)
{
	if (fDragButton == 0 || fDragStarted
		|| (abs((int32)(point.x - fClickPoint.x)) <= kDragSlop
			&& abs((int32)(point.y - fClickPoint.y)) <= kDragSlop))
		return;

	BContainerWindow *window = static_cast<BContainerWindow *>(Window());
		// we can only get here in a BContainerWindow
	Model *model = window->TargetModel();

	// Find the required height
	BFont font;
	GetFont(&font);

	font_height fontHeight;
	font.GetHeight(&fontHeight);
	float height = fontHeight.ascent + fontHeight.descent + fontHeight.leading + 2
		+ Bounds().Height() + 8;

	BRect rect(0, 0, max_c(Bounds().Width(), font.StringWidth(model->Name()) + 4), height);
	BBitmap *dragBitmap = new BBitmap(rect, B_RGBA32, true);

	dragBitmap->Lock();
	BView *view = new BView(dragBitmap->Bounds(), "", B_FOLLOW_NONE, 0);
	dragBitmap->AddChild(view);
	view->SetOrigin(0, 0);
	BRect clipRect(view->Bounds());
	BRegion newClip;
	newClip.Set(clipRect);
	view->ConstrainClippingRegion(&newClip);

	// Transparent draw magic
	view->SetHighColor(0, 0, 0, 0);
	view->FillRect(view->Bounds());
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetHighColor(0, 0, 0, 128);	// set the level of transparency by  value
	view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);

	// Draw the icon
	float hIconOffset = (rect.Width() - Bounds().Width()) / 2;
	IconCache::sIconCache->Draw(model, view, BPoint(hIconOffset, 0),
		kNormalIcon, B_MINI_ICON, true);

	// See if we need to truncate the string
	BString nameString = model->Name();
	if (view->StringWidth(model->Name()) > rect.Width()) 
		view->TruncateString(&nameString, B_TRUNCATE_END, rect.Width() - 5);

	// Draw the label
	float leftText = (view->StringWidth(nameString.String()) - Bounds().Width()) / 2;
	view->MovePenTo(BPoint(hIconOffset - leftText + 2, Bounds().Height() + (fontHeight.ascent + 2)));
	view->DrawString(nameString.String());

	view->Sync();
	dragBitmap->Unlock();

	BMessage message(B_SIMPLE_DATA);
	message.AddRef("refs", model->EntryRef());
	message.AddPoint("click_pt", fClickPoint);

	BPoint tmpLoc;
	uint32 button;
	GetMouse(&tmpLoc, &button);
	if (button)
		message.AddInt32("buttons", (int32)button);

	if (button & B_PRIMARY_MOUSE_BUTTON) {
		// add an action specifier to the message, so that it is not copied
		message.AddInt32("be:actions",
			(modifiers() & B_OPTION_KEY) != 0 ? B_COPY_TARGET : B_MOVE_TARGET);
	}

	fDragStarted = true;
	fDragButton = 0;

	DragMessage(&message, dragBitmap, B_OP_ALPHA,
		BPoint(fClickPoint.x + hIconOffset, fClickPoint.y), this);
}


void
DraggableContainerIcon::FrameMoved(BPoint /*newLocation*/)
{
	BMenuBar* bar = dynamic_cast<BMenuBar *>(Parent());
	if (bar == NULL)
		return;

	// TODO: ugly hack following:
	// This is a trick to get the actual width of all menu items
	// (BMenuBar may not have set the item coordinates yet...)
	float width, height;
	uint32 resizingMode = bar->ResizingMode();
	bar->SetResizingMode(B_FOLLOW_NONE);
	bar->GetPreferredSize(&width, &height);
	bar->SetResizingMode(resizingMode);

/*
	BMenuItem* item = bar->ItemAt(bar->CountItems() - 1);
	if (item == NULL)
		return;
*/
	// BeOS shifts the coordinates for hidden views, so we cannot
	// use them to decide if we should be visible or not...

	float gap = bar->Frame().Width() - 2 - width; //item->Frame().right;

	if (gap <= Bounds().Width() && !IsHidden())
		Hide();
	else if (gap > Bounds().Width() && IsHidden())
		Show();
}


void
DraggableContainerIcon::Draw(BRect /*updateRect*/)
{
	BContainerWindow *window = dynamic_cast<BContainerWindow *>(Window());
	if (window == NULL)
		return;

	// Draw the icon, straddling the border
	SetDrawingMode(B_OP_OVER);
	float iconOffset = (Bounds().Width() - B_MINI_ICON) / 2;
	IconCache::sIconCache->Draw(window->TargetModel(), this,
		BPoint(iconOffset, iconOffset), kNormalIcon, B_MINI_ICON, true);
}


//	#pragma mark -


BContainerWindow::BContainerWindow(LockingList<BWindow> *list,
		uint32 containerWindowFlags,
		window_look look, window_feel feel, uint32 flags, uint32 workspace)
	: BWindow(InitialWindowRect(feel), "TrackerWindow", look, feel, flags,
			workspace),
	fFileContextMenu(NULL),
	fWindowContextMenu(NULL),
	fDropContextMenu(NULL),
	fVolumeContextMenu(NULL),
	fDragContextMenu(NULL),
	fMoveToItem(NULL),
	fCopyToItem(NULL),
	fCreateLinkItem(NULL),
	fOpenWithItem(NULL),
	fNavigationItem(NULL),
	fMenuBar(NULL),
	fNavigator(NULL),
	fPoseView(NULL),
	fWindowList(list),
	fAttrMenu(NULL),
	fWindowMenu(NULL),
	fFileMenu(NULL),
	fSelectionWindow(NULL),
	fTaskLoop(NULL),
	fIsTrash(false),
	fInTrash(false),
	fIsPrinters(false),
	fContainerWindowFlags(containerWindowFlags),
	fBackgroundImage(NULL),
	fSavedZoomRect(0, 0, -1, -1),
	fContextMenu(NULL),
	fDragMessage(NULL),
	fCachedTypesList(NULL),
	fStateNeedsSaving(false),
	fSaveStateIsEnabled(true),
	fIsWatchingPath(false)
{
	InitIconPreloader();

	if (list) {
		ASSERT(list->IsLocked());
		list->AddItem(this);
	}

	AddCommonFilter(new BMessageFilter(B_MOUSE_DOWN, ActivateWindowFilter));

	Run();

	// Watch out for settings changes:
	if (TTracker *app = dynamic_cast<TTracker*>(be_app)) {
		app->Lock();
		app->StartWatching(this, kWindowsShowFullPathChanged);
		app->StartWatching(this, kSingleWindowBrowseChanged);
		app->StartWatching(this, kShowNavigatorChanged);
		app->StartWatching(this, kDontMoveFilesToTrashChanged);
		app->Unlock();
	}

	// ToDo: remove me once we have undo/redo menu items
	//	(that is, move them to AddShortcuts())
 	AddShortcut('Z', B_COMMAND_KEY, new BMessage(B_UNDO), this);
 	AddShortcut('Z', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kRedo), this);
}


BContainerWindow::~BContainerWindow()
{
	ASSERT(IsLocked());

	// stop the watchers
	if (TTracker *app = dynamic_cast<TTracker*>(be_app)) {
		app->Lock();
		app->StopWatching(this, kWindowsShowFullPathChanged);
		app->StopWatching(this, kSingleWindowBrowseChanged);
		app->StopWatching(this, kShowNavigatorChanged);
		app->StopWatching(this, kDontMoveFilesToTrashChanged);
		app->Unlock();
	}

	delete fTaskLoop;
	delete fBackgroundImage;
	delete fDragMessage;
	delete fCachedTypesList;

	if (fSelectionWindow && fSelectionWindow->Lock())
		fSelectionWindow->Quit();
}


BRect 
BContainerWindow::InitialWindowRect(window_feel feel)
{
	if (feel != kPrivateDesktopWindowFeel)
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
	if (CurrentMessage()
		&& (CurrentMessage()->FindInt32("modifiers") & B_CONTROL_KEY))
		be_app->PostMessage(kCloseAllWindows);

	Hide();
		// this will close the window instantly, even if
		// the file system is very busy right now
	return true;
}


void
BContainerWindow::Quit()
{
	// get rid of context menus
	if (fNavigationItem) {
		BMenu *menu = fNavigationItem->Menu();
		if (menu)
			menu->RemoveItem(fNavigationItem);
		delete fNavigationItem;
		fNavigationItem = NULL;
	}
	
	if (fOpenWithItem && !fOpenWithItem->Menu()) {
		delete fOpenWithItem;
		fOpenWithItem = NULL;
	}

	if (fMoveToItem && !fMoveToItem->Menu()) {
		delete fMoveToItem;
		fMoveToItem = NULL;
	}

	if (fCopyToItem && !fCopyToItem->Menu()) {
		delete fCopyToItem;
		fCopyToItem = NULL;
	}

	if (fCreateLinkItem && !fCreateLinkItem->Menu()) {
		delete fCreateLinkItem;
		fCreateLinkItem = NULL;
	}

	if (fAttrMenu && !fAttrMenu->Supermenu()) {
		delete fAttrMenu;
		fAttrMenu = NULL;
	}
		
	delete fFileContextMenu;
	fFileContextMenu = NULL;
	delete fWindowContextMenu;
	fWindowContextMenu = NULL;
	delete fDropContextMenu;
	fDropContextMenu = NULL;
	delete fVolumeContextMenu;
	fVolumeContextMenu = NULL;
	delete fDragContextMenu;
	fDragContextMenu = NULL;
	
	int32 windowCount = 0;
	
	// This is a deadlock code sequence - need to change this
	// to acquire the window list while this container window is unlocked
	if (fWindowList) {
		AutoLock<LockingList<BWindow> > lock(fWindowList);
		if (lock.IsLocked()) {
			fWindowList->RemoveItem(this);
			windowCount = fWindowList->CountItems();
		}
	}

	if (StateNeedsSaving())
		SaveState();

	if (fWindowList && windowCount == 0)
		be_app->PostMessage(B_QUIT_REQUESTED);

	_inherited::Quit();
}


BPoseView *
BContainerWindow::NewPoseView(Model *model, BRect rect, uint32 viewMode)
{
	return new BPoseView(model, rect, viewMode);
}


void
BContainerWindow::UpdateIfTrash(Model *model)
{
	BEntry entry(model->EntryRef());
	
	if (entry.InitCheck() == B_OK) {
		fIsTrash = FSIsTrashDir(&entry);
		fInTrash = FSInTrashDir(model->EntryRef());
		fIsPrinters = FSIsPrintersDir(&entry);
	}
}


void
BContainerWindow::CreatePoseView(Model *model)
{
	UpdateIfTrash(model);
	BRect rect(Bounds());
	
	TrackerSettings settings;
	if (settings.SingleWindowBrowse()
		&& settings.ShowNavigator()
		&& model->IsDirectory())
		rect.top += BNavigator::CalcNavigatorHeight() + 1;

	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom -= B_H_SCROLL_BAR_HEIGHT;
	fPoseView = NewPoseView(model, rect, kListMode);
	AddChild(fPoseView);

	if (settings.SingleWindowBrowse()
		&& model->IsDirectory()
		&& !fPoseView->IsFilePanel()) {
		BRect rect(Bounds());
		rect.top = 0;
			// The KeyMenuBar isn't attached yet, otherwise we'd use that to get the offset.
		rect.bottom = BNavigator::CalcNavigatorHeight();
		fNavigator = new BNavigator(model, rect);
		if (!settings.ShowNavigator())
			fNavigator->Hide();
		AddChild(fNavigator);
	}
	SetPathWatchingEnabled(settings.ShowNavigator() || settings.ShowFullPathInTitleBar());
}


void 
BContainerWindow::AddContextMenus()
{
	// create context sensitive menus
	fFileContextMenu = new BPopUpMenu("FileContext", false, false);
	fFileContextMenu->SetFont(be_plain_font);
	AddFileContextMenus(fFileContextMenu);
	
	fVolumeContextMenu = new BPopUpMenu("VolumeContext", false, false);
	fVolumeContextMenu->SetFont(be_plain_font);
	AddVolumeContextMenus(fVolumeContextMenu);

	fWindowContextMenu = new BPopUpMenu("WindowContext", false, false);
	fWindowContextMenu->SetFont(be_plain_font);
	AddWindowContextMenus(fWindowContextMenu);

	fDropContextMenu = new BPopUpMenu("DropContext", false, false);
	fDropContextMenu->SetFont(be_plain_font);
	AddDropContextMenus(fDropContextMenu);
	
	fDragContextMenu = new BSlowContextMenu("DragContext");
		// will get added and built dynamically in ShowContextMenu
}


void
BContainerWindow::RepopulateMenus()
{
	// Avoid these menus to be destroyed:
	if (fMoveToItem && fMoveToItem->Menu())
		fMoveToItem->Menu()->RemoveItem(fMoveToItem);

	if (fCopyToItem && fCopyToItem->Menu())
		fCopyToItem->Menu()->RemoveItem(fCopyToItem);

	if (fCreateLinkItem && fCreateLinkItem->Menu())
		fCreateLinkItem->Menu()->RemoveItem(fCreateLinkItem);

	if (fOpenWithItem && fOpenWithItem->Menu()) {
		fOpenWithItem->Menu()->RemoveItem(fOpenWithItem);
		delete fOpenWithItem;
		fOpenWithItem = NULL;
	}

	if (fNavigationItem) {
		BMenu *menu = fNavigationItem->Menu();
		if (menu) {
			menu->RemoveItem(fNavigationItem);
			BMenuItem *item = menu->RemoveItem((int32)0);
			ASSERT(item != fNavigationItem);
			delete item;
		}
	}

	delete fFileContextMenu;
	fFileContextMenu = new BPopUpMenu("FileContext", false, false);
	fFileContextMenu->SetFont(be_plain_font);
	AddFileContextMenus(fFileContextMenu);

	delete fWindowContextMenu;
	fWindowContextMenu = new BPopUpMenu("WindowContext", false, false);
	fWindowContextMenu->SetFont(be_plain_font);
	AddWindowContextMenus(fWindowContextMenu);

	fMenuBar->RemoveItem(fFileMenu);
	delete fFileMenu;
	fFileMenu = new BMenu("File");
	AddFileMenu(fFileMenu);
	fMenuBar->AddItem(fFileMenu);

	fMenuBar->RemoveItem(fWindowMenu);
	delete fWindowMenu;
	fWindowMenu = new BMenu("Window");
	fMenuBar->AddItem(fWindowMenu);
	AddWindowMenu(fWindowMenu);

	// just create the attribute, decide to add it later
	fMenuBar->RemoveItem(fAttrMenu);
	delete fAttrMenu;
	fAttrMenu = new BMenu("Attributes");
	NewAttributeMenu(fAttrMenu);
	if (PoseView()->ViewMode() == kListMode)
		ShowAttributeMenu();

	int32 selectCount = PoseView()->SelectionList()->CountItems();

	SetupOpenWithMenu(fFileMenu);
	SetupMoveCopyMenus(selectCount
			? PoseView()->SelectionList()->FirstItem()->TargetModel()->EntryRef() : NULL,
		fFileMenu);
}


void
BContainerWindow::Init(const BMessage *message)
{
	float y_delta;
	BEntry entry;

	ASSERT(fPoseView);
	if (!fPoseView)
		return;

	// deal with new unconfigured folders
	if (NeedsDefaultStateSetup())
		SetUpDefaultState();

	if (ShouldAddScrollBars())
		fPoseView->AddScrollBars();

	fMoveToItem = new BMenuItem(new BNavMenu("Move to", kMoveSelectionTo, this));
	fCopyToItem = new BMenuItem(new BNavMenu("Copy to", kCopySelectionTo, this));
	fCreateLinkItem = new BMenuItem(new BNavMenu("Create Link", kCreateLink, this),
		new BMessage(kCreateLink));

	TrackerSettings settings;

	if (ShouldAddMenus()) {
		// add menu bar, menus and resize poseview to fit
		fMenuBar = new BMenuBar(BRect(0, 0, Bounds().Width() + 1, 1), "MenuBar");
		fMenuBar->SetBorder(B_BORDER_FRAME);
		AddMenus();
		AddChild(fMenuBar);

		y_delta = KeyMenuBar()->Bounds().Height() + 1;

		float navigatorDelta = 0;

		if (Navigator() && settings.ShowNavigator()) {
			Navigator()->MoveTo(BPoint(0, y_delta));
			navigatorDelta = BNavigator::CalcNavigatorHeight() + 1;
		}

		fPoseView->MoveTo(BPoint(0, navigatorDelta + y_delta));
		fPoseView->ResizeBy(0, -(y_delta));
		if (fPoseView->VScrollBar()) {
			fPoseView->VScrollBar()->MoveBy(0, KeyMenuBar()->Bounds().Height() + 1);
			fPoseView->VScrollBar()->ResizeBy(0, -(KeyMenuBar()->Bounds().Height() + 1));
		}

		// add folder icon to menu bar
		if (!TargetModel()->IsRoot() && !IsTrash() && !IsPrintersDir()) {
			float iconSize = fMenuBar->Bounds().Height() - 2;
			if (iconSize < 16)
				iconSize = 16;
			float iconPosY = 1 + (fMenuBar->Bounds().Height() - 2 - iconSize) / 2;
			BView *icon = new DraggableContainerIcon(BRect(Bounds().Width() - 4 - iconSize + 1, 
					iconPosY, Bounds().Width() - 4, iconPosY + iconSize - 1),
				"ThisContainer", B_FOLLOW_RIGHT);
			fMenuBar->AddChild(icon);
		}
	} else {
		// add equivalents of the menu shortcuts to the menuless desktop window
		AddShortcuts();
	}

	AddContextMenus();
	AddShortcut('T', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kDelete), PoseView());
	AddShortcut('K', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kCleanupAll),
		PoseView());
	AddShortcut('Q', B_COMMAND_KEY | B_OPTION_KEY | B_SHIFT_KEY | B_CONTROL_KEY,
		new BMessage(kQuitTracker));

	AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY, new BMessage(kOpenSelection),
		PoseView());

	SetSingleWindowBrowseShortcuts(settings.SingleWindowBrowse());

#if DEBUG
	// add some debugging shortcuts
	AddShortcut('D', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage('dbug'), PoseView());
	AddShortcut('C', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage('dpcc'), PoseView());
	AddShortcut('F', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage('dpfl'), PoseView());
	AddShortcut('F', B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY,
		new BMessage('dpfL'), PoseView());
#endif

	if (message)
		RestoreState(*message);
	else
		RestoreState();

	if (ShouldAddMenus() && PoseView()->ViewMode() == kListMode) {
		// for now only show attributes in list view
		// eventually enable attribute menu to allow users to select
		// using different attributes as titles in icon view modes
		ShowAttributeMenu();
	}
	MarkAttributeMenu(fAttrMenu);
	CheckScreenIntersect();

	if (fBackgroundImage && !dynamic_cast<BDeskWindow *>(this)
		&& PoseView()->ViewMode() != kListMode)
		fBackgroundImage->Show(PoseView(), current_workspace());

	Show();

	// done showing, turn the B_NO_WORKSPACE_ACTIVATION flag off;
	// it was on to prevent workspace jerking during boot
	SetFlags(Flags() & ~B_NO_WORKSPACE_ACTIVATION);
}


void
BContainerWindow::RestoreState()
{
	SetSizeLimits(kContainerWidthMinLimit, 10000, kContainerWindowHeightLimit, 10000);
	
	UpdateTitle();

	WindowStateNodeOpener opener(this, false);
	RestoreWindowState(opener.StreamNode());
	fPoseView->Init(opener.StreamNode());

	RestoreStateCommon();
}


void
BContainerWindow::RestoreState(const BMessage &message)
{
	SetSizeLimits(kContainerWidthMinLimit, 10000, kContainerWindowHeightLimit, 10000);
	
	UpdateTitle();

	RestoreWindowState(message);
	fPoseView->Init(message);
	
	RestoreStateCommon();
}


void
BContainerWindow::RestoreStateCommon()
{
	if (BootedInSafeMode())
		// don't pick up backgrounds in safe mode
		return;

	WindowStateNodeOpener opener(this, false);

	bool isDesktop = dynamic_cast<BDeskWindow *>(this);
	if (!TargetModel()->IsRoot() && opener.Node())
		// don't pick up background image for root disks
		// to do this, would have to have a unique attribute for the
		// disks window that doesn't collide with the desktop
		// for R4 this was not done to make things simpler
		// the default image will still work though
		fBackgroundImage = BackgroundImage::GetBackgroundImage(
			opener.Node(), isDesktop);
			// look for background image info in the window's node
	
	BNode defaultingNode;
	if (!fBackgroundImage && !isDesktop
		&& DefaultStateSourceNode(kDefaultFolderTemplate, &defaultingNode)) 
		// look for background image info in the source for defaults
		fBackgroundImage = BackgroundImage::GetBackgroundImage(&defaultingNode, isDesktop);
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
	} else
		// use the default look
		SetTitle(TargetModel()->Name());

	if (Navigator())
		Navigator()->UpdateLocation(PoseView()->TargetModel(), kActionUpdatePath);	
}


void
BContainerWindow::UpdateBackgroundImage()
{
	if (BootedInSafeMode())
		return;

	bool isDesktop = dynamic_cast<BDeskWindow *>(this) != NULL;
	WindowStateNodeOpener opener(this, false);

	if (!TargetModel()->IsRoot() && opener.Node())
		fBackgroundImage = BackgroundImage::Refresh(fBackgroundImage,
			opener.Node(), isDesktop, PoseView());
	
		// look for background image info in the window's node
	BNode defaultingNode;
	if (!fBackgroundImage && !isDesktop
		&& DefaultStateSourceNode(kDefaultFolderTemplate, &defaultingNode)) 
		// look for background image info in the source for defaults
		fBackgroundImage = BackgroundImage::Refresh(fBackgroundImage,
			&defaultingNode, isDesktop, PoseView());
}


void
BContainerWindow::FrameResized(float, float)
{
	if (PoseView() && dynamic_cast<BDeskWindow *>(this) == NULL) {
		BRect extent = PoseView()->Extent();
		if (extent.bottom < PoseView()->Bounds().bottom
			&& fPreviousBounds.Height() < Bounds().Height()) {
			PoseView()->ScrollBy(0, max_c(extent.bottom - PoseView()->Bounds().bottom,
				fPreviousBounds.Height() - Bounds().Height()));
		}
		if (extent.right < PoseView()->Bounds().right
			&& fPreviousBounds.Width() < Bounds().Width()) {
			PoseView()->ScrollBy(max_c(extent.right - PoseView()->Bounds().right,
				fPreviousBounds.Width() - Bounds().Width()), 0);
		}
		PoseView()->UpdateScrollRange();
		PoseView()->ResetPosePlacementHint();
	}

	fPreviousBounds = Bounds();
	fStateNeedsSaving = true;
}


void
BContainerWindow::FrameMoved(BPoint)
{
	fStateNeedsSaving = true;
}


void
BContainerWindow::WorkspacesChanged(uint32, uint32)
{
	fStateNeedsSaving = true;
}


void 
BContainerWindow::ViewModeChanged(uint32 oldMode, uint32 newMode)
{
	BView *view = FindView("MenuBar");
	if (view != NULL) {
		// make sure the draggable icon hides if it doesn't have space left anymore
		view = view->FindView("ThisContainer");
		if (view != NULL)
			view->FrameMoved(view->Frame().LeftTop());
	}

	if (!fBackgroundImage)
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
		if (opener.StreamNode())
			SaveWindowState(opener.StreamNode());
		if (hide)
			Hide();
		if (opener.StreamNode())
			fPoseView->SaveState(opener.StreamNode());

		fStateNeedsSaving = false;
	}
}


void
BContainerWindow::SaveState(BMessage &message) const
{
	if (SaveStateIsEnabled()) {
		SaveWindowState(message);
		fPoseView->SaveState(message);
	}
}


bool
BContainerWindow::StateNeedsSaving() const
{
	return fStateNeedsSaving || PoseView()->StateNeedsSaving();	
}


status_t 
BContainerWindow::GetLayoutState(BNode *node, BMessage *message)
{
	// ToDo:
	// get rid of this, use AttrStream instead
	status_t result = node->InitCheck();
	if (result != B_OK)
		return result;

	node->RewindAttrs();
	char attrName[256];
	while (node->GetNextAttrName(attrName) == B_OK) {
		attr_info info;
		node->GetAttrInfo(attrName, &info);

		// filter out attributes that are not related to window position
		// and column resizing
		// more can be added as needed
		if (strcmp(attrName, kAttrWindowFrame) != 0
			&& strcmp(attrName, kAttrColumns) != 0
			&& strcmp(attrName, kAttrViewState) != 0
			&& strcmp(attrName, kAttrColumnsForeign) != 0
			&& strcmp(attrName, kAttrViewStateForeign) != 0)
			continue;

		char *buffer = new char[info.size];
		if (node->ReadAttr(attrName, info.type, 0, buffer, (size_t)info.size) == info.size)
			message->AddData(attrName, info.type, buffer, (ssize_t)info.size);
		delete [] buffer;
	}
	return B_OK;
}


status_t 
BContainerWindow::SetLayoutState(BNode *node, const BMessage *message)
{
	status_t result = node->InitCheck();
	if (result != B_OK)
		return result;

	for (int32 globalIndex = 0; ;) {
#if B_BEOS_VERSION_DANO
 		const char *name;
#else
		char *name;
#endif
		type_code type;
		int32 count;
		status_t result = message->GetInfo(B_ANY_TYPE, globalIndex, &name,
			&type, &count);
		if (result != B_OK)
			break;

		for (int32 index = 0; index < count; index++) {
			const void *buffer;
			int32 size;
			result = message->FindData(name, type, index, &buffer, &size);
			if (result != B_OK) {
				PRINT(("error reading %s \n", name));
				return result;
			}

			if (node->WriteAttr(name, type, 0, buffer, (size_t)size) != size) {
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


bool
BContainerWindow::ShouldAddCountView() const
{
	return true;
}


Model *
BContainerWindow::TargetModel() const
{
	return fPoseView->TargetModel();
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

	if (fSavedZoomRect == Frame())
		if (oldZoomRect.IsValid())
			ResizeTo(oldZoomRect.Width(), oldZoomRect.Height());
}


void
BContainerWindow::ResizeToFit()
{
	BScreen screen(this);
	BRect screenFrame(screen.Frame());

	screenFrame.InsetBy(5, 5);
	screenFrame.top += 15;		// keeps title bar of window visible

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

	if (PoseView()->Bounds().bottom > extent.bottom && PoseView()->Bounds().top < 0)
		PoseView()->ScrollBy(0, extent.bottom - PoseView()->Bounds().bottom);
	if (PoseView()->Bounds().right > extent.right && PoseView()->Bounds().left < 0)
		PoseView()->ScrollBy(extent.right - PoseView()->Bounds().right, 0);

	PoseView()->UpdateScrollRange();
	PoseView()->EnableScrollBars();
}


void
BContainerWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_CUT:
		case B_COPY:
		case B_PASTE:
		case kCutMoreSelectionToClipboard:
		case kCopyMoreSelectionToClipboard:
		case kPasteLinksFromClipboard:
		{
			BView *view = CurrentFocus();
			if (view->LockLooper()) {
				view->MessageReceived(message);
				view->UnlockLooper();
			}
			break;
		}

		case kNewFolder:
			PostMessage(message, PoseView());
			break;

		case kContextMenuDragNDrop:
			//
			//	sent when the SlowContextPopup goes away
			//
			if (fWaitingForRefs && Dragging())
				PostMessage(message, PoseView());
			else
				fWaitingForRefs = false;
			break;

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

			PoseView()->MoveSelectionInto(&model, this, false);
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

				PoseView()->MoveSelectionInto(&model, this, false, true,
					message->what == kCreateRelativeLink);
			} else {
				// no destination specified, create link in same dir as item
				if (!TargetModel()->IsQuery())
					PoseView()->MoveSelectionInto(TargetModel(), this, false, true,
						(message->what == kCreateRelativeLink));
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
			if (message->FindRef("refs", &ref) == B_OK) {
				BEntry entry;
				if (entry.SetTo(&ref) == B_OK) {
					if (StateNeedsSaving())
						SaveState(false);

					bool wasInTrash = IsTrash() || InTrash();
					bool isRoot = PoseView()->TargetModel()->IsRoot();

					// Switch dir and apply new state
					WindowStateNodeOpener opener(this, false);
					opener.SetTo(&entry, false);

					// Update PoseView
					PoseView()->SwitchDir(&ref, opener.StreamNode());

					fIsTrash = FSIsTrashDir(&entry);
					fInTrash = FSInTrashDir(&ref);

					if (wasInTrash ^ (IsTrash() || InTrash())
						|| isRoot != PoseView()->TargetModel()->IsRoot())
						RepopulateMenus();

					// Update Navigation bar
					if (Navigator()) {
						int32 action = kActionSet;
						if (message->FindInt32("action", &action) != B_OK)
							// Design problem? Why does FindInt32 touch 
							// 'action' at all if he can't find it??
							action = kActionSet;
						
						Navigator()->UpdateLocation(PoseView()->TargetModel(), action);
					}

					TrackerSettings settings;
					if (settings.ShowNavigator() || settings.ShowFullPathInTitleBar())
						SetPathWatchingEnabled(true);

					// Update draggable folder icon
					BView *view = FindView("MenuBar");
					if (view != NULL) {
						view = view->FindView("ThisContainer");
						if (view != NULL) {
							IconCache::sIconCache->IconChanged(TargetModel());
							view->Invalidate();
						}
					}

					// Update window title
					UpdateTitle();
				}
			}
			break;
		}

		case B_REFS_RECEIVED:
			if (Dragging()) {
				//
				//	ref in this message is the target,
				//	the end point of the drag
				//
				entry_ref ref;
				if (message->FindRef("refs", &ref) == B_OK) {
					//printf("BContainerWindow::MessageReceived - refs received\n");
					fWaitingForRefs = false;
					BEntry entry(&ref, true);
					//
					//	don't copy to printers dir
					if (!FSIsPrintersDir(&entry)) {
						if (entry.InitCheck() == B_OK && entry.IsDirectory()) {
							//
							//	build of list of entry_refs from the list
							//	in the drag message
							entry_ref dragref;
							int32 index = 0;
							BObjectList<entry_ref> *list = new BObjectList<entry_ref>(0, true);
							while (fDragMessage->FindRef("refs", index++, &dragref) == B_OK)
								list->AddItem(new entry_ref(dragref));
							//
							//	compare the target and one of the drag items' parent
							//	
							BEntry item(&dragref, true);
							BEntry itemparent;
							item.GetParent(&itemparent);
							entry_ref parentref;
							itemparent.GetRef(&parentref);
							
							entry_ref targetref;
							entry.GetRef(&targetref);

							//	if they don't match, move/copy
							if (targetref != parentref) 
								//	copy drag contents to target ref in message
								FSMoveToFolder(list, new BEntry(entry), kMoveSelectionTo);

						} else {
							// 	current message sent to apps is only B_REFS_RECEIVED
							fDragMessage->what = B_REFS_RECEIVED;
							FSLaunchItem(&ref, (const BMessage *)fDragMessage, true, true);
						}
					}
				}
				DragStop();
			}
			break;

		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 observerWhat;
			if (message->FindInt32("be:observe_change_what", &observerWhat) == B_OK) {
				TrackerSettings settings;
				switch (observerWhat) {
					case kWindowsShowFullPathChanged:
						UpdateTitle();
						if (!IsPathWatchingEnabled() && settings.ShowFullPathInTitleBar())
							SetPathWatchingEnabled(true);
						if (IsPathWatchingEnabled() && !(settings.ShowNavigator() || settings.ShowFullPathInTitleBar()))
							SetPathWatchingEnabled(false);
						break;

					case kSingleWindowBrowseChanged:
						if (settings.SingleWindowBrowse()
							&& !Navigator()
							&& TargetModel()->IsDirectory()
							&& !PoseView()->IsFilePanel()
							&& !PoseView()->IsDesktopWindow()) {
							BRect rect(Bounds());
							rect.top = KeyMenuBar()->Bounds().Height() + 1;
							rect.bottom = rect.top + BNavigator::CalcNavigatorHeight();
							fNavigator = new BNavigator(TargetModel(), rect);
							fNavigator->Hide();
							AddChild(fNavigator);
							SetPathWatchingEnabled(settings.ShowNavigator() || settings.ShowFullPathInTitleBar());
						}
						SetSingleWindowBrowseShortcuts(settings.SingleWindowBrowse());
						break;

					case kShowNavigatorChanged:
						ShowNavigator(settings.ShowNavigator());
						if (!IsPathWatchingEnabled() && settings.ShowNavigator())
							SetPathWatchingEnabled(true);
						if (IsPathWatchingEnabled() && !(settings.ShowNavigator() || settings.ShowFullPathInTitleBar()))
							SetPathWatchingEnabled(false);
						break;
						
					case kDontMoveFilesToTrashChanged:
						{
							bool dontMoveToTrash = settings.DontMoveFilesToTrash();

							BMenuItem *item = fFileContextMenu->FindItem(kMoveToTrash);
							if (item)
								item->SetLabel(dontMoveToTrash ? "Delete" : "Move To Trash");

							// Deskbar doesn't have a menu bar, so check if there is fMenuBar
							if (fMenuBar && fFileMenu) {
								item = fFileMenu->FindItem(kMoveToTrash);
								if (item)
									item->SetLabel(dontMoveToTrash ? "Delete" : "Move To Trash");
							}
							UpdateIfNeeded();
						}
						break;

					default:
						_inherited::MessageReceived(message);
				}
			}		
			break;
		}

		case B_NODE_MONITOR:
			UpdateTitle();
			break;

		case B_UNDO:
			FSUndo();
			break;

		//case B_REDO:	/* only defined in Dano/Zeta/OpenBeOS */
		case kRedo:
			FSRedo();
			break;

		default:
			_inherited::MessageReceived(message);
	}
}


void
BContainerWindow::SetCutItem(BMenu *menu)
{
	BMenuItem *item;
	if ((item = menu->FindItem(B_CUT)) == NULL
		&& (item = menu->FindItem(kCutMoreSelectionToClipboard)) == NULL)
		return;

	item->SetEnabled(PoseView()->SelectionList()->CountItems() > 0
		|| PoseView() != CurrentFocus());

	if (modifiers() & B_SHIFT_KEY) {
		item->SetLabel("Cut more");
		item->SetShortcut('X', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kCutMoreSelectionToClipboard));
	} else {
		item->SetLabel("Cut");
		item->SetShortcut('X', B_COMMAND_KEY);
		item->SetMessage(new BMessage(B_CUT));
	}
}


void
BContainerWindow::SetCopyItem(BMenu *menu)
{
	BMenuItem *item;
	if ((item = menu->FindItem(B_COPY)) == NULL
		&& (item = menu->FindItem(kCopyMoreSelectionToClipboard)) == NULL)
		return;

	item->SetEnabled(PoseView()->SelectionList()->CountItems() > 0
		|| PoseView() != CurrentFocus());

	if (modifiers() & B_SHIFT_KEY) {
		item->SetLabel("Copy more");
		item->SetShortcut('C', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kCopyMoreSelectionToClipboard));
	} else {
		item->SetLabel("Copy");
		item->SetShortcut('C', B_COMMAND_KEY);
		item->SetMessage(new BMessage(B_COPY));
	}
}


void
BContainerWindow::SetPasteItem(BMenu *menu)
{
	BMenuItem *item;
	if ((item = menu->FindItem(B_PASTE)) == NULL
		&& (item = menu->FindItem(kPasteLinksFromClipboard)) == NULL)
		return;

	item->SetEnabled(FSClipboardHasRefs() || PoseView() != CurrentFocus());

	if (modifiers() & B_SHIFT_KEY) {
		item->SetLabel("Paste links");
		item->SetShortcut('V', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kPasteLinksFromClipboard));
	} else {
		item->SetLabel("Paste");
		item->SetShortcut('V', B_COMMAND_KEY);
		item->SetMessage(new BMessage(B_PASTE));
	}
}


void
BContainerWindow::SetCleanUpItem(BMenu *menu)
{
	BMenuItem *item;
	if ((item = menu->FindItem(kCleanup)) == NULL
		&& (item = menu->FindItem(kCleanupAll)) == NULL)
		return;

	item->SetEnabled(PoseView()->CountItems() > 0
		&& (PoseView()->ViewMode() != kListMode));

	if (modifiers() & B_SHIFT_KEY) {
		item->SetLabel("Clean Up All");
		item->SetShortcut('K', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kCleanupAll));
	} else {
		item->SetLabel("Clean Up");
		item->SetShortcut('K', B_COMMAND_KEY);
		item->SetMessage(new BMessage(kCleanup));
	}
}


void
BContainerWindow::SetCloseItem(BMenu *menu)
{
	BMenuItem *item;	
	if ((item = menu->FindItem(B_QUIT_REQUESTED)) == NULL
		&& (item = menu->FindItem(kCloseAllWindows)) == NULL)
		return;

	if (modifiers() & B_OPTION_KEY) {
		item->SetLabel("Close All");
		item->SetShortcut('W', B_COMMAND_KEY | B_OPTION_KEY);
		item->SetTarget(be_app);
		item->SetMessage(new BMessage(kCloseAllWindows));
	} else {
		item->SetLabel("Close");
		item->SetShortcut('W', B_COMMAND_KEY);
		item->SetTarget(this);
		item->SetMessage(new BMessage(B_QUIT_REQUESTED));
	}
}


bool
BContainerWindow::IsShowing(const node_ref *node) const
{
	return PoseView()->Represents(node);
}


bool
BContainerWindow::IsShowing(const entry_ref *entry) const
{
	return PoseView()->Represents(entry);
}


void
BContainerWindow::AddMenus()
{
	fFileMenu = new BMenu("File");
	AddFileMenu(fFileMenu);
	fMenuBar->AddItem(fFileMenu);
	fWindowMenu = new BMenu("Window");
	fMenuBar->AddItem(fWindowMenu);
	AddWindowMenu(fWindowMenu);
	// just create the attribute, decide to add it later
	fAttrMenu = new BMenu("Attributes");
	NewAttributeMenu(fAttrMenu);
}


void
BContainerWindow::AddFileMenu(BMenu *menu)
{
	if (!PoseView()->IsFilePanel())
		menu->AddItem(new BMenuItem("Find"B_UTF8_ELLIPSIS,
			new BMessage(kFindButton), 'F'));

	if (!TargetModel()->IsQuery() && !IsTrash() && !IsPrintersDir()) {
		if (!PoseView()->IsFilePanel()) {
			TemplatesMenu *templateMenu = new TemplatesMenu(PoseView());
			menu->AddItem(templateMenu);
			templateMenu->SetTargetForItems(PoseView());
		} else
			menu->AddItem(new BMenuItem("New Folder", new BMessage(kNewFolder), 'N'));
	}
	menu->AddSeparatorItem();
	
	menu->AddItem(new BMenuItem("Open", new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem("Get Info", new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem("Edit Name", new BMessage(kEditItem), 'E'));

	if (IsTrash() || InTrash()) {
		menu->AddItem(new BMenuItem("Delete", new BMessage(kDelete)));
		menu->AddItem(new BMenuItem("Restore", new BMessage(kRestoreFromTrash)));
		if (IsTrash()) {
			// add as first item in menu
			menu->AddItem(new BMenuItem("Empty Trash", new BMessage(kEmptyTrash)), 0);
			menu->AddItem(new BSeparatorItem(), 1);
		}
	} else if (IsPrintersDir()) {
		menu->AddItem(new BMenuItem("Add Printer"B_UTF8_ELLIPSIS,
			new BMessage(kAddPrinter), 'N'), 0);
		menu->AddItem(new BSeparatorItem(), 1);
		menu->AddItem(new BMenuItem("Make Active Printer",
			new BMessage(kMakeActivePrinter)));
	} else {
		menu->AddItem(new BMenuItem("Duplicate",new BMessage(kDuplicateSelection), 'D'));

		menu->AddItem(new BMenuItem(TrackerSettings().DontMoveFilesToTrash() ?
				"Delete" : "Move to Trash",
				new BMessage(kMoveToTrash), 'T'));

		menu->AddSeparatorItem();

		// The "Move To", "Copy To", "Create Link" menus are inserted
		// at this place, have a look at:
		// BContainerWindow::SetupMoveCopyMenus()
	}
	
	BMenuItem *cutItem = NULL, *copyItem = NULL, *pasteItem = NULL;
	if (!IsPrintersDir()) {
		menu->AddSeparatorItem();

		menu->AddItem(cutItem = new BMenuItem("Cut", new BMessage(B_CUT), 'X'));
		menu->AddItem(copyItem = new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
		menu->AddItem(pasteItem = new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));

		menu->AddSeparatorItem();
				
		menu->AddItem(new BMenuItem("Identify", new BMessage(kIdentifyEntry)));
		BMenu *addOnMenuItem = new BMenu(kAddOnsMenuName);
		addOnMenuItem->SetFont(be_plain_font);
		menu->AddItem(addOnMenuItem);
	}

	menu->SetTargetForItems(PoseView());
	if (cutItem)
		cutItem->SetTarget(this);
	if (copyItem)
		copyItem->SetTarget(this);
	if (pasteItem)
		pasteItem->SetTarget(this);
}


void
BContainerWindow::AddWindowMenu(BMenu *menu)
{
	BMenuItem *item;

	BMenu* iconSizeMenu = new BMenu("Icon View");

	BMessage* message = new BMessage(kIconMode);
	message->AddInt32("size", 32);
	item = new BMenuItem("32 x 32", message, '1');
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kScaleIconMode);
	message->AddInt32("size", 40);
	item = new BMenuItem("40 x 40", message);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kScaleIconMode);
	message->AddInt32("size", 48);
	item = new BMenuItem("48 x 48", message);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kScaleIconMode);
	message->AddInt32("size", 64);
	item = new BMenuItem("64 x 64", message);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	menu->AddItem(iconSizeMenu);

	item = new BMenuItem("Mini Icon View", new BMessage(kMiniIconMode), '2');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem("List View", new BMessage(kListMode), '3');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	menu->AddSeparatorItem();

	item = new BMenuItem("Resize to Fit", new BMessage(kResizeToFit), 'Y');
	item->SetTarget(this);
	menu->AddItem(item);

	item = new BMenuItem("Clean Up", new BMessage(kCleanup), 'K');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem("Select"B_UTF8_ELLIPSIS, new BMessage(kShowSelectionWindow),
		'A', B_SHIFT_KEY);
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem("Select All", new BMessage(B_SELECT_ALL), 'A');
	item->SetTarget(PoseView());
	menu->AddItem(item);
	
	item = new BMenuItem("Invert Selection", new BMessage(kInvertSelection), 'S');
	item->SetTarget(PoseView());
	menu->AddItem(item);
	
	if (!IsTrash()) {
		item = new BMenuItem("Open Parent", new BMessage(kOpenParentDir),
			B_UP_ARROW);
		item->SetTarget(PoseView());
		menu->AddItem(item);
	}

	item = new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W');
	item->SetTarget(this);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	item = new BMenuItem("Settings"B_UTF8_ELLIPSIS, new BMessage(kShowSettingsWindow));
	item->SetTarget(be_app);
	menu->AddItem(item);
}


void
BContainerWindow::AddShortcuts()
{
	// add equivalents of the menu shortcuts to the menuless desktop window
	ASSERT(!IsTrash());
	ASSERT(!PoseView()->IsFilePanel());
	ASSERT(!TargetModel()->IsQuery());

	AddShortcut('X', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kCutMoreSelectionToClipboard), this);
	AddShortcut('C', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kCopyMoreSelectionToClipboard), this);
	AddShortcut('F', B_COMMAND_KEY, new BMessage(kFindButton), PoseView());
	AddShortcut('N', B_COMMAND_KEY, new BMessage(kNewFolder), PoseView());
	AddShortcut('O', B_COMMAND_KEY, new BMessage(kOpenSelection), PoseView());
	AddShortcut('I', B_COMMAND_KEY, new BMessage(kGetInfo), PoseView());
	AddShortcut('E', B_COMMAND_KEY, new BMessage(kEditItem), PoseView());
	AddShortcut('D', B_COMMAND_KEY, new BMessage(kDuplicateSelection), PoseView());
	AddShortcut('T', B_COMMAND_KEY, new BMessage(kMoveToTrash), PoseView());
	AddShortcut('K', B_COMMAND_KEY, new BMessage(kCleanup), PoseView());
	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_SELECT_ALL), PoseView());
	AddShortcut('S', B_COMMAND_KEY, new BMessage(kInvertSelection), PoseView());
	AddShortcut('A', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kShowSelectionWindow), PoseView());
	AddShortcut('G', B_COMMAND_KEY, new BMessage(kEditQuery), PoseView());
		// it is ok to add a global Edit query shortcut here, PoseView will
		// filter out cases where selected pose is not a query
	AddShortcut('U', B_COMMAND_KEY, new BMessage(kUnmountVolume), PoseView());
	AddShortcut(B_UP_ARROW, B_COMMAND_KEY, new BMessage(kOpenParentDir), PoseView());
	AddShortcut('O', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage(kOpenSelectionWith),
		PoseView());
}


void
BContainerWindow::MenusBeginning()
{
	if (!fMenuBar)
		return;

	if (CurrentMessage() && CurrentMessage()->what == B_MOUSE_DOWN)
		// don't commit active pose if only a keyboard shortcut is
		// invoked - this would prevent Cut/Copy/Paste from working
		fPoseView->CommitActivePose();

	// File menu
	int32 selectCount = PoseView()->SelectionList()->CountItems();

	SetupOpenWithMenu(fFileMenu);
	SetupMoveCopyMenus(selectCount
		? PoseView()->SelectionList()->FirstItem()->TargetModel()->EntryRef() : NULL, fFileMenu);

	UpdateMenu(fMenuBar, kMenuBarContext);

	AddMimeTypesToMenu(fAttrMenu);

	if (IsPrintersDir())
		EnableNamedMenuItem(fFileMenu, "Make Active Printer", selectCount == 1);
}


void
BContainerWindow::MenusEnded()
{
	// when we're done we want to clear nav menus for next time
	DeleteSubmenu(fNavigationItem);
	DeleteSubmenu(fMoveToItem);
	DeleteSubmenu(fCopyToItem);
	DeleteSubmenu(fCreateLinkItem);
	DeleteSubmenu(fOpenWithItem);
}


void
BContainerWindow::SetupNavigationMenu(const entry_ref *ref, BMenu *parent)
{
	// start by removing nav item (and separator) from old menu
	if (fNavigationItem) {
		BMenu *menu = fNavigationItem->Menu();
		if (menu) {
			menu->RemoveItem(fNavigationItem);
			BMenuItem *item = menu->RemoveItem((int32)0);
			ASSERT(item != fNavigationItem);
			delete item;
		}
	}

	// if we weren't passed a ref then we're navigating this window
	if (!ref)
		ref = TargetModel()->EntryRef();

	BEntry entry;
	if (entry.SetTo(ref) != B_OK)
		return;

	// only navigate directories and queries (check for symlink here)
	Model model(&entry);
	entry_ref resolvedRef;

	if (model.InitCheck() != B_OK
		|| (!model.IsContainer() && !model.IsSymLink()))
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

	if (!fNavigationItem) {
		fNavigationItem = new ModelMenuItem(&model,
			new BNavMenu(model.Name(), B_REFS_RECEIVED, be_app, this));
	}

	// setup a navigation menu item which will dynamically load items
	// as menu items are traversed
	BNavMenu *navMenu = dynamic_cast<BNavMenu *>(fNavigationItem->Submenu());
	navMenu->SetNavDir(ref);
	fNavigationItem->SetLabel(model.Name());
	fNavigationItem->SetEntry(&entry);

	parent->AddItem(fNavigationItem, 0);
	parent->AddItem(new BSeparatorItem(), 1);
	
	BMessage *message = new BMessage(B_REFS_RECEIVED);
	message->AddRef("refs", ref);
	fNavigationItem->SetMessage(message);
	fNavigationItem->SetTarget(be_app);
	
	if (!Dragging())
		parent->SetTrackingHook(NULL, NULL);
}


void
BContainerWindow::SetUpEditQueryItem(BMenu *menu)
{
	ASSERT(menu);
	// File menu
	int32 selectCount = PoseView()->SelectionList()->CountItems();

	// add Edit query if appropriate
	bool queryInSelection = false;
	if (selectCount && selectCount < 100) {
		// only do this for a limited number of selected poses

		// if any queries selected, add an edit query menu item
		for (int32 index = 0; index < selectCount; index++) {
			BPose *pose = PoseView()->SelectionList()->ItemAt(index);
			Model model(pose->TargetModel()->EntryRef(), true);
			if (model.InitCheck() != B_OK)
				continue;

			if (model.IsQuery() || model.IsQueryTemplate()) {
				queryInSelection = true;
				break;
			}
		}
	}

	bool poseViewIsQuery = TargetModel()->IsQuery();
		// if the view is a query pose view, add edit query menu item

	BMenuItem *item = menu->FindItem("Edit Query");
	if (!poseViewIsQuery && !queryInSelection && item)
		item->Menu()->RemoveItem(item);
		
	else if ((poseViewIsQuery || queryInSelection) && menu && !item) {

		// add edit query item after Open 
		item = menu->FindItem(kOpenSelection);
		if (item) {	
			int32 itemIndex = item->Menu()->IndexOf(item);
			BMenuItem *query = new BMenuItem("Edit Query", new BMessage(kEditQuery), 'G');
			item->Menu()->AddItem(query, itemIndex + 1);
			query->SetTarget(PoseView());
		}
	}
}


void
BContainerWindow::SetupOpenWithMenu(BMenu *parent)
{
	// start by removing nav item (and separator) from old menu
	if (fOpenWithItem) {
		BMenu *menu = fOpenWithItem->Menu();
		if (menu) 
			menu->RemoveItem(fOpenWithItem);

		delete fOpenWithItem;
		fOpenWithItem = 0;
	}

	if (PoseView()->SelectionList()->CountItems() == 0)
		// no selection, nothing to open
		return;

	if (TargetModel()->IsRoot())
		// don't add ourselves if we are root
		return;

	// ToDo:
	// check if only item in selection list is the root
	// and do not add if true

	// add after "Open"
	BMenuItem *item = parent->FindItem(kOpenSelection);

	int32 count = PoseView()->SelectionList()->CountItems();
	if (!count)
		return;

	// build a list of all refs to open
	BMessage message(B_REFS_RECEIVED);
	for (int32 index = 0; index < count; index++) {
		BPose *pose = PoseView()->SelectionList()->ItemAt(index);
		message.AddRef("refs", pose->TargetModel()->EntryRef());
	}

	// add Tracker token so that refs received recipients can script us
	message.AddMessenger("TrackerViewToken", BMessenger(PoseView()));

	int32 index = item->Menu()->IndexOf(item);
	fOpenWithItem = new BMenuItem(
		new OpenWithMenu("Open With"B_UTF8_ELLIPSIS, &message, this, be_app),
		new BMessage(kOpenSelectionWith));
	fOpenWithItem->SetTarget(PoseView());
	fOpenWithItem->SetShortcut('O', B_COMMAND_KEY | B_CONTROL_KEY);

	item->Menu()->AddItem(fOpenWithItem, index + 1);
}


void
BContainerWindow::PopulateMoveCopyNavMenu(BNavMenu *navMenu, uint32 what,
	const entry_ref *ref, bool addLocalOnly)
{
	BVolume volume;
	BVolumeRoster volumeRoster;
	BDirectory directory;
	BEntry entry;
	BPath path;
	Model model;
	dev_t device = ref->device;

	int32 volumeCount = 0;

	// count persistent writable volumes
	volumeRoster.Rewind();
	while (volumeRoster.GetNextVolume(&volume) == B_OK)
		if (!volume.IsReadOnly() && volume.IsPersistent())
			volumeCount++;

	// add the current folder
	if (entry.SetTo(ref) == B_OK
		&& entry.GetParent(&entry) == B_OK
		&& model.SetTo(&entry) == B_OK) {
		BNavMenu *menu = new BNavMenu("Current Folder",what,this);
		menu->SetNavDir(model.EntryRef());
		menu->SetShowParent(true);

		BMenuItem *item = new SpecialModelMenuItem(&model,menu);
		item->SetMessage(new BMessage((uint32)what));

		navMenu->AddItem(item);
	}

	// add the recent folder menu
	// the "Tracker" settings directory is only used to get its icon
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("Tracker");
		if (entry.SetTo(path.Path()) == B_OK
			&& model.SetTo(&entry) == B_OK) {
			BMenu *menu = new RecentsMenu("Recent Folders",kRecentFolders,what,this);

			BMenuItem *item = new SpecialModelMenuItem(&model,menu);
			item->SetMessage(new BMessage((uint32)what));

			navMenu->AddItem(item);
		}
	}

	// add Desktop
	FSGetBootDeskDir(&directory);
	if (directory.InitCheck() == B_OK
		&& directory.GetEntry(&entry) == B_OK
		&& model.SetTo(&entry) == B_OK)
		navMenu->AddNavDir(&model, what, this, true);
			// ask NavMenu to populate submenu for us			

	// add the home dir
	if (find_directory(B_USER_DIRECTORY, &path) == B_OK
		&& entry.SetTo(path.Path()) == B_OK
		&& model.SetTo(&entry) == B_OK)
		navMenu->AddNavDir(&model, what, this, true);

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
				&& model.SetTo(&entry) == B_OK) 
				navMenu->AddNavDir(&model, what, this, true);
					// ask NavMenu to populate submenu for us
		}
	}
}


void
BContainerWindow::SetupMoveCopyMenus(const entry_ref *item_ref, BMenu *parent)
{
	if (IsTrash() || InTrash() || IsPrintersDir() || !fMoveToItem || !fCopyToItem || !fCreateLinkItem)
		return;

	// Grab the modifiers state since we use it twice
	uint32 modifierKeys = modifiers();

	// re-parent items to this menu since they're shared
 	int32 index;
 	BMenuItem *trash = parent->FindItem(kMoveToTrash);
 	if (trash)
 		index = parent->IndexOf(trash) + 2;
 	else
		index = 0;

	if (fMoveToItem->Menu() != parent) {
		if (fMoveToItem->Menu())
			fMoveToItem->Menu()->RemoveItem(fMoveToItem);

		parent->AddItem(fMoveToItem, index++);
	}

	if (fCopyToItem->Menu() != parent) {
		if (fCopyToItem->Menu())
			fCopyToItem->Menu()->RemoveItem(fCopyToItem);

		parent->AddItem(fCopyToItem, index++);
	}

	if (fCreateLinkItem->Menu() != parent) {
		if (fCreateLinkItem->Menu())
			fCreateLinkItem->Menu()->RemoveItem(fCreateLinkItem);

		parent->AddItem(fCreateLinkItem, index);
	}

    // Set the "Create Link" item label here so it
    // appears correctly when menus are disabled, too.
    if (modifierKeys & B_SHIFT_KEY)
    	fCreateLinkItem->SetLabel("Create Relative Link");
    else
    	fCreateLinkItem->SetLabel("Create Link");
    	
	// only enable once the menus are built
	fMoveToItem->SetEnabled(false);
	fCopyToItem->SetEnabled(false);
	fCreateLinkItem->SetEnabled(false);

	// get ref for item which is selected
	BEntry entry;
	if (entry.SetTo(item_ref) != B_OK)
		return;
			
	Model tempModel(&entry);
	if (tempModel.InitCheck() != B_OK)
		return;

	if (tempModel.IsRoot() || tempModel.IsVolume())
		return;

	// configure "Move to" menu item 
	PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu *>(fMoveToItem->Submenu()),
		kMoveSelectionTo, item_ref, true);

	// configure "Copy to" menu item 
	// add all mounted volumes (except the one this item lives on)
	PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu *>(fCopyToItem->Submenu()),
		kCopySelectionTo, item_ref, false);

	// Set "Create Link" menu item message and
	// add all mounted volumes (except the one this item lives on)
	if (modifierKeys & B_SHIFT_KEY) {
		fCreateLinkItem->SetMessage(new BMessage(kCreateRelativeLink));
		PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu *>(fCreateLinkItem->Submenu()),
			kCreateRelativeLink, item_ref, false);
	} else {
		fCreateLinkItem->SetMessage(new BMessage(kCreateLink));
		PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu *>(fCreateLinkItem->Submenu()),
		kCreateLink, item_ref, false);
	}

	fMoveToItem->SetEnabled(true);
	fCopyToItem->SetEnabled(true);
	fCreateLinkItem->SetEnabled(true);
}


uint32
BContainerWindow::ShowDropContextMenu(BPoint loc)
{
	BPoint global(loc);

	PoseView()->ConvertToScreen(&global);
	PoseView()->CommitActivePose();
	BRect mouseRect(global.x, global.y, global.x, global.y);
	mouseRect.InsetBy(-5, -5);

	// Change the "Create Link" item - allow user to
	// create relative links with the Shift key down.
	BMenuItem *item = fDropContextMenu->FindItem(kCreateLink);
	if (item == NULL)
		item = fDropContextMenu->FindItem(kCreateRelativeLink);
	if (item && (modifiers() & B_SHIFT_KEY)) {
		item->SetLabel("Create Relative Link Here");
		item->SetMessage(new BMessage(kCreateRelativeLink));
	} else if (item) {
		item->SetLabel("Create Link Here");
		item->SetMessage(new BMessage(kCreateLink));
	}

	item = fDropContextMenu->Go(global, true, true, mouseRect);
	if (item)
		return item->Command();

	return 0;
}


void
BContainerWindow::ShowContextMenu(BPoint loc, const entry_ref *ref, BView *)
{
	ASSERT(IsLocked());
	BPoint global(loc);
	PoseView()->ConvertToScreen(&global);
	PoseView()->CommitActivePose();
	BRect mouseRect(global.x, global.y, global.x, global.y);
	mouseRect.InsetBy(-5, -5);

	if (ref) {
		// clicked on a pose, show file or volume context menu
		Model model(ref);

		bool showAsVolume = false;
		bool filePanel = PoseView()->IsFilePanel();

		if (Dragging()) {
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
				// printf("ShowContextMenu - target is %s %i\n", ref->name, IsShowing(ref));
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
				fDragContextMenu->SetTypesList(fCachedTypesList);
				fDragContextMenu->SetTarget(BMessenger(this));
				BPoseView *poseView = PoseView();
				if (poseView) {
					BMessenger tmpTarget(poseView);
					fDragContextMenu->InitTrackingHook(
						&BPoseView::MenuTrackingHook, &tmpTarget, fDragMessage);
				}

				// this is now asynchronous so that we don't
				// deadlock in Window::Quit,
				fDragContextMenu->Go(global, true, false, true);
			}
			return;
		} else if (TargetModel()->IsRoot() || model.IsVolume()) {
			fContextMenu = fVolumeContextMenu;
			showAsVolume = true;
		} else
			fContextMenu = fFileContextMenu;

		// clean up items from last context menu

		if (fContextMenu) {
			if (fContextMenu->Window())
				return;
			else
				MenusEnded();

			if (model.InitCheck() == B_OK) { // ??? Do I need this ???
				if (showAsVolume) {
					// non-volume enable/disable copy, move, identify
					EnableNamedMenuItem(fContextMenu, kDuplicateSelection, false);
					EnableNamedMenuItem(fContextMenu, kMoveToTrash, false);
					EnableNamedMenuItem(fContextMenu, kIdentifyEntry, false);

					// volume model, enable/disable the Unmount item
					bool ejectableVolumeSelected = false;

					BVolume boot;
					BVolumeRoster().GetBootVolume(&boot);
					BVolume volume;
					volume.SetTo(model.NodeRef()->device);
					if (volume != boot)
						ejectableVolumeSelected = true;

					EnableNamedMenuItem(fContextMenu, "Unmount", ejectableVolumeSelected);
				}
			}

			SetupNavigationMenu(ref, fContextMenu);
			if (!showAsVolume && !filePanel) {
				SetupMoveCopyMenus(ref, fContextMenu);
				SetupOpenWithMenu(fContextMenu);
			}

			UpdateMenu(fContextMenu, kPosePopUpContext);

			fContextMenu->Go(global, true, false, mouseRect, true);
		}
	} else if (fWindowContextMenu) {
		if (fWindowContextMenu->Window())
			return;

		MenusEnded();		

		// clicked on a window, show window context menu

		SetupNavigationMenu(ref, fWindowContextMenu);
		UpdateMenu(fWindowContextMenu, kWindowPopUpContext);

		fWindowContextMenu->Go(global, true, false, mouseRect, true);
	}
	fContextMenu = NULL;
}


void 
BContainerWindow::AddFileContextMenus(BMenu *menu)
{
	menu->AddItem(new BMenuItem("Open", new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem("Get Info", new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem("Edit Name", new BMessage(kEditItem), 'E'));

	if (!IsTrash() && !InTrash() && !IsPrintersDir()) 
		menu->AddItem(new BMenuItem("Duplicate",
			new BMessage(kDuplicateSelection), 'D'));

	if (!IsTrash() && !InTrash()) {
		menu->AddItem(new BMenuItem(TrackerSettings().DontMoveFilesToTrash() ?
				"Delete" : "Move to Trash",
				new BMessage(kMoveToTrash), 'T'));

		// add separator for copy to/move to items (navigation items)
		menu->AddSeparatorItem();
	} else {
		menu->AddItem(new BMenuItem("Delete", new BMessage(kDelete), 0));
		menu->AddItem(new BMenuItem("Restore", new BMessage(kRestoreFromTrash), 0));
	}

#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	menu->AddSeparatorItem();
	BMenuItem *cutItem, *copyItem;
	menu->AddItem(cutItem = new BMenuItem("Cut", new BMessage(B_CUT), 'X'));
	menu->AddItem(copyItem = new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
#endif

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Identify", new BMessage(kIdentifyEntry)));
	BMenu *addOnMenuItem = new BMenu(kAddOnsMenuName);
	addOnMenuItem->SetFont(be_plain_font);
	menu->AddItem(addOnMenuItem);

	// set targets as needed
	menu->SetTargetForItems(PoseView());
#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	cutItem->SetTarget(this);
	copyItem->SetTarget(this);
#endif
}


void 
BContainerWindow::AddVolumeContextMenus(BMenu *menu)
{	
	menu->AddItem(new BMenuItem("Open", new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem("Get Info", new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem("Edit Name", new BMessage(kEditItem), 'E'));

	menu->AddSeparatorItem();
	menu->AddItem(new MountMenu("Mount"));

	BMenuItem *item = new BMenuItem("Unmount", new BMessage(kUnmountVolume), 'U');
	item->SetEnabled(false);
	menu->AddItem(item);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenu(kAddOnsMenuName));

	menu->SetTargetForItems(PoseView());
}


void 
BContainerWindow::AddWindowContextMenus(BMenu *menu)
{
	// create context sensitive menu for empty area of window
	// since we check view mode before display, this should be a radio
	// mode menu

	bool needSeparator = true;
	if (IsTrash()) 
		menu->AddItem(new BMenuItem("Empty Trash", new BMessage(kEmptyTrash)));
	else if (IsPrintersDir())
		menu->AddItem(new BMenuItem("Add Printer"B_UTF8_ELLIPSIS, new BMessage(kAddPrinter), 'N'));
	else if (InTrash())
		needSeparator = false;
	else {
		TemplatesMenu *templateMenu = new TemplatesMenu(PoseView());
		menu->AddItem(templateMenu);
		templateMenu->SetTargetForItems(PoseView());
		templateMenu->SetFont(be_plain_font);
	}

	if (needSeparator)
		menu->AddSeparatorItem();

#if 0
	BMenuItem *pasteItem = new BMenuItem("Paste", new BMessage(B_PASTE), 'V');
	menu->AddItem(pasteItem);
	menu->AddSeparatorItem();
#endif
	menu->AddItem(new BMenuItem("Clean Up", new BMessage(kCleanup), 'K'));
	menu->AddItem(new BMenuItem("Select"B_UTF8_ELLIPSIS,
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY));
	menu->AddItem(new BMenuItem("Select All", new BMessage(B_SELECT_ALL), 'A'));
	if (!IsTrash()) 
		menu->AddItem(new BMenuItem("Open Parent", new BMessage(kOpenParentDir),
			B_UP_ARROW));

	menu->AddSeparatorItem();
	BMenu *addOnMenuItem = new BMenu(kAddOnsMenuName);
	addOnMenuItem->SetFont(be_plain_font);
	menu->AddItem(addOnMenuItem);

#if DEBUG
	menu->AddSeparatorItem();
	BMenuItem *testing = new BMenuItem("Test Icon Cache", new BMessage(kTestIconCache));
	menu->AddItem(testing);
#endif

	// target items as needed
	menu->SetTargetForItems(PoseView());
#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	pasteItem->SetTarget(this);
#endif
}


void 
BContainerWindow::AddDropContextMenus(BMenu *menu)
{
	menu->AddItem(new BMenuItem("Create Link Here", new BMessage(kCreateLink)));
	menu->AddItem(new BMenuItem("Move Here", new BMessage(kMoveSelectionTo)));
	menu->AddItem(new BMenuItem("Copy Here", new BMessage(kCopySelectionTo)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Cancel", new BMessage(kCancelButton)));
}


void
BContainerWindow::EachAddon(bool (*eachAddon)(const Model *, const char *,
	uint32 shortcut, bool primary, void *context), void *passThru)
{
	BObjectList<Model> uniqueList(10, true);
	BPath path;
	bool bail = false;
	if (find_directory(B_BEOS_ADDONS_DIRECTORY, &path) == B_OK)
		bail = EachAddon(path, eachAddon, &uniqueList, passThru);

	if (!bail && find_directory(B_USER_ADDONS_DIRECTORY, &path) == B_OK)
		bail = EachAddon(path, eachAddon, &uniqueList, passThru);

	if (!bail && find_directory(B_COMMON_ADDONS_DIRECTORY, &path) == B_OK)
		EachAddon(path, eachAddon, &uniqueList, passThru);
}


bool
BContainerWindow::EachAddon(BPath &path, bool (*eachAddon)(const Model *,
	const char *, uint32 shortcut, bool primary, void *),
	BObjectList<Model> *uniqueList, void *params)
{
	path.Append("Tracker");

	BDirectory dir;
	BEntry entry;

	if (dir.SetTo(path.Path()) != B_OK)
		return false;

	// build a list of the MIME types of the selected items

	BObjectList<BString> mimeTypes(10, true);

	int32 count = PoseView()->SelectionList()->CountItems();
	if (!count) {
		// just add the type of the current directory
		AddMimeTypeString(mimeTypes, TargetModel());
	} else {
		for (int32 index = 0; index < count; index++) {
			BPose *pose = PoseView()->SelectionList()->ItemAt(index);
			AddMimeTypeString(mimeTypes, pose->TargetModel());
		}
	}

	dir.Rewind();
	while (dir.GetNextEntry(&entry) == B_OK) {
		bool primary = false;

		if (entry.IsSymLink()) {
			// resolve symlinks if needed
			entry_ref ref;
			entry.GetRef(&ref);
			entry.SetTo(&ref, true);
		}

		Model *model = new Model(&entry);
		if (model->InitCheck() != B_OK || !model->IsExecutable()) {
			delete model;
			continue;
		}

		// check if it supports at least one of the selected entries

		if (mimeTypes.CountItems()) {
			BFile file(&entry, B_READ_ONLY);
			if (file.InitCheck() == B_OK) {
				BAppFileInfo info(&file);
				if (info.InitCheck() == B_OK) {
					bool secondary = true;

					// does this add-on has types set at all?
					BMessage message;
					if (info.GetSupportedTypes(&message) == B_OK) {
						type_code type;
						int32 count;
						if (message.GetInfo("types", &type, &count) == B_OK)
							secondary = false;
					}

					// check all supported types if it has some set
					if (!secondary) {
						for (int32 i = mimeTypes.CountItems(); !primary && i-- > 0;) {
							BString *type = mimeTypes.ItemAt(i);
							if (info.IsSupportedType(type->String())) {
								BMimeType mimeType(type->String());
								if (info.Supports(&mimeType))
									primary = true;
								else
									secondary = true;
							}
						}
					}

					if (!secondary && !primary) {
						delete model;
						continue;
					}
				}
			}
		}

		char name[B_FILE_NAME_LENGTH];
		uint32 key;
		StripShortcut(model, name, key);

		// do a uniqueness check
		if (uniqueList->EachElement(MatchOne, name)) {
			// found one already in the list
			delete model;
			continue;
		}
		uniqueList->AddItem(model);

		if ((eachAddon)(model, name, key, primary, params))
			return true;
	}
	return false;
}


void
BContainerWindow::BuildAddOnMenu(BMenu *menu)
{
	BMenuItem *item = menu->FindItem(kAddOnsMenuName);
	if (menu->IndexOf(item) == 0) {
		// the folder of the context menu seems to be named "Add-Ons"
		// so we just take the last menu item, which is correct if not
		// build with debug option
		item = menu->ItemAt(menu->CountItems() - 1);
	}
	if (item == NULL)
		return;

	menu = item->Submenu();
	if (!menu)
		return;

	menu->SetFont(be_plain_font);

	// found the addons menu, empty it first
	for (;;) {
		item = menu->RemoveItem(0L);
		if (!item)
			break;
		delete item;
	}

	BObjectList<BMenuItem> primaryList;
	BObjectList<BMenuItem> secondaryList;

	AddOneAddonParams params;
	params.primaryList = &primaryList;
	params.secondaryList = &secondaryList;

	EachAddon(AddOneAddon, &params);

	primaryList.SortItems(CompareLabels);
	secondaryList.SortItems(CompareLabels);

	int32 count = primaryList.CountItems();
	for (int32 index = 0; index < count; index++)
		menu->AddItem(primaryList.ItemAt(index));

	if (count != 0)
		menu->AddSeparatorItem();

	count = secondaryList.CountItems();
	for (int32 index = 0; index < count; index++)
		menu->AddItem(secondaryList.ItemAt(index));

	menu->SetTargetForItems(this);
}


void
BContainerWindow::UpdateMenu(BMenu *menu, UpdateMenuContext context)
{
	const int32 selectCount = PoseView()->SelectionList()->CountItems();
	const int32 count = PoseView()->CountItems();

	if (context == kMenuBarContext) {
		EnableNamedMenuItem(menu, kOpenSelection, selectCount > 0);
		EnableNamedMenuItem(menu, kGetInfo, selectCount > 0);
		EnableNamedMenuItem(menu, kIdentifyEntry, selectCount > 0);
		EnableNamedMenuItem(menu, kMoveToTrash, selectCount > 0);
		EnableNamedMenuItem(menu, kRestoreFromTrash, selectCount > 0);
		EnableNamedMenuItem(menu, kDelete, selectCount > 0);
		EnableNamedMenuItem(menu, kDuplicateSelection, selectCount > 0);
	}

	if (context == kMenuBarContext || context == kPosePopUpContext) {
		SetUpEditQueryItem(menu);
		EnableNamedMenuItem(menu, kEditItem, selectCount == 1
			&& (context == kPosePopUpContext || !PoseView()->ActivePose()));
		SetCutItem(menu);
		SetCopyItem(menu);
		SetPasteItem(menu);
	}

	if (context == kMenuBarContext || context == kWindowPopUpContext) {
		BMenu* sizeMenu = NULL;
		if (BMenuItem* item = menu->FindItem("Icon View")) {
			sizeMenu = item->Submenu();
		}

		uint32 viewMode = PoseView()->ViewMode();
		if (sizeMenu) {
			if (viewMode == kIconMode || viewMode == kScaleIconMode) {
				int32 iconSize = (int32)PoseView()->IconSizeInt();
				for (int32 i = 0; BMenuItem* item = sizeMenu->ItemAt(i); i++) {
					BMessage* message = item->Message();
					if (!message) {
						item->SetMarked(false);
						continue;
					}
					int32 size;
					if (message->FindInt32("size", &size) < B_OK)
						size = -1;
					item->SetMarked(iconSize == size);
				}
			} else {
				for (int32 i = 0; BMenuItem* item = sizeMenu->ItemAt(i); i++)
					item->SetMarked(false);
			}
		} else {
			MarkNamedMenuItem(menu, kIconMode, viewMode == kIconMode);
		}

		MarkNamedMenuItem(menu, kListMode, viewMode == kListMode);
		MarkNamedMenuItem(menu, kMiniIconMode, viewMode == kMiniIconMode);

		SetCloseItem(menu);
		SetCleanUpItem(menu);
		SetPasteItem(menu);

		EnableNamedMenuItem(menu, kOpenParentDir, !TargetModel()->IsRoot());
		EnableNamedMenuItem(menu, kEmptyTrash, count > 0);
		EnableNamedMenuItem(menu, B_SELECT_ALL, count > 0);

		BMenuItem *item = menu->FindItem(kTemplatesMenuName);
		if (item) {
			TemplatesMenu *templateMenu = dynamic_cast<TemplatesMenu *>(
				item->Submenu());
			if (templateMenu)
				templateMenu->UpdateMenuState();
		}
	}

	BuildAddOnMenu(menu);
}


void
BContainerWindow::LoadAddOn(BMessage *message)
{
	UpdateIfNeeded();

	entry_ref addonRef;
	status_t result = message->FindRef("refs", &addonRef);
	if (result != B_OK) {
		char buffer[1024];
		sprintf(buffer, "Error %s loading Add-On %s.", strerror(result), addonRef.name);
		(new BAlert("", buffer, "Cancel", 0, 0,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		return;
	}

	// add selected refs to message
	BMessage *refs = new BMessage(B_REFS_RECEIVED);

	BObjectList<BPose> *list = PoseView()->SelectionList();

	int32 index = 0;
	BPose *pose;
	while ((pose = list->ItemAt(index++)) != NULL)
		refs->AddRef("refs", pose->TargetModel()->EntryRef());
	
	refs->AddMessenger("TrackerViewToken", BMessenger(PoseView()));
	
	LaunchInNewThread("Add-on", B_NORMAL_PRIORITY, &AddOnThread, refs, addonRef, 
		*TargetModel()->EntryRef());
}


BMenuItem * 
BContainerWindow::NewAttributeMenuItem(const char *label, const char *attrName, 
	int32 attrType, float attrWidth, int32 attrAlign, bool attrEditable, bool attrStatField)
{
	BMessage *message = new BMessage(kAttributeItem);
	message->AddString("attr_name", attrName);
	message->AddInt32("attr_type", attrType);
	message->AddInt32("attr_hash", (int32)AttrHashString(attrName, (uint32)attrType));
	message->AddFloat("attr_width", attrWidth);
	message->AddInt32("attr_align", attrAlign);
	message->AddBool("attr_editable", attrEditable);
	message->AddBool("attr_statfield", attrStatField);

	BMenuItem *menuItem = new BMenuItem(label, message);
	menuItem->SetTarget(PoseView());

	return menuItem;
}


void
BContainerWindow::NewAttributeMenu(BMenu *menu)
{
	ASSERT(PoseView());

	BMenuItem *item;
	menu->AddItem(item = new BMenuItem("Copy Attributes", new BMessage(kCopyAttributes)));
	item->SetTarget(PoseView());
	menu->AddItem(item = new BMenuItem("Paste Attributes", new BMessage(kPasteAttributes)));
	item->SetTarget(PoseView());
	menu->AddSeparatorItem();

	menu->AddItem(NewAttributeMenuItem ("Name", kAttrStatName, B_STRING_TYPE,
		145, B_ALIGN_LEFT, true, true));

	menu->AddItem(NewAttributeMenuItem ("Size", kAttrStatSize, B_OFF_T_TYPE,
		80, B_ALIGN_RIGHT, false, true));

	menu->AddItem(NewAttributeMenuItem ("Modified", kAttrStatModified, B_TIME_TYPE,
		150, B_ALIGN_LEFT, false, true));

	menu->AddItem(NewAttributeMenuItem ("Created", kAttrStatCreated, B_TIME_TYPE,
		150, B_ALIGN_LEFT, false, true));

	menu->AddItem(NewAttributeMenuItem ("Kind", kAttrMIMEType, B_MIME_STRING_TYPE,
		145, B_ALIGN_LEFT, false, false));

	if (IsTrash() || InTrash())
		menu->AddItem(NewAttributeMenuItem ("Original name", kAttrOriginalPath, B_STRING_TYPE,
			225, B_ALIGN_LEFT, false, false));
	else
		menu->AddItem(NewAttributeMenuItem ("Path", kAttrPath, B_STRING_TYPE,
			225, B_ALIGN_LEFT, false, false));

#ifdef OWNER_GROUP_ATTRIBUTES
	menu->AddItem(NewAttributeMenuItem ("Owner", kAttrStatOwner, B_STRING_TYPE,
		60, B_ALIGN_LEFT, false, true));

	menu->AddItem(NewAttributeMenuItem ("Group", kAttrStatGroup, B_STRING_TYPE,
		60, B_ALIGN_LEFT, false, true));
#endif

	menu->AddItem(NewAttributeMenuItem ("Permissions", kAttrStatMode, B_STRING_TYPE,
		80, B_ALIGN_LEFT, false, true));
}


void
BContainerWindow::ShowAttributeMenu()
{
	ASSERT(fAttrMenu);
	fMenuBar->AddItem(fAttrMenu);
}


void
BContainerWindow::HideAttributeMenu()
{
	ASSERT(fAttrMenu);
	fMenuBar->RemoveItem(fAttrMenu);
}


void 
BContainerWindow::MarkAttributeMenu()
{
	MarkAttributeMenu(fAttrMenu);
}


void
BContainerWindow::MarkAttributeMenu(BMenu *menu)
{
	if (!menu)
		return;

	int32 count = menu->CountItems();
	for (int32 index = 0; index < count; index++) {
		BMenuItem *item = menu->ItemAt(index);
		int32 attrHash;
		if (item->Message())
			if (item->Message()->FindInt32("attr_hash", &attrHash) == B_OK)
				item->SetMarked(PoseView()->ColumnFor((uint32)attrHash) != 0);
			else 
				item->SetMarked(false);

		BMenu *submenu = item->Submenu();
		if (submenu) {
			int32 count2 = submenu->CountItems();
			for (int32 subindex = 0; subindex < count2; subindex++) {
				item = submenu->ItemAt(subindex);
				if (item->Message())
					if (item->Message()->FindInt32("attr_hash", &attrHash) == B_OK) 
						item->SetMarked(PoseView()->ColumnFor((uint32)attrHash) != 0);
					else
						item->SetMarked(false);
			}
		}
	}
}


void 
BContainerWindow::AddMimeTypesToMenu()
{
	AddMimeTypesToMenu(fAttrMenu);
}


void
BContainerWindow::AddMimeTypesToMenu(BMenu *menu)
{
	if (!menu)
		return;

	// find start of mime types in menu
	int32 count = menu->CountItems();
	int32 start;

	for (start = 0; start < count; start++) { 
		if (menu->ItemAt(start)->Submenu())
			break;
	}

 	// Remove old mime menu:
 	int32 removeIndex = count - 1;
 	while (menu->ItemAt(removeIndex)->Submenu() != NULL) {
		delete menu->RemoveItem(removeIndex);
 		removeIndex--;
 	}
 
 	// Add a separator item if there is none yet
 	if (dynamic_cast<BSeparatorItem *>(menu->ItemAt(removeIndex)) == NULL)
		menu->AddSeparatorItem(); 		

	int32 typeCount = PoseView()->CountMimeTypes();

	for (int32 index = 0; index < typeCount; index++) {

		bool shouldAdd = true;
		const char *signature = PoseView()->MimeTypeAt(index);

		for (int32 subindex = start; subindex < count; subindex++) {
			BMenuItem *item = menu->ItemAt(subindex);
			if (!item)
				continue;
			BMessage *message = item->Message();
			if (!message)
				continue;
			const char *str;
			if (message->FindString("mimetype", &str) == B_OK
				&& strcmp(signature, str) == 0) {
				shouldAdd = false;
				break;
			}
		}

		if (shouldAdd) {
			BMessage attr_msg;
			char desc[B_MIME_TYPE_LENGTH];
			const char *nameToAdd = signature;

			BMimeType mimetype(signature);

			if (!mimetype.IsInstalled()) 
				continue;

			// only add things to menu which have "user-visible" data
			if (mimetype.GetAttrInfo(&attr_msg) != B_OK) 
				continue;

			if (mimetype.GetShortDescription(desc) == B_OK && desc[0])
				nameToAdd = desc;

					// go through each field in meta mime and add it to a menu
			BMenu *localMenu = 0;
			int32 index = -1;
			const char *str;

			while (attr_msg.FindString("attr:public_name", ++index, &str) == B_OK) {
				if (!attr_msg.FindBool("attr:viewable", index))
					// don't add if attribute not viewable
					continue;
					
				int32 type;
				int32 align;
				int32 width;
				bool editable;
				
				const char *attrName;
				
				if (attr_msg.FindString("attr:name", index, &attrName) != B_OK)
					continue;

				if (attr_msg.FindInt32("attr:type", index, &type) != B_OK)
					continue;

				if (attr_msg.FindBool("attr:editable", index, &editable) != B_OK)
					continue;

				if (attr_msg.FindInt32("attr:width", index, &width) != B_OK)
					continue;

				if (attr_msg.FindInt32("attr:alignment", index, &align) != B_OK)
					continue;

				if (!localMenu) {
					// do a lazy allocation of the menu
					localMenu = new BMenu(nameToAdd);
					BFont font;
					menu->GetFont(&font);
					localMenu->SetFont(&font);
				}
				localMenu->AddItem(NewAttributeMenuItem (str, attrName, type,
					width, align, editable, false));
			}
			if (localMenu) {
				BMessage *message = new BMessage(kMIMETypeItem);
				message->AddString("mimetype", signature);
				menu->AddItem(new IconMenuItem(localMenu, message, signature, B_MINI_ICON));
			}
		}
	}

	// remove separator if it's the only item in menu
	BMenuItem *item = menu->ItemAt(menu->CountItems() - 1);
	if (dynamic_cast<BSeparatorItem *>(item) != NULL) {
		menu->RemoveItem(item);
		delete item;
	}

	MarkAttributeMenu(menu);
}


BHandler *
BContainerWindow::ResolveSpecifier(BMessage *message, int32 index,
	BMessage *specifier, int32 form, const char	*property)
{
	if (strcmp(property, "Poses") == 0) {
//		PRINT(("BContainerWindow::ResolveSpecifier %s\n", property));
		message->PopSpecifier();
		return PoseView();
	}

	return _inherited::ResolveSpecifier(message, index, specifier,
		form, property);
}


PiggybackTaskLoop *
BContainerWindow::DelayedTaskLoop()
{
	if (!fTaskLoop)
		fTaskLoop = new PiggybackTaskLoop;

	return fTaskLoop;
}


bool 
BContainerWindow::NeedsDefaultStateSetup()
{
	if (!TargetModel())
		return false;

	if (TargetModel()->IsRoot())
		// don't try to set up anything if we are root
		return false;
		
	WindowStateNodeOpener opener(this, false);
	if (!opener.StreamNode())
		// can't read state, give up
		return false;
	
	return !NodeHasSavedState(opener.Node());
}


bool
BContainerWindow::DefaultStateSourceNode(const char *name, BNode *result,
	bool createNew, bool createFolder)
{
//	PRINT(("looking for default state in tracker settings dir\n"));
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
			const char *nextSlash = strchr(name, '/');
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
	
// 	PRINT(("using default state from %s\n", path.Path()));
	result->SetTo(path.Path());
	return result->InitCheck() == B_OK;
}


void 
BContainerWindow::SetUpDefaultState()
{
	BNode defaultingNode;
		// this is where we'll ulitimately get the state from
	bool gotDefaultingNode = 0;
	bool shouldStagger = false;
	
	ASSERT(TargetModel());

	PRINT(("folder %s does not have any saved state\n", TargetModel()->Name()));
	
	WindowStateNodeOpener opener(this, true);
		// this is our destination node, whatever it is for this window
	if (!opener.StreamNode())
		return;
	
	if (!TargetModel()->IsRoot()) {
		BDirectory desktop;
		FSGetDeskDir(&desktop, TargetModel()->EntryRef()->device);

		// try copying state from our parent directory, unless it is the desktop folder
		BEntry entry(TargetModel()->EntryRef());
		BDirectory parent;
		if (entry.GetParent(&parent) == B_OK && parent != desktop) {
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
		// a window, that's probably not a problem; would be OK if state got committed
		// after every change
		&& !DefaultStateSourceNode(kDefaultFolderTemplate, &defaultingNode, true))
		return;

	// copy over the attributes
	
	// set up a filter of the attributes we want copied
	const char *allowAttrs[] = {
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
	SelectiveAttributeTransformer frameOffsetter(kAttrWindowFrame, OffsetFrameOne, &params);
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
BContainerWindow::RestoreWindowState(AttributeStreamNode *node)
{
	if (!node || dynamic_cast<BDeskWindow *>(this))
		// don't restore any window state if we are a desktop window
		return;
		
	const char *rectAttributeName;
	const char *workspaceAttributeName;
	if (TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}
	
	BRect frame(Frame());
	if (node->Read(rectAttributeName, 0, B_RECT_TYPE, sizeof(BRect), &frame) == sizeof(BRect)) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	} else
		sNewWindRect.OffsetBy(kWindowStaggerBy, kWindowStaggerBy);

	fPreviousBounds = Bounds();

	uint32 workspace;
	if ((fContainerWindowFlags & kRestoreWorkspace)
		&& node->Read(workspaceAttributeName, 0, B_INT32_TYPE, sizeof(uint32), &workspace) == sizeof(uint32))
		SetWorkspaces(workspace);

	if (fContainerWindowFlags & kIsHidden)
		Minimize(true);

#if __HAIKU__
	// restore window decor settings
	int32 size = node->Contains(kAttrWindowDecor, B_RAW_TYPE);
	if (size > 0) {
		char buffer[size];
		if ((fContainerWindowFlags & kRestoreDecor)
			&& node->Read(kAttrWindowDecor, 0, B_RAW_TYPE, size, buffer) == size) {
			BMessage decorSettings;
			if (decorSettings.Unflatten(buffer) == B_OK)
				SetDecoratorSettings(decorSettings);
		}
	}
#endif // __HAIKU__
}


void 
BContainerWindow::RestoreWindowState(const BMessage &message)
{
	if (dynamic_cast<BDeskWindow *>(this))
		// don't restore any window state if we are a desktop window
		return;
		
	const char *rectAttributeName;
	const char *workspaceAttributeName;
	if (TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}

	BRect frame(Frame());
	if (message.FindRect(rectAttributeName, &frame) == B_OK) {	
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	} else
		sNewWindRect.OffsetBy(kWindowStaggerBy, kWindowStaggerBy);

	uint32 workspace;
	if ((fContainerWindowFlags & kRestoreWorkspace)
		&& message.FindInt32(workspaceAttributeName, (int32 *)&workspace) == B_OK) 
		SetWorkspaces(workspace);

	if (fContainerWindowFlags & kIsHidden)
		Minimize(true);

#if __HAIKU__
	// restore window decor settings
	BMessage decorSettings;
	if ((fContainerWindowFlags & kRestoreDecor)
		&& message.FindMessage(kAttrWindowDecor, &decorSettings) == B_OK) {
		SetDecoratorSettings(decorSettings);
	}
#endif // __HAIKU__
}


void 
BContainerWindow::SaveWindowState(AttributeStreamNode *node)
{
	ASSERT(node);
	const char *rectAttributeName;
	const char *workspaceAttributeName;
	if (TargetModel() && TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}
	
	// node is null if it already got deleted
	BRect frame(Frame());
	node->Write(rectAttributeName, 0, B_RECT_TYPE, sizeof(BRect), &frame);

	uint32 workspaces = Workspaces();
	node->Write(workspaceAttributeName, 0, B_INT32_TYPE, sizeof(uint32),
		&workspaces);

#if __HAIKU__
	BMessage decorSettings;
	if (GetDecoratorSettings(&decorSettings) == B_OK) {
		int32 size = decorSettings.FlattenedSize();
		char buffer[size];
		if (decorSettings.Flatten(buffer, size) == B_OK) {
			node->Write(kAttrWindowDecor, 0, B_RAW_TYPE, size, buffer);
		}
	}
#endif // __HAIKU__
}


void 
BContainerWindow::SaveWindowState(BMessage &message) const
{
	const char *rectAttributeName;
	const char *workspaceAttributeName;

	if (TargetModel() && TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}
	
	// node is null if it already got deleted
	BRect frame(Frame());
	message.AddRect(rectAttributeName, frame);
	message.AddInt32(workspaceAttributeName, (int32)Workspaces());

#if __HAIKU__
	BMessage decorSettings;
	if (GetDecoratorSettings(&decorSettings) == B_OK) {
		message.AddMessage(kAttrWindowDecor, &decorSettings);
	}
#endif // __HAIKU__
}


status_t
BContainerWindow::DragStart(const BMessage *incoming)
{
	if (!incoming)
		return B_ERROR;	

	//	if already dragging, or
	//	if all the refs match
	if (Dragging() && SpringLoadedFolderCompareMessages(incoming, fDragMessage))
		return B_OK;

	//	cache the current drag message
	//	build a list of the mimetypes in the message
	SpringLoadedFolderCacheDragData(incoming, &fDragMessage, &fCachedTypesList);
	
	fWaitingForRefs = true;
	
	return B_OK;
}


void
BContainerWindow::DragStop()
{
	delete fDragMessage;
	fDragMessage = NULL;

	delete fCachedTypesList;
	fCachedTypesList = NULL;
	
	fWaitingForRefs = false;
}


void
BContainerWindow::ShowSelectionWindow()
{
	if (fSelectionWindow == NULL) {
		fSelectionWindow = new SelectionWindow(this);	
		fSelectionWindow->Show();
	} else if (fSelectionWindow->Lock()) {
		if (fSelectionWindow->IsHidden()) {
			fSelectionWindow->MoveCloseToMouse();
			fSelectionWindow->Show();
		}
		fSelectionWindow->Unlock();
	}
}


void
BContainerWindow::ShowNavigator(bool show)
{
	if (PoseView()->IsDesktopWindow())
		return;
	
	if (show) {
		if (Navigator() && !Navigator()->IsHidden())
			return;

		if (Navigator() == NULL) {
			BRect rect(Bounds());
			rect.top = KeyMenuBar()->Bounds().Height() + 1;
			rect.bottom = rect.top + BNavigator::CalcNavigatorHeight();
			fNavigator = new BNavigator(TargetModel(), rect);
			AddChild(fNavigator);
		}
		
		if (Navigator()->IsHidden()) {
			if (Navigator()->Bounds().top == 0)
				Navigator()->MoveTo(0, KeyMenuBar()->Bounds().Height() + 1);
				// This is if the navigator was created with a .top = 0.
			Navigator()->Show();
		}
		
		float displacement = Navigator()->Frame().Height() + 1;
	
		PoseView()->MoveBy(0, displacement);
		PoseView()->ResizeBy(0, -displacement);
		
		if (PoseView()->VScrollBar()) {
			PoseView()->VScrollBar()->MoveBy(0, displacement);
			PoseView()->VScrollBar()->ResizeBy(0, -displacement);
			PoseView()->UpdateScrollRange();
		}		
	} else {		
		if (!Navigator() || Navigator()->IsHidden())
			return;

		float displacement = Navigator()->Frame().Height() + 1;
	
		PoseView()->ResizeBy(0, displacement);
		PoseView()->MoveBy(0, -displacement);

		if (PoseView()->VScrollBar()) {
			PoseView()->VScrollBar()->ResizeBy(0, displacement);
			PoseView()->VScrollBar()->MoveBy(0, -displacement);
			PoseView()->UpdateScrollRange();
		}

		fNavigator->Hide();
	}
}


void
BContainerWindow::SetSingleWindowBrowseShortcuts(bool enabled)
{
	if (PoseView()->IsDesktopWindow())
		return;
	
	if (enabled) {
		if (!Navigator())
			return;

		RemoveShortcut(B_DOWN_ARROW, B_OPTION_KEY | B_COMMAND_KEY);
		RemoveShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY);
		RemoveShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY | B_CONTROL_KEY);	
		RemoveShortcut(B_UP_ARROW, B_COMMAND_KEY | B_CONTROL_KEY);

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

	} else {

		RemoveShortcut(B_LEFT_ARROW, B_COMMAND_KEY);
		RemoveShortcut(B_RIGHT_ARROW, B_COMMAND_KEY);
		RemoveShortcut(B_UP_ARROW, B_COMMAND_KEY);
			// This is added again, below, with a new meaning.
					
		RemoveShortcut(B_LEFT_ARROW, B_OPTION_KEY | B_COMMAND_KEY);
		RemoveShortcut(B_RIGHT_ARROW, B_OPTION_KEY | B_COMMAND_KEY);
		RemoveShortcut(B_UP_ARROW, B_OPTION_KEY | B_COMMAND_KEY);
			// This also changes meaning, added again below.
			
		AddShortcut(B_DOWN_ARROW, B_OPTION_KEY | B_COMMAND_KEY,
			new BMessage(kOpenSelection), PoseView());
		AddShortcut(B_UP_ARROW, B_COMMAND_KEY,
			new BMessage(kOpenParentDir), PoseView());
			// We change the meaning from kNavigatorCommandUp to kOpenParentDir.
		AddShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY,
			new BMessage(kOpenParentDir), PoseView());
		AddShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY | B_CONTROL_KEY,	
			new BMessage(kOpenParentDir), PoseView());	
		AddShortcut(B_UP_ARROW, B_COMMAND_KEY | B_CONTROL_KEY,
			new BMessage(kOpenParentDir), PoseView());
			// the command option results in closing the parent window
			// the control is a secret backdoor to get at the Disks menu
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


//	#pragma mark -


WindowStateNodeOpener::WindowStateNodeOpener(BContainerWindow *window, bool forWriting)
	:	fModelOpener(NULL),
		fNode(NULL),
		fStreamNode(NULL)
{
	if (window->TargetModel() && window->TargetModel()->IsRoot()) {
		BVolume bootVol;
		BVolumeRoster().GetBootVolume(&bootVol);
		BDirectory dir;
		if (FSGetDeskDir(&dir, bootVol.Device()) == B_OK) {
			fNode = new BDirectory(dir);
			fStreamNode = new AttributeStreamFileNode(fNode);
		}
	} else if (window->TargetModel()){
		fModelOpener = new ModelNodeLazyOpener(window->TargetModel(), forWriting, false);
		if (fModelOpener->IsOpen(forWriting))
			fStreamNode = new AttributeStreamFileNode(fModelOpener->TargetModel()->Node());
	}
}

WindowStateNodeOpener::~WindowStateNodeOpener()
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;
}


void 
WindowStateNodeOpener::SetTo(const BDirectory *node)
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;
	
	fModelOpener = NULL;
	fNode = new BDirectory(*node);
	fStreamNode = new AttributeStreamFileNode(fNode);
}


void 
WindowStateNodeOpener::SetTo(const BEntry *entry, bool forWriting)
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;
	
	fModelOpener = NULL;
	fNode = new BFile(entry, (uint32)(forWriting ? O_RDWR : O_RDONLY));
	fStreamNode = new AttributeStreamFileNode(fNode);
}


void 
WindowStateNodeOpener::SetTo(Model *model, bool forWriting)
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;
	
	fNode = NULL;
	fStreamNode = NULL;
	fModelOpener = new ModelNodeLazyOpener(model, forWriting, false);
	if (fModelOpener->IsOpen(forWriting))
		fStreamNode = new AttributeStreamFileNode(fModelOpener->TargetModel()->Node());
}


AttributeStreamNode *
WindowStateNodeOpener::StreamNode() const
{
	return fStreamNode;
}


BNode *
WindowStateNodeOpener::Node() const
{
	if (!fStreamNode)
		return NULL;
	
	if (fNode)
		return fNode;

	return fModelOpener->TargetModel()->Node();
}


//	#pragma mark -


BackgroundView::BackgroundView(BRect frame)
	:	BView(frame, "", B_FOLLOW_ALL,
			B_FRAME_EVENTS | B_WILL_DRAW | B_PULSE_NEEDED)
{
}


void
BackgroundView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
BackgroundView::FrameResized(float, float)
{
	Invalidate();
}


void 
BackgroundView::PoseViewFocused(bool)
{
	Invalidate();
}


void 
BackgroundView::WindowActivated(bool)
{
	Invalidate();
}


void
BackgroundView::Draw(BRect)
{
	BContainerWindow *window = dynamic_cast<BContainerWindow *>(Window());
	if (!window)
		return;

	BRect frame(window->PoseView()->Frame());

	frame.InsetBy(-1, -1);
	frame.top -= kTitleViewHeight;
	frame.bottom += B_H_SCROLL_BAR_HEIGHT;
	frame.right += B_V_SCROLL_BAR_WIDTH;
	SetHighColor(100, 100, 100);
	StrokeRect(frame);

	// draw the pose view focus  
	if (window->IsActive() && window->PoseView()->IsFocus()) {
		frame.InsetBy(-2, -2);
		SetHighColor(keyboard_navigation_color());
		StrokeRect(frame);
	}
}


void 
BackgroundView::Pulse()
{
	BContainerWindow *window = dynamic_cast<BContainerWindow *>(Window());
	if (window) 
		window->PulseTaskLoop();
}

