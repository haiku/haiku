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
#include <Application.h>
#include <AppFileInfo.h>
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
#include <MenuItem.h>
#include <MenuBar.h>
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
#include "AttributeStream.h"
#include "AutoDeleter.h"
#include "AutoLock.h"
#include "BackgroundImage.h"
#include "Commands.h"
#include "CountView.h"
#include "DeskWindow.h"
#include "FavoritesMenu.h"
#include "FindPanel.h"
#include "FSClipboard.h"
#include "FSUndoRedo.h"
#include "FSUtils.h"
#include "IconMenuItem.h"
#include "OpenWithWindow.h"
#include "MimeTypes.h"
#include "MountMenu.h"
#include "Navigator.h"
#include "NavMenu.h"
#include "PoseView.h"
#include "QueryContainerWindow.h"
#include "SelectionWindow.h"
#include "TitleView.h"
#include "Tracker.h"
#include "TrackerSettings.h"
#include "Thread.h"
#include "TemplatesMenu.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ContainerWindow"


#ifdef _IMPEXP_BE
_IMPEXP_BE
#endif
void do_minimize_team(BRect zoomRect, team_id team, bool zoom);


// Amount you have to move the mouse before a drag starts
const float kDragSlop = 3.0f;


namespace BPrivate {

class DraggableContainerIcon : public BView {
	public:
		DraggableContainerIcon();

		virtual void MouseDown(BPoint where);
		virtual void MouseUp(BPoint);
		virtual void MouseMoved(BPoint point, uint32, const BMessage*);
		virtual void Draw(BRect updateRect);

	private:
		uint32	fDragButton;
		BPoint	fClickPoint;
		bool	fDragStarted;
};

}	// namespace BPrivate


struct AddOneAddonParams {
	BObjectList<BMenuItem>* primaryList;
	BObjectList<BMenuItem>* secondaryList;
};

struct StaggerOneParams {
	bool rectFromParent;
};


const int32 kWindowStaggerBy = 17;


BRect BContainerWindow::sNewWindRect(85, 50, 548, 280);

LockingList<AddonShortcut>* BContainerWindow::fAddonsList
	= new LockingList<struct AddonShortcut>(10, true);


namespace BPrivate {

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


int
CompareLabels(const BMenuItem* item1, const BMenuItem* item2)
{
	return strcasecmp(item1->Label(), item2->Label());
}

}	// namespace BPrivate


static int32
AddOnMenuGenerate(const entry_ref* addonRef, BMenu* menu,
		BContainerWindow* window)
{
	BEntry entry(addonRef);
	BPath path;
	status_t result = entry.InitCheck();
	if (result != B_OK)
		return result;

	result = entry.GetPath(&path);
	if (result != B_OK)
		return result;

	image_id addonImage = load_add_on(path.Path());
	if (addonImage < 0)
		return addonImage;

	void (*populateMenu)(BMessage*, BMenu*, BHandler*);
	result = get_image_symbol(addonImage, "populate_menu", 2,
		(void**)&populateMenu);
	if (result < 0) {
		PRINT(("Couldn't find populate_menu\n"));
		unload_add_on(addonImage);
		return result;
	}

	BMessage* message = window->AddOnMessage(B_TRACKER_ADDON_MESSAGE);
	message->AddRef("addon_ref", addonRef);

	// call add-on code
	(*populateMenu)(message, menu, window->PoseView());

	unload_add_on(addonImage);
	return B_OK;
}


static status_t
RunAddOnMessageThread(BMessage *message, void *)
{
	entry_ref addonRef;
	BEntry entry;
	BPath path;
	status_t result = message->FindRef("addon_ref", &addonRef);
	image_id addonImage;

	if (result != B_OK)
		goto end;

	entry = BEntry(&addonRef);
	result = entry.InitCheck();
	if (result != B_OK)
		goto end;

	result = entry.GetPath(&path);
	if (result != B_OK)
		goto end;

	addonImage = load_add_on(path.Path());
	if (addonImage < 0) {
		result = addonImage;
		goto end;
	}
	void (*messageReceived)(BMessage*);
	result = get_image_symbol(addonImage, "message_received", 2,
		(void**)&messageReceived);

	if (result < 0) {
		PRINT(("Couldn't find message_received\n"));
		unload_add_on(addonImage);
		goto end;
	}
	// call add-on code
	(*messageReceived)(message);
	unload_add_on(addonImage);
	return B_OK;

end:
	BString buffer(B_TRANSLATE("Error %error loading add-On %name."));
	buffer.ReplaceFirst("%error", strerror(result));
	buffer.ReplaceFirst("%name", addonRef.name);

	BAlert* alert = new BAlert("", buffer.String(), B_TRANSLATE("Cancel"),
		0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();

	return result;
}


static bool
AddOneAddon(const Model* model, const char* name, uint32 shortcut,
	uint32 modifiers, bool primary, void* context,
	BContainerWindow* window, BMenu* menu)
{
	AddOneAddonParams* params = (AddOneAddonParams*)context;

	BMessage* message = new BMessage(kLoadAddOn);
	message->AddRef("refs", model->EntryRef());

	ModelMenuItem* item = new ModelMenuItem(model, name, message,
		(char)shortcut, modifiers);

	const entry_ref* addonRef = model->EntryRef();
	AddOnMenuGenerate(addonRef, menu, window);

	if (primary)
		params->primaryList->AddItem(item);
	else
		params->secondaryList->AddItem(item);

	return false;
}


static int32
AddOnThread(BMessage* refsMessage, entry_ref addonRef, entry_ref directoryRef)
{
	ObjectDeleter<BMessage> _(refsMessage);

	BEntry entry(&addonRef);
	BPath path;
	status_t result = entry.InitCheck();
	if (result == B_OK)
		result = entry.GetPath(&path);

	if (result == B_OK) {
		image_id addonImage = load_add_on(path.Path());
		if (addonImage >= 0) {
			void (*processRefs)(entry_ref, BMessage*, void*);
			result = get_image_symbol(addonImage, "process_refs", 2,
				(void**)&processRefs);

			if (result >= 0) {
				// call add-on code
				(*processRefs)(directoryRef, refsMessage, NULL);

				unload_add_on(addonImage);
				return B_OK;
			} else
				PRINT(("couldn't find process_refs\n"));

			unload_add_on(addonImage);
		} else
			result = addonImage;
	}

	BString buffer(B_TRANSLATE("Error %error loading Add-On %name."));
	buffer.ReplaceFirst("%error", strerror(result));
	buffer.ReplaceFirst("%name", addonRef.name);

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

	((BRect*)castToRect)->OffsetBy(kWindowStaggerBy, kWindowStaggerBy);

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


//	#pragma mark - DraggableContainerIcon


DraggableContainerIcon::DraggableContainerIcon()
	:
	BView("DraggableContainerIcon", B_WILL_DRAW),
	fDragButton(0),
	fDragStarted(false)
{
}


void
DraggableContainerIcon::MouseDown(BPoint where)
{
	// we only like container windows
	BContainerWindow* window = dynamic_cast<BContainerWindow*>(Window());
	ThrowOnAssert(window != NULL);

	// we don't like the Trash icon (because it cannot be moved)
	if (window->IsTrash() || window->IsPrintersDir())
		return;

	uint32 buttons;
	window->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);

	if (IconCache::sIconCache->IconHitTest(where, window->TargetModel(),
			kNormalIcon, B_MINI_ICON)) {
		// The click hit the icon, initiate a drag
		fDragButton = buttons
			& (B_PRIMARY_MOUSE_BUTTON | B_SECONDARY_MOUSE_BUTTON);
		fDragStarted = false;
		fClickPoint = where;
	} else
		fDragButton = 0;

	if (!fDragButton)
		Window()->Activate(true);
}


void
DraggableContainerIcon::MouseUp(BPoint)
{
	if (!fDragStarted)
		Window()->Activate(true);

	fDragButton = 0;
	fDragStarted = false;
}


void
DraggableContainerIcon::MouseMoved(BPoint where, uint32, const BMessage*)
{
	if (fDragButton == 0 || fDragStarted
		|| (abs((int32)(where.x - fClickPoint.x)) <= kDragSlop
			&& abs((int32)(where.y - fClickPoint.y)) <= kDragSlop))
		return;

	BContainerWindow* window = static_cast<BContainerWindow*>(Window());
		// we can only get here in a BContainerWindow
	Model* model = window->TargetModel();

	// Find the required height
	BFont font;
	GetFont(&font);

	font_height fontHeight;
	font.GetHeight(&fontHeight);
	float height = ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading + 2 + Bounds().Height() + 8);

	BRect rect(0, 0, std::max(Bounds().Width(),
		font.StringWidth(model->Name()) + 4), height);
	BBitmap* dragBitmap = new BBitmap(rect, B_RGBA32, true);

	dragBitmap->Lock();
	BView* view = new BView(dragBitmap->Bounds(), "", B_FOLLOW_NONE, 0);
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

	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	textColor.alpha = 128;
		// set the level of transparency by value
	view->SetHighColor(textColor);
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
	float leftText = (view->StringWidth(nameString.String())
		- Bounds().Width()) / 2;
	view->MovePenTo(BPoint(hIconOffset - leftText + 2, Bounds().Height()
		+ (fontHeight.ascent + 2)));
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

	if ((button & B_PRIMARY_MOUSE_BUTTON) != 0) {
		// add an action specifier to the message, so that it is not copied
		message.AddInt32("be:actions", (modifiers() & B_OPTION_KEY) != 0
			? B_COPY_TARGET : B_MOVE_TARGET);
	}

	fDragStarted = true;
	fDragButton = 0;

	DragMessage(&message, dragBitmap, B_OP_ALPHA,
		BPoint(fClickPoint.x + hIconOffset, fClickPoint.y), this);
}


void
DraggableContainerIcon::Draw(BRect updateRect)
{
	BContainerWindow* window = dynamic_cast<BContainerWindow*>(Window());
	ThrowOnAssert(window != NULL);

	BRect rect(Bounds());
	rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);
	be_control_look->DrawBorder(this, rect, updateRect, base, B_PLAIN_BORDER,
		0, BControlLook::B_BOTTOM_BORDER);
	be_control_look->DrawMenuBarBackground(this, rect, updateRect, base, 0,
		BControlLook::B_ALL_BORDERS & ~BControlLook::B_LEFT_BORDER);

	// Draw the icon, straddling the border
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	float iconOffsetX = (Bounds().Width() - B_MINI_ICON) / 2;
	float iconOffsetY = (Bounds().Height() - B_MINI_ICON) / 2;
	IconCache::sIconCache->Draw(window->TargetModel(), this,
		BPoint(iconOffsetX, iconOffsetY), kNormalIcon, B_MINI_ICON, true);
}


//	#pragma mark - BContainerWindow


BContainerWindow::BContainerWindow(LockingList<BWindow>* list,
	uint32 containerWindowFlags, window_look look, window_feel feel,
	uint32 flags, uint32 workspace, bool useLayouts, bool isDeskWindow)
	:
	BWindow(InitialWindowRect(feel), "TrackerWindow", look, feel, flags,
		workspace),
	fUseLayouts(useLayouts),
	fMenuContainer(NULL),
	fPoseContainer(NULL),
	fBorderedView(NULL),
	fVScrollBarContainer(NULL),
	fCountContainer(NULL),
	fFileContextMenu(NULL),
	fWindowContextMenu(NULL),
	fDropContextMenu(NULL),
	fVolumeContextMenu(NULL),
	fTrashContextMenu(NULL),
	fDragContextMenu(NULL),
	fMoveToItem(NULL),
	fCopyToItem(NULL),
	fCreateLinkItem(NULL),
	fOpenWithItem(NULL),
	fNavigationItem(NULL),
	fMenuBar(NULL),
	fDraggableIcon(NULL),
	fNavigator(NULL),
	fPoseView(NULL),
	fWindowList(list),
	fAttrMenu(NULL),
	fWindowMenu(NULL),
	fFileMenu(NULL),
	fArrangeByMenu(NULL),
	fSelectionWindow(NULL),
	fTaskLoop(NULL),
	fIsTrash(false),
	fInTrash(false),
	fIsPrinters(false),
	fIsDesktop(isDeskWindow),
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

	if (list != NULL) {
		ASSERT(list->IsLocked());
		list->AddItem(this);
	}

	if (useLayouts) {
		SetFlags(Flags() | B_AUTO_UPDATE_SIZE_LIMITS);

		fRootLayout = new BGroupLayout(B_VERTICAL, 0);
		fRootLayout->SetInsets(0);
		SetLayout(fRootLayout);
		fRootLayout->Owner()->AdoptSystemColors();

		if (!fIsDesktop) {
			fMenuContainer = new BGroupView(B_HORIZONTAL, 0);
			fRootLayout->AddView(fMenuContainer);

			fPoseContainer = new BGridView(0.0, 0.0);
			fRootLayout->AddView(fPoseContainer);

			fBorderedView = new BorderedView;
			fPoseContainer->GridLayout()->AddView(fBorderedView, 0, 1);

			fCountContainer = new BGroupView(B_HORIZONTAL, 0);
			fPoseContainer->GridLayout()->AddView(fCountContainer, 0, 2);
		}
	}

	AddCommonFilter(new BMessageFilter(B_MOUSE_DOWN, ActivateWindowFilter));

	Run();

	// watch out for settings changes
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker != NULL && tracker->Lock()) {
		tracker->StartWatching(this, kWindowsShowFullPathChanged);
		tracker->StartWatching(this, kSingleWindowBrowseChanged);
		tracker->StartWatching(this, kShowNavigatorChanged);
		tracker->StartWatching(this, kDontMoveFilesToTrashChanged);
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
		tracker->StopWatching(this, kDontMoveFilesToTrashChanged);
		tracker->Unlock();
	}

	delete fTaskLoop;
	delete fBackgroundImage;
	delete fDragMessage;
	delete fCachedTypesList;

	if (fSelectionWindow != NULL && fSelectionWindow->Lock())
		fSelectionWindow->Quit();
}


BRect
BContainerWindow::InitialWindowRect(window_feel feel)
{
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
	// get rid of context menus
	if (fNavigationItem) {
		BMenu* menu = fNavigationItem->Menu();
		if (menu != NULL)
			menu->RemoveItem(fNavigationItem);

		delete fNavigationItem;
		fNavigationItem = NULL;
	}

	if (fOpenWithItem != NULL && fOpenWithItem->Menu() == NULL) {
		delete fOpenWithItem;
		fOpenWithItem = NULL;
	}

	if (fMoveToItem != NULL && fMoveToItem->Menu() == NULL) {
		delete fMoveToItem;
		fMoveToItem = NULL;
	}

	if (fCopyToItem != NULL && fCopyToItem->Menu() == NULL) {
		delete fCopyToItem;
		fCopyToItem = NULL;
	}

	if (fCreateLinkItem != NULL && fCreateLinkItem->Menu() == NULL) {
		delete fCreateLinkItem;
		fCreateLinkItem = NULL;
	}

	if (fAttrMenu != NULL && fAttrMenu->Supermenu() == NULL) {
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
BContainerWindow::UpdateIfTrash(Model* model)
{
	BEntry entry(model->EntryRef());

	if (entry.InitCheck() == B_OK) {
		fIsTrash = model->IsTrash();
		fInTrash = FSInTrashDir(model->EntryRef());
		fIsPrinters = FSIsPrintersDir(&entry);
	}
}


void
BContainerWindow::CreatePoseView(Model* model)
{
	UpdateIfTrash(model);

	fPoseView = NewPoseView(model, kListMode);
	fBorderedView->GroupLayout()->AddView(fPoseView);
	fBorderedView->GroupLayout()->SetInsets(1, 0, 1, 1);
	fBorderedView->EnableBorderHighlight(false);

	TrackerSettings settings;
	if (settings.SingleWindowBrowse() && model->IsDirectory()
		&& !fPoseView->IsFilePanel()) {
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

	fTrashContextMenu = new BPopUpMenu("TrashContext", false, false);
	fTrashContextMenu->SetFont(be_plain_font);
	AddTrashContextMenus(fTrashContextMenu);
}


void
BContainerWindow::RepopulateMenus()
{
	// Avoid these menus to be destroyed:
	if (fMoveToItem != NULL && fMoveToItem->Menu() != NULL)
		fMoveToItem->Menu()->RemoveItem(fMoveToItem);

	if (fCopyToItem != NULL && fCopyToItem->Menu() != NULL)
		fCopyToItem->Menu()->RemoveItem(fCopyToItem);

	if (fCreateLinkItem != NULL && fCreateLinkItem->Menu() != NULL)
		fCreateLinkItem->Menu()->RemoveItem(fCreateLinkItem);

	if (fOpenWithItem != NULL && fOpenWithItem->Menu() != NULL) {
		fOpenWithItem->Menu()->RemoveItem(fOpenWithItem);
		delete fOpenWithItem;
		fOpenWithItem = NULL;
	}

	if (fNavigationItem != NULL) {
		BMenu* menu = fNavigationItem->Menu();
		if (menu != NULL) {
			menu->RemoveItem(fNavigationItem);
			BMenuItem* item = menu->RemoveItem((int32)0);
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

	if (fMenuBar != NULL) {
		fMenuBar->RemoveItem(fFileMenu);
		delete fFileMenu;
		fFileMenu = new BMenu(B_TRANSLATE("File"));
		AddFileMenu(fFileMenu);
		fMenuBar->AddItem(fFileMenu);

		fMenuBar->RemoveItem(fWindowMenu);
		delete fWindowMenu;
		fWindowMenu = new BMenu(B_TRANSLATE("Window"));
		fMenuBar->AddItem(fWindowMenu);
		AddWindowMenu(fWindowMenu);

		// just create the attribute, decide to add it later
		fMenuBar->RemoveItem(fAttrMenu);
		delete fAttrMenu;
		fAttrMenu = new BMenu(B_TRANSLATE("Attributes"));
		NewAttributeMenu(fAttrMenu);
		if (PoseView()->ViewMode() == kListMode)
			ShowAttributeMenu();

		PopulateArrangeByMenu(fArrangeByMenu);

		int32 selectCount = PoseView()->SelectionList()->CountItems();

		SetupOpenWithMenu(fFileMenu);
		SetupMoveCopyMenus(selectCount ? PoseView()->SelectionList()
				->FirstItem()->TargetModel()->EntryRef() : NULL,
			fFileMenu);
	}
}


void
BContainerWindow::Init(const BMessage* message)
{
	BEntry entry;

	ASSERT(fPoseView != NULL);
	if (fPoseView == NULL)
		return;

	// deal with new unconfigured folders
	if (NeedsDefaultStateSetup())
		SetUpDefaultState();

	if (ShouldAddScrollBars())
		fPoseView->AddScrollBars();

	fMoveToItem = new BMenuItem(new BNavMenu(B_TRANSLATE("Move to"),
		kMoveSelectionTo, this));
	fCopyToItem = new BMenuItem(new BNavMenu(B_TRANSLATE("Copy to"),
		kCopySelectionTo, this));
	fCreateLinkItem = new BMenuItem(new BNavMenu(B_TRANSLATE("Create link"),
		kCreateLink, this), new BMessage(kCreateLink));

	TrackerSettings settings;

	if (ShouldAddMenus()) {
		fMenuBar = new BMenuBar("MenuBar");
		fMenuContainer->GroupLayout()->AddView(fMenuBar);
		AddMenus();

		if (!TargetModel()->IsRoot() && !IsTrash())
			_AddFolderIcon();
	} else {
		// add equivalents of the menu shortcuts to the menuless
		// desktop window
		AddShortcuts();
	}

	AddContextMenus();
	AddShortcut('T', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kDelete),
		PoseView());
	AddShortcut('K', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kCleanupAll),
		PoseView());
	AddShortcut('Q', B_COMMAND_KEY | B_OPTION_KEY | B_SHIFT_KEY
		| B_CONTROL_KEY, new BMessage(kQuitTracker));

	AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY, new BMessage(kOpenSelection),
		PoseView());

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
		BObjectList<const char> unmodified(3, true);
		if (keymap.GetModifiedCharacters("+", B_SHIFT_KEY, 0, &unmodified)
				== B_OK) {
			int32 count = unmodified.CountItems();
			for (int32 i = 0; i < count; i++) {
				uint32 key = BUnicodeChar::FromUTF8(unmodified.ItemAt(i));
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

	if (ShouldAddMenus() && PoseView()->ViewMode() == kListMode) {
		// for now only show attributes in list view
		// eventually enable attribute menu to allow users to select
		// using different attributes as titles in icon view modes
		ShowAttributeMenu();
	}
	MarkAttributeMenu(fAttrMenu);
	CheckScreenIntersect();

	if (fBackgroundImage != NULL && !fIsDesktop
		&& PoseView()->ViewMode() != kListMode) {
		fBackgroundImage->Show(PoseView(), current_workspace());
	}

	Show();

	// done showing, turn the B_NO_WORKSPACE_ACTIVATION flag off;
	// it was on to prevent workspace jerking during boot
	SetFlags(Flags() & ~B_NO_WORKSPACE_ACTIVATION);
}

void
BContainerWindow::InitLayout()
{
	fBorderedView->GroupLayout()->AddView(0, fPoseView->TitleView());

	fCountContainer->GroupLayout()->AddView(fPoseView->CountView(), 0.25f);

	bool forFilePanel = PoseView()->IsFilePanel();
	if (!forFilePanel) {
		// Eliminate the extra borders
		fPoseContainer->GridLayout()->SetInsets(-1, 0, -1, -1);
		fCountContainer->GroupLayout()->SetInsets(0, -1, 0, 0);
	}

	if (fPoseView->VScrollBar() != NULL) {
		fVScrollBarContainer = new BGroupView(B_VERTICAL, 0);
		fVScrollBarContainer->GroupLayout()->AddView(fPoseView->VScrollBar());
		fVScrollBarContainer->GroupLayout()->SetInsets(-1, forFilePanel ? 0 : -1,
			0, 0);
		fPoseContainer->GridLayout()->AddView(fVScrollBarContainer, 1, 1);
	}
	if (fPoseView->HScrollBar() != NULL) {
		BGroupView* hScrollBarContainer = new BGroupView(B_VERTICAL, 0);
		hScrollBarContainer->GroupLayout()->AddView(fPoseView->HScrollBar());
		hScrollBarContainer->GroupLayout()->SetInsets(0, -1, 0,
			forFilePanel ? 0 : -1);
		fCountContainer->GroupLayout()->AddView(hScrollBarContainer);

		BSize size = fPoseView->HScrollBar()->MinSize();
		if (forFilePanel) {
			// Count view height is 1px smaller than scroll bar because it has
			// no upper border.
			size.height -= 1;
		}
		fPoseView->CountView()->SetExplicitMinSize(size);
	}
}

void
BContainerWindow::RestoreState()
{
	UpdateTitle();

	WindowStateNodeOpener opener(this, false);
	RestoreWindowState(opener.StreamNode());
	fPoseView->Init(opener.StreamNode());

	RestoreStateCommon();
}


void
BContainerWindow::RestoreState(const BMessage &message)
{
	UpdateTitle();

	RestoreWindowState(message);
	fPoseView->Init(message);

	RestoreStateCommon();
}


void
BContainerWindow::RestoreStateCommon()
{
	if (!fIsDesktop && fUseLayouts)
		InitLayout();

	if (BootedInSafeMode())
		// don't pick up backgrounds in safe mode
		return;

	WindowStateNodeOpener opener(this, false);

	if (!TargetModel()->IsRoot() && opener.Node() != NULL) {
		// don't pick up background image for root disks
		// to do this, would have to have a unique attribute for the
		// disks window that doesn't collide with the desktop
		// for R4 this was not done to make things simpler
		// the default image will still work though
		fBackgroundImage = BackgroundImage::GetBackgroundImage(
			opener.Node(), fIsDesktop);
			// look for background image info in the window's node
	}

	BNode defaultingNode;
	if (fBackgroundImage == NULL && !fIsDesktop
		&& DefaultStateSourceNode(kDefaultFolderTemplate, &defaultingNode)) {
		// look for background image info in the source for defaults
		fBackgroundImage = BackgroundImage::GetBackgroundImage(&defaultingNode,
			fIsDesktop);
	}
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

	if (Navigator() != NULL) {
		Navigator()->UpdateLocation(PoseView()->TargetModel(),
			kActionUpdatePath);
	}
}


void
BContainerWindow::UpdateBackgroundImage()
{
	if (BootedInSafeMode())
		return;

	WindowStateNodeOpener opener(this, false);

	if (!TargetModel()->IsRoot() && opener.Node() != NULL) {
		fBackgroundImage = BackgroundImage::Refresh(fBackgroundImage,
			opener.Node(), fIsDesktop, PoseView());
	}

		// look for background image info in the window's node
	BNode defaultingNode;
	if (!fBackgroundImage && !fIsDesktop
		&& DefaultStateSourceNode(kDefaultFolderTemplate, &defaultingNode)) {
		// look for background image info in the source for defaults
		fBackgroundImage = BackgroundImage::Refresh(fBackgroundImage,
			&defaultingNode, fIsDesktop, PoseView());
	}
}


void
BContainerWindow::FrameResized(float, float)
{
	if (PoseView() != NULL && !fIsDesktop) {
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
			fPoseView->SaveState(opener.StreamNode());

		fStateNeedsSaving = false;
	}
}


void
BContainerWindow::SaveState(BMessage& message) const
{
	if (SaveStateIsEnabled()) {
		SaveWindowState(message);
		fPoseView->SaveState(message);
	}
}


bool
BContainerWindow::StateNeedsSaving() const
{
	return fPoseView != NULL && (fStateNeedsSaving || fPoseView->StateNeedsSaving());
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
		case kCutMoreSelectionToClipboard:
		case kCopyMoreSelectionToClipboard:
		case kPasteLinksFromClipboard:
		{
			BView* view = CurrentFocus();
			if (dynamic_cast<BTextView*>(view) == NULL) {
				// The selected item is not a BTextView, so forward the
				// message to the PoseView.
				if (fPoseView != NULL)
					fPoseView->MessageReceived(message);
			} else {
				// Since we catch the generic clipboard shortcuts in a way that
				// means the BTextView will never get them, we must
				// manually forward them ourselves.
				//
				// However, we have to take care to not forward the custom
				// clipboard messages, else we would wind up in infinite
				// recursion.
				if (message->what == B_CUT || message->what == B_COPY
						|| message->what == B_PASTE) {
					view->MessageReceived(message);
				}
			}
			break;
		}

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

		case kNewFolder:
			PostMessage(message, PoseView());
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
			} else if (!TargetModel()->IsQuery()
				&& !TargetModel()->IsVirtualDirectory()) {
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

					if (Navigator() != NULL) {
						// update Navigation bar
						int32 action = kActionSet;
						if (message->FindInt32("action", &action) != B_OK) {
							// Design problem? Why does FindInt32 touch
							// 'action' at all if he can't find it??
							action = kActionSet;
						}
						Navigator()->UpdateLocation(PoseView()->TargetModel(),
							action);
					}

					TrackerSettings settings;
					if (settings.ShowNavigator()
						|| settings.ShowFullPathInTitleBar()) {
						SetPathWatchingEnabled(true);
					}
					SetSingleWindowBrowseShortcuts(
						settings.SingleWindowBrowse());

					// Update draggable folder icon
					if (fMenuBar != NULL) {
						if (!TargetModel()->IsRoot() && !IsTrash()) {
							// Folder icon should be visible, but in single
							// window navigation, it might not be.
							if (fDraggableIcon != NULL) {
								IconCache::sIconCache->IconChanged(
									TargetModel());
								fDraggableIcon->Invalidate();
							} else
								_AddFolderIcon();
						} else if (fDraggableIcon != NULL)
							fDraggableIcon->RemoveSelf();
					}

					// Update window title
					UpdateTitle();
				}
			}
			break;
		}

		case B_REFS_RECEIVED:
			if (Dragging()) {
				// ref in this message is the target,
				// the end point of the drag

				entry_ref ref;
				if (message->FindRef("refs", &ref) == B_OK) {
					fWaitingForRefs = false;
					BEntry entry(&ref, true);
					// don't copy to printers dir
					if (!FSIsPrintersDir(&entry)) {
						if (entry.InitCheck() == B_OK
							&& entry.IsDirectory()) {
							Model targetModel(&entry, true, false);
							BPoint dropPoint;
							uint32 buttons;
							PoseView()->GetMouse(&dropPoint, &buttons, true);
							PoseView()->HandleDropCommon(fDragMessage,
								&targetModel, NULL, PoseView(), dropPoint);
						}
					}
				}
				DragStop();
			}
			break;

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
							&& !(settings.ShowNavigator()
								|| settings.ShowFullPathInTitleBar())) {
							SetPathWatchingEnabled(false);
						}
						break;

					case kSingleWindowBrowseChanged:
						if (settings.SingleWindowBrowse()
							&& !Navigator()
							&& TargetModel()->IsDirectory()
							&& !PoseView()->IsFilePanel()
							&& !PoseView()->IsDesktopWindow()) {
							fNavigator = new BNavigator(TargetModel());
							fPoseContainer->GridLayout()->AddView(fNavigator,
								0, 0, 2);
							fNavigator->Hide();
							SetPathWatchingEnabled(settings.ShowNavigator()
								|| settings.ShowFullPathInTitleBar());
						}

						if (!settings.SingleWindowBrowse()
							&& !fIsDesktop && TargetModel()->IsDesktop()) {
							// Close the "Desktop" window, but not the Desktop
							this->Quit();
						}

						SetSingleWindowBrowseShortcuts(
							settings.SingleWindowBrowse());
						break;

					case kShowNavigatorChanged:
						ShowNavigator(settings.ShowNavigator());
						if (!IsPathWatchingEnabled()
							&& settings.ShowNavigator()) {
							SetPathWatchingEnabled(true);
						}
						if (IsPathWatchingEnabled()
							&& !(settings.ShowNavigator()
								|| settings.ShowFullPathInTitleBar())) {
							SetPathWatchingEnabled(false);
						}
						SetSingleWindowBrowseShortcuts(
							settings.SingleWindowBrowse());
						break;

					case kDontMoveFilesToTrashChanged:
					{
						bool dontMoveToTrash
							= settings.DontMoveFilesToTrash();

						BMenuItem* item
							= fFileContextMenu->FindItem(kMoveToTrash);
						if (item != NULL) {
							item->SetLabel(dontMoveToTrash
								? B_TRANSLATE("Delete")
								: B_TRANSLATE("Move to Trash"));
						}
						// Deskbar doesn't have a menu bar, so check if
						// there is fMenuBar
						if (fMenuBar != NULL && fFileMenu != NULL) {
							item = fFileMenu->FindItem(kMoveToTrash);
							if (item != NULL) {
								item->SetLabel(dontMoveToTrash
									? B_TRANSLATE("Delete")
									: B_TRANSLATE("Move to Trash"));
							}
						}
						UpdateIfNeeded();
						break;
					}

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


void
BContainerWindow::SetCutItem(BMenu* menu)
{
	BMenuItem* item;
	if ((item = menu->FindItem(B_CUT)) == NULL
		&& (item = menu->FindItem(kCutMoreSelectionToClipboard)) == NULL)
		return;

	item->SetEnabled(PoseView()->SelectionList()->CountItems() > 0
		|| PoseView() != CurrentFocus());

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Cut more"));
		item->SetShortcut('X', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kCutMoreSelectionToClipboard));
	} else {
		item->SetLabel(B_TRANSLATE("Cut"));
		item->SetShortcut('X', B_COMMAND_KEY);
		item->SetMessage(new BMessage(B_CUT));
	}
}


void
BContainerWindow::SetCopyItem(BMenu* menu)
{
	BMenuItem* item;
	if ((item = menu->FindItem(B_COPY)) == NULL
		&& (item = menu->FindItem(kCopyMoreSelectionToClipboard)) == NULL) {
		return;
	}

	item->SetEnabled(PoseView()->SelectionList()->CountItems() > 0
		|| PoseView() != CurrentFocus());

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Copy more"));
		item->SetShortcut('C', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kCopyMoreSelectionToClipboard));
	} else {
		item->SetLabel(B_TRANSLATE("Copy"));
		item->SetShortcut('C', B_COMMAND_KEY);
		item->SetMessage(new BMessage(B_COPY));
	}
}


void
BContainerWindow::SetPasteItem(BMenu* menu)
{
	BMenuItem* item;
	if ((item = menu->FindItem(B_PASTE)) == NULL
		&& (item = menu->FindItem(kPasteLinksFromClipboard)) == NULL) {
		return;
	}

	item->SetEnabled(FSClipboardHasRefs() || PoseView() != CurrentFocus());

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Paste links"));
		item->SetShortcut('V', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kPasteLinksFromClipboard));
	} else {
		item->SetLabel(B_TRANSLATE("Paste"));
		item->SetShortcut('V', B_COMMAND_KEY);
		item->SetMessage(new BMessage(B_PASTE));
	}
}


void
BContainerWindow::SetArrangeMenu(BMenu* menu)
{
	BMenuItem* item;
	if ((item = menu->FindItem(kCleanup)) == NULL
		&& (item = menu->FindItem(kCleanupAll)) == NULL) {
		return;
	}

	item->Menu()->SetEnabled(PoseView()->CountItems() > 0
		&& (PoseView()->ViewMode() != kListMode));

	BMenu* arrangeMenu;

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Clean up all"));
		item->SetShortcut('K', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kCleanupAll));
		arrangeMenu = item->Menu();
	} else {
		item->SetLabel(B_TRANSLATE("Clean up"));
		item->SetShortcut('K', B_COMMAND_KEY);
		item->SetMessage(new BMessage(kCleanup));
		arrangeMenu = item->Menu();
	}

	MarkArrangeByMenu(arrangeMenu);
}


void
BContainerWindow::SetCloseItem(BMenu* menu)
{
	BMenuItem* item;
	if ((item = menu->FindItem(B_QUIT_REQUESTED)) == NULL
		&& (item = menu->FindItem(kCloseAllWindows)) == NULL) {
		return;
	}

	if ((modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Close all"));
		item->SetShortcut('W', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetTarget(be_app);
		item->SetMessage(new BMessage(kCloseAllWindows));
	} else {
		item->SetLabel(B_TRANSLATE("Close"));
		item->SetShortcut('W', B_COMMAND_KEY);
		item->SetTarget(this);
		item->SetMessage(new BMessage(B_QUIT_REQUESTED));
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
	fFileMenu = new BMenu(B_TRANSLATE("File"));
	AddFileMenu(fFileMenu);
	fMenuBar->AddItem(fFileMenu);
	fWindowMenu = new BMenu(B_TRANSLATE("Window"));
	fMenuBar->AddItem(fWindowMenu);
	AddWindowMenu(fWindowMenu);
	// just create the attribute, decide to add it later
	fAttrMenu = new BMenu(B_TRANSLATE("Attributes"));
	NewAttributeMenu(fAttrMenu);
	PopulateArrangeByMenu(fArrangeByMenu);
}


void
BContainerWindow::AddFileMenu(BMenu* menu)
{
	if (!PoseView()->IsFilePanel()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Find" B_UTF8_ELLIPSIS),
			new BMessage(kFindButton), 'F'));
	}

	if (!TargetModel()->IsQuery() && !TargetModel()->IsVirtualDirectory()
		&& !IsTrash() && !IsPrintersDir() && !TargetModel()->IsRoot()) {
		if (!PoseView()->IsFilePanel()) {
			TemplatesMenu* templatesMenu = new TemplatesMenu(PoseView(),
				B_TRANSLATE("New"));
			menu->AddItem(templatesMenu);
			templatesMenu->SetTargetForItems(PoseView());
		} else {
			menu->AddItem(new BMenuItem(B_TRANSLATE("New folder"),
				new BMessage(kNewFolder), 'N'));
		}
	}
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"),
		new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
		new BMessage(kEditItem), 'E'));

	if (IsTrash() || InTrash()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Restore"),
			new BMessage(kRestoreFromTrash)));
		if (IsTrash()) {
			// add as first item in menu
			menu->AddItem(new BMenuItem(B_TRANSLATE("Empty Trash"),
				new BMessage(kEmptyTrash)), 0);
			menu->AddItem(new BSeparatorItem(), 1);
		}
	} else if (IsPrintersDir()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Add printer" B_UTF8_ELLIPSIS),
			new BMessage(kAddPrinter), 'N'), 0);
		menu->AddItem(new BSeparatorItem(), 1);
		menu->AddItem(new BMenuItem(B_TRANSLATE("Make active printer"),
			new BMessage(kMakeActivePrinter)));
	} else if (TargetModel()->IsRoot()) {
		BMenuItem* item = new BMenuItem(B_TRANSLATE("Unmount"),
			new BMessage(kUnmountVolume), 'U');
		item->SetEnabled(false);
		menu->AddItem(item);
		menu->AddItem(new BMenuItem(
			B_TRANSLATE("Mount settings" B_UTF8_ELLIPSIS),
			new BMessage(kRunAutomounterSettings)));
	} else {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Duplicate"),
			new BMessage(kDuplicateSelection), 'D'));
		menu->AddItem(new BMenuItem(TrackerSettings().DontMoveFilesToTrash()
			? B_TRANSLATE("Delete") : B_TRANSLATE("Move to Trash"),
			new BMessage(kMoveToTrash), 'T'));
		menu->AddSeparatorItem();

		// The "Move To", "Copy To", "Create Link" menus are inserted
		// at this place, have a look at:
		// BContainerWindow::SetupMoveCopyMenus()
	}

	BMenuItem* cutItem = NULL;
	BMenuItem* copyItem = NULL;
	BMenuItem* pasteItem = NULL;
	if (!IsPrintersDir()) {
		menu->AddSeparatorItem();

		if (!TargetModel()->IsRoot()) {
			cutItem = new(std::nothrow) BMenuItem(B_TRANSLATE("Cut"),
				new BMessage(B_CUT), 'X');
			menu->AddItem(cutItem);
			copyItem = new(std::nothrow) BMenuItem(B_TRANSLATE("Copy"),
				new BMessage(B_COPY), 'C');
			menu->AddItem(copyItem);
			pasteItem = new(std::nothrow) BMenuItem(B_TRANSLATE("Paste"),
				new BMessage(B_PASTE), 'V');
			menu->AddItem(pasteItem);
			menu->AddSeparatorItem();
			menu->AddItem(new BMenuItem(B_TRANSLATE("Identify"),
				new BMessage(kIdentifyEntry)));
		}
		BMenu* addOnMenuItem = new BMenu(B_TRANSLATE("Add-ons"));
		addOnMenuItem->SetFont(be_plain_font);
		menu->AddItem(addOnMenuItem);
	}

	menu->SetTargetForItems(PoseView());
	if (cutItem != NULL)
		cutItem->SetTarget(this);

	if (copyItem != NULL)
		copyItem->SetTarget(this);

	if (pasteItem != NULL)
		pasteItem->SetTarget(this);
}


void
BContainerWindow::AddWindowMenu(BMenu* menu)
{
	BMenuItem* item;

	BMenu* iconSizeMenu = new BMenu(B_TRANSLATE("Icon view"));

	BMessage* message = new BMessage(kIconMode);
	message->AddInt32("size", 32);
	item = new BMenuItem(B_TRANSLATE("32 x 32"), message);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 40);
	item = new BMenuItem(B_TRANSLATE("40 x 40"), message);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 48);
	item = new BMenuItem(B_TRANSLATE("48 x 48"), message);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 64);
	item = new BMenuItem(B_TRANSLATE("64 x 64"), message);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 96);
	item = new BMenuItem(B_TRANSLATE("96 x 96"), message);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 128);
	item = new BMenuItem(B_TRANSLATE("128 x 128"), message);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

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
	iconSizeMenu->Superitem()->SetShortcut('1', B_COMMAND_KEY);
	iconSizeMenu->Superitem()->SetMessage(new BMessage(kIconMode));
	iconSizeMenu->Superitem()->SetTarget(PoseView());

	item = new BMenuItem(B_TRANSLATE("Mini icon view"),
		new BMessage(kMiniIconMode), '2');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("List view"),
		new BMessage(kListMode), '3');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Resize to fit"),
		new BMessage(kResizeToFit), 'Y');
	item->SetTarget(this);
	menu->AddItem(item);

	fArrangeByMenu = new BMenu(B_TRANSLATE("Arrange by"));
	menu->AddItem(fArrangeByMenu);

	item = new BMenuItem(B_TRANSLATE("Select" B_UTF8_ELLIPSIS),
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY);
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Invert selection"),
		new BMessage(kInvertSelection), 'S');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	if (!IsTrash()) {
		item = new BMenuItem(B_TRANSLATE("Open parent"),
			new BMessage(kOpenParentDir), B_UP_ARROW);
		item->SetTarget(PoseView());
		menu->AddItem(item);
	}

	item = new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(B_QUIT_REQUESTED), 'W');
	item->SetTarget(this);
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Close all in workspace"),
		new BMessage(kCloseAllInWorkspace), 'Q');
	item->SetTarget(be_app);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Preferences" B_UTF8_ELLIPSIS),
		new BMessage(kShowSettingsWindow), ',');
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
	ASSERT(!TargetModel()->IsVirtualDirectory());

	AddShortcut('X', B_COMMAND_KEY | B_SHIFT_KEY,
		new BMessage(kCutMoreSelectionToClipboard), this);
	AddShortcut('C', B_COMMAND_KEY | B_SHIFT_KEY,
		new BMessage(kCopyMoreSelectionToClipboard), this);
	AddShortcut('F', B_COMMAND_KEY,
		new BMessage(kFindButton), PoseView());
	AddShortcut('N', B_COMMAND_KEY,
		new BMessage(kNewFolder), PoseView());
	AddShortcut('O', B_COMMAND_KEY,
		new BMessage(kOpenSelection), PoseView());
	AddShortcut('I', B_COMMAND_KEY,
		new BMessage(kGetInfo), PoseView());
	AddShortcut('E', B_COMMAND_KEY,
		new BMessage(kEditItem), PoseView());
	AddShortcut('D', B_COMMAND_KEY,
		new BMessage(kDuplicateSelection), PoseView());
	AddShortcut('T', B_COMMAND_KEY,
		new BMessage(kMoveToTrash), PoseView());
	AddShortcut('K', B_COMMAND_KEY,
		new BMessage(kCleanup), PoseView());
	AddShortcut('A', B_COMMAND_KEY,
		new BMessage(B_SELECT_ALL), PoseView());
	AddShortcut('S', B_COMMAND_KEY,
		new BMessage(kInvertSelection), PoseView());
	AddShortcut('A', B_COMMAND_KEY | B_SHIFT_KEY,
		new BMessage(kShowSelectionWindow), PoseView());
	AddShortcut('G', B_COMMAND_KEY,
		new BMessage(kEditQuery), PoseView());
		// it is ok to add a global Edit query shortcut here, PoseView will
		// filter out cases where selected pose is not a query
	AddShortcut('U', B_COMMAND_KEY,
		new BMessage(kUnmountVolume), PoseView());
	AddShortcut(B_UP_ARROW, B_COMMAND_KEY,
		new BMessage(kOpenParentDir), PoseView());
	AddShortcut('O', B_COMMAND_KEY | B_CONTROL_KEY,
		new BMessage(kOpenSelectionWith), PoseView());

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
		fPoseView->CommitActivePose();
	}

	// File menu
	int32 selectCount = PoseView()->SelectionList()->CountItems();

	SetupOpenWithMenu(fFileMenu);
	SetupMoveCopyMenus(selectCount
		? PoseView()->SelectionList()->FirstItem()->TargetModel()->EntryRef()
		: NULL, fFileMenu);

	if (TargetModel()->IsRoot()) {
		BVolume boot;
		BVolumeRoster().GetBootVolume(&boot);

		bool ejectableVolumeSelected = false;

		int32 count = PoseView()->SelectionList()->CountItems();
		for (int32 index = 0; index < count; index++) {
			Model* model
				= PoseView()->SelectionList()->ItemAt(index)->TargetModel();
			if (model->IsVolume()) {
				BVolume volume;
				volume.SetTo(model->NodeRef()->device);
				if (volume != boot) {
					ejectableVolumeSelected = true;
					break;
				}
			}
		}
		BMenuItem* item = fMenuBar->FindItem(kUnmountVolume);
		if (item != NULL)
			item->SetEnabled(ejectableVolumeSelected);
	}

	UpdateMenu(fMenuBar, kMenuBarContext);

	AddMimeTypesToMenu(fAttrMenu);

	if (IsPrintersDir()) {
		EnableNamedMenuItem(fFileMenu, B_TRANSLATE("Make active printer"),
			selectCount == 1);
	}
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
BContainerWindow::SetupNavigationMenu(const entry_ref* ref, BMenu* parent)
{
	// start by removing nav item (and separator) from old menu
	if (fNavigationItem != NULL) {
		BMenu* menu = fNavigationItem->Menu();
		if (menu != NULL) {
			menu->RemoveItem(fNavigationItem);
			BMenuItem* item = menu->RemoveItem((int32)0);
			ASSERT(item != fNavigationItem);
			delete item;
		}
	}

	// if we weren't passed a ref then we're navigating this window
	if (ref == NULL)
		ref = TargetModel()->EntryRef();

	BEntry entry;
	if (entry.SetTo(ref) != B_OK)
		return;

	// only navigate directories and queries (check for symlink here)
	Model model(&entry);
	entry_ref resolvedRef;

	if (model.InitCheck() != B_OK
		|| (!model.IsContainer() && !model.IsSymLink())) {
		return;
	}

	if (model.IsSymLink()) {
		if (entry.SetTo(model.EntryRef(), true) != B_OK)
			return;

		Model resolvedModel(&entry);
		if (resolvedModel.InitCheck() != B_OK || !resolvedModel.IsContainer())
			return;

		entry.GetRef(&resolvedRef);
		ref = &resolvedRef;
	}

	if (fNavigationItem == NULL) {
		fNavigationItem = new ModelMenuItem(&model,
			new BNavMenu(model.Name(), B_REFS_RECEIVED, be_app, this));
	}

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

	if (!Dragging())
		parent->SetTrackingHook(NULL, NULL);
}


void
BContainerWindow::SetUpEditQueryItem(BMenu* menu)
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
			BPose* pose = PoseView()->SelectionList()->ItemAt(index);
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

	BMenuItem* item = menu->FindItem(kEditQuery);
	if (!poseViewIsQuery && !queryInSelection && item != NULL)
		item->Menu()->RemoveItem(item);
	else if ((poseViewIsQuery || queryInSelection) && item == NULL) {
		// add edit query item after Open
		item = menu->FindItem(kOpenSelection);
		if (item) {
			int32 itemIndex = item->Menu()->IndexOf(item);
			BMenuItem* query = new BMenuItem(B_TRANSLATE("Edit query"),
				new BMessage(kEditQuery), 'G');
			item->Menu()->AddItem(query, itemIndex + 1);
			query->SetTarget(PoseView());
		}
	}
}


void
BContainerWindow::SetupOpenWithMenu(BMenu* parent)
{
	// start by removing nav item (and separator) from old menu
	if (fOpenWithItem) {
		BMenu* menu = fOpenWithItem->Menu();
		if (menu != NULL)
			menu->RemoveItem(fOpenWithItem);

		delete fOpenWithItem;
		fOpenWithItem = 0;
	}

	if (PoseView()->SelectionList()->CountItems() == 0) {
		// no selection, nothing to open
		return;
	}

	if (TargetModel()->IsRoot()) {
		// don't add ourselves if we are root
		return;
	}

	// ToDo:
	// check if only item in selection list is the root
	// and do not add if true

	// add after "Open"
	BMenuItem* item = parent->FindItem(kOpenSelection);

	int32 count = PoseView()->SelectionList()->CountItems();
	if (count == 0)
		return;

	// build a list of all refs to open
	BMessage message(B_REFS_RECEIVED);
	for (int32 index = 0; index < count; index++) {
		BPose* pose = PoseView()->SelectionList()->ItemAt(index);
		message.AddRef("refs", pose->TargetModel()->EntryRef());
	}

	// add Tracker token so that refs received recipients can script us
	message.AddMessenger("TrackerViewToken", BMessenger(PoseView()));

	int32 index = item->Menu()->IndexOf(item);
	fOpenWithItem = new BMenuItem(
		new OpenWithMenu(B_TRANSLATE("Open with" B_UTF8_ELLIPSIS),
			&message, this, be_app), new BMessage(kOpenSelectionWith));
	fOpenWithItem->SetTarget(PoseView());
	fOpenWithItem->SetShortcut('O', B_COMMAND_KEY | B_CONTROL_KEY);

	item->Menu()->AddItem(fOpenWithItem, index + 1);
}


void
BContainerWindow::PopulateMoveCopyNavMenu(BNavMenu* navMenu, uint32 what,
	const entry_ref* ref, bool addLocalOnly)
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

		BMenuItem* item = new SpecialModelMenuItem(&model,menu);
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

			BMenuItem* item = new SpecialModelMenuItem(&model,menu);
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
BContainerWindow::SetupMoveCopyMenus(const entry_ref* item_ref, BMenu* parent)
{
	if (IsTrash() || InTrash() || IsPrintersDir() || fMoveToItem == NULL
		|| fCopyToItem == NULL || fCreateLinkItem == NULL
		|| TargetModel()->IsRoot()) {
		return;
	}

	// re-parent items to this menu since they're shared
	BMenuItem* trash = parent->FindItem(kMoveToTrash);
	int32 index = trash != NULL ? parent->IndexOf(trash) + 2 : 0;

	if (fMoveToItem->Menu() != parent) {
		if (fMoveToItem->Menu() != NULL)
			fMoveToItem->Menu()->RemoveItem(fMoveToItem);

		parent->AddItem(fMoveToItem, index++);
	}

	if (fCopyToItem->Menu() != parent) {
		if (fCopyToItem->Menu() != NULL)
			fCopyToItem->Menu()->RemoveItem(fCopyToItem);

		parent->AddItem(fCopyToItem, index++);
	}

	if (fCreateLinkItem->Menu() != parent) {
		if (fCreateLinkItem->Menu() != NULL)
			fCreateLinkItem->Menu()->RemoveItem(fCreateLinkItem);

		parent->AddItem(fCreateLinkItem, index);
	}

	// Set the "Create Link" item label here so it
	// appears correctly when menus are disabled, too.
	if ((modifiers() & B_SHIFT_KEY) != 0)
		fCreateLinkItem->SetLabel(B_TRANSLATE("Create relative link"));
	else
		fCreateLinkItem->SetLabel(B_TRANSLATE("Create link"));

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
	PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu*>(fMoveToItem->Submenu()),
		kMoveSelectionTo, item_ref, true);

	// configure "Copy to" menu item
	// add all mounted volumes (except the one this item lives on)
	PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu*>(fCopyToItem->Submenu()),
		kCopySelectionTo, item_ref, false);

	// Set "Create Link" menu item message and
	// add all mounted volumes (except the one this item lives on)
	if ((modifiers() & B_SHIFT_KEY) != 0) {
		fCreateLinkItem->SetMessage(new BMessage(kCreateRelativeLink));
		PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu*>
				(fCreateLinkItem->Submenu()),
			kCreateRelativeLink, item_ref, false);
	} else {
		fCreateLinkItem->SetMessage(new BMessage(kCreateLink));
		PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu*>
				(fCreateLinkItem->Submenu()),
			kCreateLink, item_ref, false);
	}

	fMoveToItem->SetEnabled(true);
	fCopyToItem->SetEnabled(true);
	fCreateLinkItem->SetEnabled(true);

	// Set the "Identify" item label
	BMenuItem* identifyItem = parent->FindItem(kIdentifyEntry);
	if (identifyItem != NULL) {
		if ((modifiers() & B_SHIFT_KEY) != 0) {
			identifyItem->SetLabel(B_TRANSLATE("Force identify"));
			identifyItem->Message()->ReplaceBool("force", true);
		} else {
			identifyItem->SetLabel(B_TRANSLATE("Identify"));
			identifyItem->Message()->ReplaceBool("force", false);
		}
	}
}


uint32
BContainerWindow::ShowDropContextMenu(BPoint loc)
{
	BPoint global(loc);

	PoseView()->ConvertToScreen(&global);
	PoseView()->CommitActivePose();

	// Change the "Create Link" item - allow user to
	// create relative links with the Shift key down.
	BMenuItem* item = fDropContextMenu->FindItem(kCreateLink);
	if (item == NULL)
		item = fDropContextMenu->FindItem(kCreateRelativeLink);

	if (item != NULL && (modifiers() & B_SHIFT_KEY) != 0) {
		item->SetLabel(B_TRANSLATE("Create relative link here"));
		item->SetMessage(new BMessage(kCreateRelativeLink));
	} else if (item != NULL) {
		item->SetLabel(B_TRANSLATE("Create link here"));
		item->SetMessage(new BMessage(kCreateLink));
	}

	item = fDropContextMenu->Go(global, true, true);
	if (item != NULL)
		return item->Command();

	return 0;
}


void
BContainerWindow::ShowContextMenu(BPoint loc, const entry_ref* ref, BView*)
{
	ASSERT(IsLocked());
	BPoint global(loc);
	PoseView()->ConvertToScreen(&global);
	PoseView()->CommitActivePose();

	if (ref != NULL) {
		// clicked on a pose, show file or volume context menu
		Model model(ref);

		if (model.IsTrash()) {
			if (fTrashContextMenu->Window() || Dragging())
				return;

			DeleteSubmenu(fNavigationItem);

			// selected item was trash, show the trash context menu instead

			EnableNamedMenuItem(fTrashContextMenu, kEmptyTrash,
				static_cast<TTracker*>(be_app)->TrashFull());

			SetupNavigationMenu(ref, fTrashContextMenu);
			fTrashContextMenu->Go(global, true, true, true);
		} else {
			bool showAsVolume = false;
			bool isFilePanel = PoseView()->IsFilePanel();

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

				if (!FSIsPrintersDir(&entry)
					&& !fDragContextMenu->IsShowing()) {
					//printf("ShowContextMenu - target is %s %i\n",
					//	ref->name, IsShowing(ref));
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
					BPoseView* poseView = PoseView();
					if (poseView != NULL) {
						BMessenger target(poseView);
						fDragContextMenu->InitTrackingHook(
							&BPoseView::MenuTrackingHook, &target,
							fDragMessage);
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

			if (fContextMenu != NULL) {
				if (fContextMenu->Window())
					return;
				else
					MenusEnded();

				if (model.InitCheck() == B_OK) { // ??? Do I need this ???
					if (showAsVolume) {
						// non-volume enable/disable copy, move, identify
						EnableNamedMenuItem(fContextMenu, kDuplicateSelection,
							false);
						EnableNamedMenuItem(fContextMenu, kMoveToTrash, false);
						EnableNamedMenuItem(fContextMenu, kIdentifyEntry,
							false);

						// volume model, enable/disable the Unmount item
						bool ejectableVolumeSelected = false;

						BVolume boot;
						BVolumeRoster().GetBootVolume(&boot);
						BVolume volume;
						volume.SetTo(model.NodeRef()->device);
						if (volume != boot)
							ejectableVolumeSelected = true;

						EnableNamedMenuItem(fContextMenu,
							B_TRANSLATE("Unmount"),	ejectableVolumeSelected);
					}
				}

				SetupNavigationMenu(ref, fContextMenu);
				if (!showAsVolume && !isFilePanel) {
					SetupMoveCopyMenus(ref, fContextMenu);
					SetupOpenWithMenu(fContextMenu);
				}

				UpdateMenu(fContextMenu, kPosePopUpContext);

				fContextMenu->Go(global, true, true, true);
			}
		}
	} else if (fWindowContextMenu != NULL) {
		if (fWindowContextMenu->Window())
			return;

		// Repopulate desktop menu if IsDesktop
		if (fIsDesktop)
			RepopulateMenus();

		MenusEnded();

		// clicked on a window, show window context menu

		SetupNavigationMenu(ref, fWindowContextMenu);
		UpdateMenu(fWindowContextMenu, kWindowPopUpContext);

		fWindowContextMenu->Go(global, true, true, true);
	}

	fContextMenu = NULL;
}


void
BContainerWindow::AddFileContextMenus(BMenu* menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"),
		new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
		new BMessage(kEditItem), 'E'));

	if (!IsTrash() && !InTrash() && !IsPrintersDir()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Duplicate"),
			new BMessage(kDuplicateSelection), 'D'));
	}

	if (!IsTrash() && !InTrash()) {
		menu->AddItem(new BMenuItem(TrackerSettings().DontMoveFilesToTrash()
			? B_TRANSLATE("Delete")	: B_TRANSLATE("Move to Trash"),
			new BMessage(kMoveToTrash), 'T'));
		if (!IsPrintersDir()) {
			// add separator for copy to/move to items (navigation items)
			menu->AddSeparatorItem();
		}
	} else {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Delete"),
			new BMessage(kDelete), 0));
		menu->AddItem(new BMenuItem(B_TRANSLATE("Restore"),
			new BMessage(kRestoreFromTrash), 0));
	}

#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	menu->AddSeparatorItem();
	BMenuItem* cutItem = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X');
	menu->AddItem(cutItem);
	BMenuItem* copyItem = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C');
	menu->AddItem(copyItem);
#endif

	menu->AddSeparatorItem();

	BMessage* message = new BMessage(kIdentifyEntry);
	message->AddBool("force", false);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Identify"), message));

	BMenu* addOnMenuItem = new BMenu(B_TRANSLATE("Add-ons"));
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
BContainerWindow::AddVolumeContextMenus(BMenu* menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"),
		new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
		new BMessage(kEditItem), 'E'));

	menu->AddSeparatorItem();
	menu->AddItem(new MountMenu(B_TRANSLATE("Mount")));

	BMenuItem* item = new BMenuItem(B_TRANSLATE("Unmount"),
		new BMessage(kUnmountVolume), 'U');
	item->SetEnabled(false);
	menu->AddItem(item);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenu(B_TRANSLATE("Add-ons")));

	menu->SetTargetForItems(PoseView());
}


void
BContainerWindow::AddWindowContextMenus(BMenu* menu)
{
	// create context sensitive menu for empty area of window
	// since we check view mode before display, this should be a radio
	// mode menu

	Model* targetModel = TargetModel();
	ASSERT(targetModel != NULL);

	bool needSeparator = true;
	if (IsTrash()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Empty Trash"),
			new BMessage(kEmptyTrash)));
	} else if (IsPrintersDir()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Add printer" B_UTF8_ELLIPSIS),
			new BMessage(kAddPrinter), 'N'));
	} else if (InTrash() || targetModel->IsRoot()) {
		needSeparator = false;
	} else {
		TemplatesMenu* templatesMenu = new TemplatesMenu(PoseView(),
			B_TRANSLATE("New"));
		menu->AddItem(templatesMenu);
		templatesMenu->SetTargetForItems(PoseView());
		templatesMenu->SetFont(be_plain_font);
	}

	if (needSeparator)
		menu->AddSeparatorItem();

#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	BMenuItem* pasteItem = new BMenuItem("Paste", new BMessage(B_PASTE), 'V');
	menu->AddItem(pasteItem);
	menu->AddSeparatorItem();
#endif

	BMenu* arrangeBy = new BMenu(B_TRANSLATE("Arrange by"));
	PopulateArrangeByMenu(arrangeBy);
	menu->AddItem(arrangeBy);

	menu->AddItem(new BMenuItem(B_TRANSLATE("Select" B_UTF8_ELLIPSIS),
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A'));
	if (!IsTrash()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Open parent"),
			new BMessage(kOpenParentDir), B_UP_ARROW));
	}

	if (targetModel->IsRoot()) {
		menu->AddSeparatorItem();
		menu->AddItem(new MountMenu(B_TRANSLATE("Mount")));
	}

	menu->AddSeparatorItem();
	BMenu* addOnMenuItem = new BMenu(B_TRANSLATE("Add-ons"));
	addOnMenuItem->SetFont(be_plain_font);
	menu->AddItem(addOnMenuItem);

#if DEBUG
	menu->AddSeparatorItem();
	BMenuItem* testing = new BMenuItem("Test icon cache",
		new BMessage(kTestIconCache));
	menu->AddItem(testing);
#endif

	// target items as needed
	menu->SetTargetForItems(PoseView());
#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	pasteItem->SetTarget(this);
#endif
}


void
BContainerWindow::AddDropContextMenus(BMenu* menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Create link here"),
		new BMessage(kCreateLink)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Move here"),
		new BMessage(kMoveSelectionTo)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Copy here"),
		new BMessage(kCopySelectionTo)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Cancel"),
		new BMessage(kCancelButton)));
}


void
BContainerWindow::AddTrashContextMenus(BMenu* menu)
{
	// setup special trash context menu
	menu->AddItem(new BMenuItem(B_TRANSLATE("Empty Trash"),
		new BMessage(kEmptyTrash)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"),
		new BMessage(kGetInfo), 'I'));
	menu->SetTargetForItems(PoseView());
}


void
BContainerWindow::EachAddon(bool (*eachAddon)(const Model*, const char*,
		uint32 shortcut, uint32 modifiers, bool primary, void* context,
		BContainerWindow* window, BMenu* menu),
	void* passThru, BStringList& mimeTypes, BMenu* menu)
{
	AutoLock<LockingList<AddonShortcut> > lock(fAddonsList);
	if (lock.IsLocked()) {
		for (int i = fAddonsList->CountItems() - 1; i >= 0; i--) {
			struct AddonShortcut* item = fAddonsList->ItemAt(i);
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
							if (message.GetInfo("types", &typeCode,
									&count) == B_OK) {
								secondary = false;
							}
						}

						// check all supported types if it has some set
						if (!secondary) {
							for (int32 i = mimeTypes.CountStrings();
									!primary && i-- > 0;) {
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
			((eachAddon)(item->model, item->model->Name(), item->key,
				item->modifiers, primary, passThru, this, menu));
		}
	}
}


void
BContainerWindow::BuildMimeTypeList(BStringList& mimeTypes)
{
	int32 count = PoseView()->SelectionList()->CountItems();
	if (count <= 0) {
		// just add the type of the current directory
		AddMimeTypeString(mimeTypes, TargetModel());
	} else {
		_UpdateSelectionMIMEInfo();
		for (int32 index = 0; index < count; index++) {
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
BContainerWindow::BuildAddOnMenu(BMenu* parentMenu)
{
	BMenuItem* item = parentMenu->FindItem(B_TRANSLATE("Add-ons"));
	if (parentMenu->IndexOf(item) == 0) {
		// the folder of the context menu seems to be named "Add-Ons"
		// so we just take the last menu item, which is correct if not
		// build with debug option
		item = parentMenu->ItemAt(parentMenu->CountItems() - 1);
	}
	if (item == NULL)
		return;

	BFont font;
	parentMenu->GetFont(&font);

	BMenu* menu = item->Submenu();
	if (menu == NULL)
		return;

	menu->SetFont(&font);

	// found the addons menu, empty it first
	for (;;) {
		item = menu->RemoveItem((int32)0);
		if (!item)
			break;
		delete item;
	}

	BObjectList<BMenuItem> primaryList;
	BObjectList<BMenuItem> secondaryList;
	BStringList mimeTypes(10);
	BuildMimeTypeList(mimeTypes);

	AddOneAddonParams params;
	params.primaryList = &primaryList;
	params.secondaryList = &secondaryList;

	// build a list of the MIME types of the selected items

	EachAddon(AddOneAddon, &params, mimeTypes, parentMenu);

	primaryList.SortItems(CompareLabels);
	secondaryList.SortItems(CompareLabels);

	int32 count = primaryList.CountItems();
	for (int32 index = 0; index < count; index++)
		menu->AddItem(primaryList.ItemAt(index));

	if (count > 0)
		menu->AddSeparatorItem();

	count = secondaryList.CountItems();
	for (int32 index = 0; index < count; index++)
		menu->AddItem(secondaryList.ItemAt(index));

	menu->SetTargetForItems(this);
}


void
BContainerWindow::UpdateMenu(BMenu* menu, UpdateMenuContext context)
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

	Model* selectedModel = NULL;
	if (selectCount == 1) {
		selectedModel = PoseView()->SelectionList()->FirstItem()->
			TargetModel();
	}

	if (context == kMenuBarContext || context == kPosePopUpContext) {
		SetUpEditQueryItem(menu);
		EnableNamedMenuItem(menu, kEditItem, selectCount == 1
			&& (context == kPosePopUpContext || !PoseView()->ActivePose())
			&& selectedModel != NULL
			&& !selectedModel->IsDesktop()
			&& !selectedModel->IsRoot()
			&& !selectedModel->IsTrash()
			&& !selectedModel->HasLocalizedName());
		SetCutItem(menu);
		SetCopyItem(menu);
		SetPasteItem(menu);
	}

	if (context == kMenuBarContext || context == kWindowPopUpContext) {
		uint32 viewMode = PoseView()->ViewMode();

		BMenu* iconSizeMenu = NULL;
		if (BMenuItem* item = menu->FindItem(kIconMode))
			iconSizeMenu = item->Submenu();

		if (iconSizeMenu != NULL) {
			if (viewMode == kIconMode) {
				int32 iconSize = PoseView()->IconSizeInt();
				BMenuItem* item = iconSizeMenu->ItemAt(0);
				for (int32 i = 0; (item = iconSizeMenu->ItemAt(i)) != NULL;
						i++) {
					BMessage* message = item->Message();
					if (message == NULL) {
						item->SetMarked(false);
						continue;
					}
					int32 size;
					if (message->FindInt32("size", &size) != B_OK)
						size = -1;
					item->SetMarked(iconSize == size);
				}
			} else {
				BMenuItem* item;
				for (int32 i = 0; (item = iconSizeMenu->ItemAt(i)) != NULL; i++)
					item->SetMarked(false);
			}
		}

		MarkNamedMenuItem(menu, kIconMode, viewMode == kIconMode);
		MarkNamedMenuItem(menu, kListMode, viewMode == kListMode);
		MarkNamedMenuItem(menu, kMiniIconMode, viewMode == kMiniIconMode);

		SetCloseItem(menu);
		SetArrangeMenu(menu);
		SetPasteItem(menu);

		BEntry entry(TargetModel()->EntryRef());
		BDirectory parent;
		entry_ref ref;
		BEntry root("/");

		bool parentIsRoot = (entry.GetParent(&parent) == B_OK
			&& parent.GetEntry(&entry) == B_OK
			&& entry.GetRef(&ref) == B_OK
			&& entry == root);

		EnableNamedMenuItem(menu, kOpenParentDir, !TargetModel()->IsDesktop()
			&& !TargetModel()->IsRoot()
			&& (!parentIsRoot
				|| TrackerSettings().SingleWindowBrowse()
				|| TrackerSettings().ShowDisksIcon()
				|| (modifiers() & B_CONTROL_KEY) != 0));

		EnableNamedMenuItem(menu, kEmptyTrash, count > 0);
		EnableNamedMenuItem(menu, B_SELECT_ALL, count > 0);

		BMenuItem* item = menu->FindItem(B_TRANSLATE("New"));
		if (item != NULL) {
			TemplatesMenu* templatesMenu = dynamic_cast<TemplatesMenu*>(
				item->Submenu());
			if (templatesMenu != NULL)
				templatesMenu->UpdateMenuState();
		}
	}

	BuildAddOnMenu(menu);
}


BMessage*
BContainerWindow::AddOnMessage(int32 what)
{
	BMessage* message = new BMessage(what);

	// add selected refs to message
	BObjectList<BPose>* selectionList = PoseView()->SelectionList();

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

	entry_ref addonRef;
	status_t result = message->FindRef("refs", &addonRef);
	if (result != B_OK) {
		BString buffer(B_TRANSLATE("Error %error loading add-On %name."));
		buffer.ReplaceFirst("%error", strerror(result));
		buffer.ReplaceFirst("%name", addonRef.name);

		BAlert* alert = new BAlert("", buffer.String(),	B_TRANSLATE("Cancel"),
			0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return;
	}

	// add selected refs to message
	BMessage* refs = AddOnMessage(B_REFS_RECEIVED);

	LaunchInNewThread("Add-on", B_NORMAL_PRIORITY, &AddOnThread, refs,
		addonRef, *TargetModel()->EntryRef());
}


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

	float iconSize = fMenuBar->Bounds().Height() - 2;
	if (iconSize < 16)
		iconSize = 16;

	fDraggableIcon = new(std::nothrow) DraggableContainerIcon();
	if (fDraggableIcon != NULL) {
		BLayoutItem* item = fMenuContainer->GroupLayout()->AddView(
			fDraggableIcon);
		item->SetExplicitMinSize(BSize(iconSize + 5, iconSize));
		item->SetExplicitMaxSize(BSize(iconSize + 5, item->MaxSize().Height()));

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
BContainerWindow::NewAttributeMenu(BMenu* menu)
{
	ASSERT(PoseView());

	BMenuItem* item;
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

	if (IsTrash() || InTrash()) {
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
BContainerWindow::MarkAttributeMenu(BMenu* menu)
{
	if (menu == NULL)
		return;

	int32 count = menu->CountItems();
	for (int32 index = 0; index < count; index++) {
		BMenuItem* item = menu->ItemAt(index);
		int32 attrHash;
		if (item->Message() != NULL) {
			if (item->Message()->FindInt32("attr_hash", &attrHash) == B_OK)
				item->SetMarked(PoseView()->ColumnFor((uint32)attrHash) != 0);
			else
				item->SetMarked(false);
		}

		BMenu* submenu = item->Submenu();
		if (submenu != NULL) {
			int32 count2 = submenu->CountItems();
			for (int32 subindex = 0; subindex < count2; subindex++) {
				item = submenu->ItemAt(subindex);
				if (item->Message() != NULL) {
					if (item->Message()->FindInt32("attr_hash", &attrHash)
						== B_OK) {
						item->SetMarked(PoseView()->ColumnFor((uint32)attrHash)
							!= 0);
					} else
						item->SetMarked(false);
				}
			}
		}
	}
}


void
BContainerWindow::MarkArrangeByMenu(BMenu* menu)
{
	if (menu == NULL)
		return;

	int32 count = menu->CountItems();
	for (int32 index = 0; index < count; index++) {
		BMenuItem* item = menu->ItemAt(index);
		if (item->Message() != NULL) {
			uint32 attrHash;
			if (item->Message()->FindInt32("attr_hash",
					(int32*)&attrHash) == B_OK) {
				item->SetMarked(PoseView()->PrimarySort() == attrHash);
			} else if (item->Command() == kArrangeReverseOrder)
				item->SetMarked(PoseView()->ReverseSort());
		}
	}
}


void
BContainerWindow::AddMimeTypesToMenu()
{
	AddMimeTypesToMenu(fAttrMenu);
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
	BMenuItem* item = menu->ItemAt(menu->CountItems() - 1);
	if (dynamic_cast<BSeparatorItem*>(item) != NULL) {
		menu->RemoveItem(item);
		delete item;
	}

	MarkAttributeMenu(menu);
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
BContainerWindow::SetUpDefaultState()
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

	if (fIsDesktop) {
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
	if (node == NULL || fIsDesktop) {
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
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	} else
		sNewWindRect.OffsetBy(kWindowStaggerBy, kWindowStaggerBy);

	fPreviousBounds = Bounds();

	uint32 workspace;
	if (((fContainerWindowFlags & kRestoreWorkspace) != 0)
		&& node->Read(workspaceAttributeName, 0, B_INT32_TYPE, sizeof(uint32),
			&workspace) == sizeof(uint32))
		SetWorkspaces(workspace);

	if ((fContainerWindowFlags & kIsHidden) != 0)
		Minimize(true);

	// restore window decor settings
	int32 size = node->Contains(kAttrWindowDecor, B_RAW_TYPE);
	if (size > 0) {
		char buffer[size];
		if (((fContainerWindowFlags & kRestoreDecor) != 0)
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
	if (fIsDesktop) {
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
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	} else
		sNewWindRect.OffsetBy(kWindowStaggerBy, kWindowStaggerBy);

	uint32 workspace;
	if ((fContainerWindowFlags & kRestoreWorkspace)
		&& message.FindInt32(workspaceAttributeName,
			(int32*)&workspace) == B_OK) {
		SetWorkspaces(workspace);
	}

	if (fContainerWindowFlags & kIsHidden)
		Minimize(true);

	// restore window decor settings
	BMessage decorSettings;
	if ((fContainerWindowFlags & kRestoreDecor)
		&& message.FindMessage(kAttrWindowDecor, &decorSettings) == B_OK) {
		SetDecoratorSettings(decorSettings);
	}
}


void
BContainerWindow::SaveWindowState(AttributeStreamNode* node)
{
	if (fIsDesktop) {
		// don't save window state if we are the Desktop
		return;
	}

	ASSERT(node != NULL);

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
	message.AddRect(rectAttributeName, frame);
	message.AddInt32(workspaceAttributeName, (int32)Workspaces());

	BMessage decorSettings;
	if (GetDecoratorSettings(&decorSettings) == B_OK) {
		message.AddMessage(kAttrWindowDecor, &decorSettings);
	}
}


status_t
BContainerWindow::DragStart(const BMessage* dragMessage)
{
	if (dragMessage == NULL)
		return B_ERROR;

	// if already dragging, or
	// if all the refs match
	if (Dragging()
		&& SpringLoadedFolderCompareMessages(dragMessage, fDragMessage)) {
		return B_OK;
	}

	// cache the current drag message
	// build a list of the mimetypes in the message
	SpringLoadedFolderCacheDragData(dragMessage, &fDragMessage,
		&fCachedTypesList);

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
	if (PoseView()->IsDesktopWindow() || !TargetModel()->IsDirectory()
		|| fPoseView->IsFilePanel()) {
		return;
	}

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
	if (PoseView()->IsDesktopWindow())
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


void
BContainerWindow::PopulateArrangeByMenu(BMenu* menu)
{
	if (!fAttrMenu || !menu)
		return;
	// empty fArrangeByMenu...
	BMenuItem* item;
	while ((item = menu->RemoveItem((int32)0)) != NULL)
		delete item;

	int32 itemCount = fAttrMenu->CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		item = fAttrMenu->ItemAt(i);
		if (item->Command() == kAttributeItem) {
			BMessage* message = new BMessage(*(item->Message()));
			message->what = kArrangeBy;
			BMenuItem* newItem = new BMenuItem(item->Label(), message);
			newItem->SetTarget(PoseView());
			menu->AddItem(newItem);
		}
	}

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Reverse order"),
		new BMessage(kArrangeReverseOrder));

	item->SetTarget(PoseView());
	menu->AddItem(item);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Clean up"), new BMessage(kCleanup), 'K');
	item->SetTarget(PoseView());
	menu->AddItem(item);
}


//	#pragma mark - WindowStateNodeOpener


WindowStateNodeOpener::WindowStateNodeOpener(BContainerWindow* window,
	bool forWriting)
	:
	fModelOpener(NULL),
	fNode(NULL),
	fStreamNode(NULL)
{
	if (window->TargetModel() && window->TargetModel()->IsRoot()) {
		BDirectory dir;
		if (FSGetDeskDir(&dir) == B_OK) {
			fNode = new BDirectory(dir);
			fStreamNode = new AttributeStreamFileNode(fNode);
		}
	} else if (window->TargetModel()){
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
