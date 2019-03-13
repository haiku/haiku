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


#include "DeskWindow.h"

#include <Catalog.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Messenger.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PathFinder.h>
#include <PathMonitor.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Screen.h>
#include <String.h>
#include <StringList.h>
#include <Volume.h>
#include <WindowPrivate.h>

#include <fcntl.h>
#include <unistd.h>

#include "Attributes.h"
#include "AutoLock.h"
#include "BackgroundImage.h"
#include "Commands.h"
#include "FSUtils.h"
#include "IconMenuItem.h"
#include "KeyInfos.h"
#include "MountMenu.h"
#include "PoseView.h"
#include "Tracker.h"
#include "TemplatesMenu.h"


const char* kShelfPath = "tracker_shelf";
	// replicant support

const char* kShortcutsSettings = "shortcuts_settings";
const char* kDefaultShortcut = "BEOS:default_shortcut";
const uint32 kDefaultModifiers = B_OPTION_KEY | B_COMMAND_KEY;


static struct AddonShortcut*
MatchOne(struct AddonShortcut* item, void* castToName)
{
	if (strcmp(item->model->Name(), (const char*)castToName) == 0) {
		// found match, bail out
		return item;
	}

	return 0;
}


static void
AddOneShortcut(Model* model, char key, uint32 modifiers, BDeskWindow* window)
{
	if (key == '\0')
		return;

	BMessage* runAddon = new BMessage(kLoadAddOn);
	runAddon->AddRef("refs", model->EntryRef());
	window->AddShortcut(key, modifiers, runAddon);
}



static struct AddonShortcut*
RevertToDefault(struct AddonShortcut* item, void* castToWindow)
{
	if (item->key != item->defaultKey || item->modifiers != kDefaultModifiers) {
		BDeskWindow* window = static_cast<BDeskWindow*>(castToWindow);
		if (window != NULL) {
			window->RemoveShortcut(item->key, item->modifiers);
			item->key = item->defaultKey;
			item->modifiers = kDefaultModifiers;
			AddOneShortcut(item->model, item->key, item->modifiers, window);
		}
	}

	return 0;
}


static struct AddonShortcut*
FindElement(struct AddonShortcut* item, void* castToOther)
{
	Model* other = static_cast<Model*>(castToOther);
	if (*item->model->EntryRef() == *other->EntryRef())
		return item;

	return 0;
}


static void
LoadAddOnDir(BDirectory directory, BDeskWindow* window,
	LockingList<AddonShortcut>* list)
{
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		Model* model = new Model(&entry);
		if (model->InitCheck() == B_OK && model->IsSymLink()) {
			// resolve symlinks
			Model* resolved = new Model(model->EntryRef(), true, true);
			if (resolved->InitCheck() == B_OK)
				model->SetLinkTo(resolved);
			else
				delete resolved;
		}
		if (model->InitCheck() != B_OK
			|| !model->ResolveIfLink()->IsExecutable()) {
			delete model;
			continue;
		}

		char* name = strdup(model->Name());
		if (!list->EachElement(MatchOne, name)) {
			struct AddonShortcut* item = new struct AddonShortcut;
			item->model = model;

			BResources resources(model->ResolveIfLink()->EntryRef());
			size_t size;
			char* shortcut = (char*)resources.LoadResource(B_STRING_TYPE,
				kDefaultShortcut, &size);
			if (shortcut == NULL || strlen(shortcut) > 1)
				item->key = '\0';
			else
				item->key = shortcut[0];
			AddOneShortcut(model, item->key, kDefaultModifiers, window);
			item->defaultKey = item->key;
			item->modifiers = kDefaultModifiers;
			list->AddItem(item);
		}
		free(name);
	}

	node_ref nodeRef;
	directory.GetNodeRef(&nodeRef);

	TTracker::WatchNode(&nodeRef, B_WATCH_DIRECTORY, window);
}


// #pragma mark - BDeskWindow


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeskWindow"


BDeskWindow::BDeskWindow(LockingList<BWindow>* windowList)
	:
	BContainerWindow(windowList, 0, kDesktopWindowLook,
		kDesktopWindowFeel, B_NOT_MOVABLE | B_WILL_ACCEPT_FIRST_CLICK
			| B_NOT_ZOOMABLE | B_NOT_CLOSABLE | B_NOT_MINIMIZABLE
			| B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS, B_ALL_WORKSPACES,
			false, true),
	fDeskShelf(NULL),
	fNodeRef(NULL),
	fShortcutsSettings(NULL)
{
	// Add icon view switching shortcuts. These are displayed in the context
	// menu, although they obviously don't work from those menu items.
	BMessage* message = new BMessage(kIconMode);
	AddShortcut('1', B_COMMAND_KEY, message, PoseView());

	message = new BMessage(kMiniIconMode);
	AddShortcut('2', B_COMMAND_KEY, message, PoseView());

	message = new BMessage(kIconMode);
	message->AddInt32("scale", 1);
	AddShortcut('+', B_COMMAND_KEY, message, PoseView());

	message = new BMessage(kIconMode);
	message->AddInt32("scale", 0);
	AddShortcut('-', B_COMMAND_KEY, message, PoseView());
}


BDeskWindow::~BDeskWindow()
{
	SaveDesktopPoseLocations();
		// explicit call to SavePoseLocations so that extended pose info
		// gets committed properly
	PoseView()->DisableSaveLocation();
		// prevent double-saving, this would slow down quitting
	PoseView()->StopSettingsWatch();
	stop_watching(this);
}


void
BDeskWindow::Init(const BMessage*)
{
	// Set the size of the screen before calling the container window's
	// Init() because it will add volume poses to this window and
	// they will be clipped otherwise

	BScreen screen(this);
	fOldFrame = screen.Frame();

	PoseView()->SetShowHideSelection(false);
	ResizeTo(fOldFrame.Width(), fOldFrame.Height());

	InitKeyIndices();
	InitAddonsList(false);
	ApplyShortcutPreferences(false);

	_inherited::Init();

	entry_ref ref;
	BPath path;
	if (!BootedInSafeMode() && FSFindTrackerSettingsDir(&path) == B_OK) {
		path.Append(kShelfPath);
		close(open(path.Path(), O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR
			| S_IRGRP | S_IROTH));
		if (get_ref_for_path(path.Path(), &ref) == B_OK)
			fDeskShelf = new BShelf(&ref, fPoseView);

		if (fDeskShelf != NULL)
			fDeskShelf->SetDisplaysZombies(true);
	}
}


void
BDeskWindow::InitAddonsList(bool update)
{
	AutoLock<LockingList<AddonShortcut> > lock(fAddonsList);
	if (lock.IsLocked()) {
		if (update) {
			for (int i = fAddonsList->CountItems() - 1; i >= 0; i--) {
				AddonShortcut* item = fAddonsList->ItemAt(i);
				RemoveShortcut(item->key, B_OPTION_KEY | B_COMMAND_KEY);
			}
			fAddonsList->MakeEmpty(true);
		}

		BStringList addOnPaths;
		BPathFinder::FindPaths(B_FIND_PATH_ADD_ONS_DIRECTORY, "Tracker",
			addOnPaths);
		int32 count = addOnPaths.CountStrings();
		for (int32 i = 0; i < count; i++) {
			LoadAddOnDir(BDirectory(addOnPaths.StringAt(i)), this,
				fAddonsList);
		}
	}
}


void
BDeskWindow::ApplyShortcutPreferences(bool update)
{
	AutoLock<LockingList<AddonShortcut> > lock(fAddonsList);
	if (lock.IsLocked()) {
		if (!update) {
			BPath path;
			if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
				BPathMonitor::StartWatching(path.Path(),
					B_WATCH_STAT | B_WATCH_FILES_ONLY, this);
				path.Append(kShortcutsSettings);
				fShortcutsSettings = new char[strlen(path.Path()) + 1];
				strcpy(fShortcutsSettings, path.Path());
			}
		}

		fAddonsList->EachElement(RevertToDefault, this);

		BFile shortcutSettings(fShortcutsSettings, B_READ_ONLY);
		BMessage fileMsg;
		if (shortcutSettings.InitCheck() != B_OK
			|| fileMsg.Unflatten(&shortcutSettings) != B_OK) {
			fNodeRef = NULL;
			return;
		}
		shortcutSettings.GetNodeRef(fNodeRef);

		int32 i = 0;
		BMessage message;
		while (fileMsg.FindMessage("spec", i++, &message) == B_OK) {
			int32 key;
			if (message.FindInt32("key", &key) == B_OK) {
				// only handle shortcuts referring add-ons
				BString command;
				if (message.FindString("command", &command) != B_OK)
					continue;

				bool isInAddons = false;

				BStringList addOnPaths;
				BPathFinder::FindPaths(B_FIND_PATH_ADD_ONS_DIRECTORY,
					"Tracker/", addOnPaths);
				for (int32 i = 0; i < addOnPaths.CountStrings(); i++) {
					if (command.StartsWith(addOnPaths.StringAt(i))) {
						isInAddons = true;
						break;
					}
				}

				if (!isInAddons)
					continue;

				BEntry entry(command);
				if (entry.InitCheck() != B_OK)
					continue;

				const char* shortcut = GetKeyName(key);
				if (strlen(shortcut) != 1)
					continue;

				uint32 modifiers = B_COMMAND_KEY;
					// it's required by interface kit to at least
					// have B_COMMAND_KEY
				int32 value;
				if (message.FindInt32("mcidx", 0, &value) == B_OK)
					modifiers |= (value != 0 ? B_SHIFT_KEY : 0);

				if (message.FindInt32("mcidx", 1, &value) == B_OK)
					modifiers |= (value != 0 ? B_CONTROL_KEY : 0);

				if (message.FindInt32("mcidx", 3, &value) == B_OK)
					modifiers |= (value != 0 ? B_OPTION_KEY : 0);

				Model model(&entry);
				AddonShortcut* item = fAddonsList->EachElement(FindElement,
					&model);
				if (item != NULL) {
					if (item->key != '\0')
						RemoveShortcut(item->key, item->modifiers);

					item->key = shortcut[0];
					item->modifiers = modifiers;
					AddOneShortcut(&model, item->key, item->modifiers, this);
				}
			}
		}
	}
}


void
BDeskWindow::Quit()
{
	if (fNavigationItem != NULL) {
		// this duplicates BContainerWindow::Quit because
		// fNavigationItem can be part of fTrashContextMenu
		// and would get deleted with it
		BMenu* menu = fNavigationItem->Menu();
		if (menu != NULL)
			menu->RemoveItem(fNavigationItem);

		delete fNavigationItem;
		fNavigationItem = 0;
	}

	fAddonsList->MakeEmpty(true);
	delete fAddonsList;

	delete fDeskShelf;
	_inherited::Quit();
}


BPoseView*
BDeskWindow::NewPoseView(Model* model, uint32 viewMode)
{
	return new DesktopPoseView(model, viewMode);
}


void
BDeskWindow::CreatePoseView(Model* model)
{
	fPoseView = NewPoseView(model, kIconMode);
	fPoseView->SetIconMapping(false);
	fPoseView->SetEnsurePosesVisible(true);
	fPoseView->SetAutoScroll(false);

	BScreen screen(this);
	rgb_color desktopColor = screen.DesktopColor();
	if (desktopColor.alpha != 255) {
		desktopColor.alpha = 255;
#if B_BEOS_VERSION > B_BEOS_VERSION_5
		// This call seems to have the power to cause R5 to freeze!
		// Please report if commenting this out helped or helped not
		// on your system
		screen.SetDesktopColor(desktopColor);
#endif
	}

	fPoseView->SetViewColor(desktopColor);
	fPoseView->SetLowColor(desktopColor);

	fPoseView->SetResizingMode(B_FOLLOW_ALL);
	fPoseView->ResizeTo(Bounds().Size());
	AddChild(fPoseView);

	PoseView()->StartSettingsWatch();
}


void
BDeskWindow::AddWindowContextMenus(BMenu* menu)
{
	TemplatesMenu* tempateMenu = new TemplatesMenu(PoseView(),
		B_TRANSLATE("New"));

	menu->AddItem(tempateMenu);
	tempateMenu->SetTargetForItems(PoseView());
	tempateMenu->SetFont(be_plain_font);

	menu->AddSeparatorItem();

	BMenu* iconSizeMenu = new BMenu(B_TRANSLATE("Icon view"));

	BMessage* message = new BMessage(kIconMode);
	message->AddInt32("size", 32);
	BMenuItem* item = new BMenuItem(B_TRANSLATE("32 x 32"), message);
	item->SetMarked(PoseView()->IconSizeInt() == 32);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 40);
	item = new BMenuItem(B_TRANSLATE("40 x 40"), message);
	item->SetMarked(PoseView()->IconSizeInt() == 40);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 48);
	item = new BMenuItem(B_TRANSLATE("48 x 48"), message);
	item->SetMarked(PoseView()->IconSizeInt() == 48);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 64);
	item = new BMenuItem(B_TRANSLATE("64 x 64"), message);
	item->SetMarked(PoseView()->IconSizeInt() == 64);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 96);
	item = new BMenuItem(B_TRANSLATE("96 x 96"), message);
	item->SetMarked(PoseView()->IconSizeInt() == 96);
	item->SetTarget(PoseView());
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 128);
	item = new BMenuItem(B_TRANSLATE("128 x 128"), message);
	item->SetMarked(PoseView()->IconSizeInt() == 128);
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
	iconSizeMenu->Superitem()->SetMarked(PoseView()->ViewMode() == kIconMode);

	item = new BMenuItem(B_TRANSLATE("Mini icon view"),
		new BMessage(kMiniIconMode), '2');
	item->SetMarked(PoseView()->ViewMode() == kMiniIconMode);
	menu->AddItem(item);

	menu->AddSeparatorItem();

#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	BMenuItem* pasteItem = new BMenuItem(B_TRANSLATE("Paste"),
		new BMessage(B_PASTE), 'V');
	menu->AddItem(pasteItem);
	menu->AddSeparatorItem();
#endif
	menu->AddItem(new BMenuItem(B_TRANSLATE("Clean up"),
		new BMessage(kCleanup), 'K'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Select" B_UTF8_ELLIPSIS),
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A'));

	menu->AddSeparatorItem();
	menu->AddItem(new MountMenu(B_TRANSLATE("Mount")));

	menu->AddSeparatorItem();
	menu->AddItem(new BMenu(B_TRANSLATE("Add-ons")));

	// target items as needed
	menu->SetTargetForItems(PoseView());
#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	pasteItem->SetTarget(this);
#endif
}


void
BDeskWindow::WorkspaceActivated(int32 workspace, bool state)
{
	if (fBackgroundImage)
		fBackgroundImage->WorkspaceActivated(PoseView(), workspace, state);
}


void
BDeskWindow::SaveDesktopPoseLocations()
{
	PoseView()->SavePoseLocations(&fOldFrame);
}


void
BDeskWindow::ScreenChanged(BRect frame, color_space space)
{
	bool frameChanged = (frame != fOldFrame);

	SaveDesktopPoseLocations();
	fOldFrame = frame;
	ResizeTo(frame.Width(), frame.Height());

	if (fBackgroundImage)
		fBackgroundImage->ScreenChanged(frame, space);

	PoseView()->CheckPoseVisibility(frameChanged ? &frame : 0);
		// if frame changed, pass new frame so that icons can
		// get rearranged based on old pose info for the frame
}


void
BDeskWindow::UpdateDesktopBackgroundImages()
{
	WindowStateNodeOpener opener(this, false);
	fBackgroundImage = BackgroundImage::Refresh(fBackgroundImage,
		opener.Node(), true, PoseView());
}


void
BDeskWindow::Show()
{
	if (fBackgroundImage)
		fBackgroundImage->Show(PoseView(), current_workspace());

	PoseView()->CheckPoseVisibility();

	_inherited::Show();
}


bool
BDeskWindow::ShouldAddScrollBars() const
{
	return false;
}


bool
BDeskWindow::ShouldAddMenus() const
{
	return false;
}


bool
BDeskWindow::ShouldAddContainerView() const
{
	return false;
}


void
BDeskWindow::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		const rgb_color* color;
		ssize_t size;
		// handle "roColour"-style color drops
		if (message->FindData("RGBColor", 'RGBC',
			(const void**)&color, &size) == B_OK) {
			BScreen(this).SetDesktopColor(*color);
			fPoseView->SetViewColor(*color);
			fPoseView->SetLowColor(*color);

			// Notify the backgrounds app that the background changed
			status_t initStatus;
			BMessenger messenger("application/x-vnd.Haiku-Backgrounds", -1,
				&initStatus);
			if (initStatus == B_OK)
				messenger.SendMessage(message);

			return;
		}
	}

	switch (message->what) {
		case B_PATH_MONITOR:
		{
			const char* path = "";
			if (!(message->FindString("path", &path) == B_OK
					&& strcmp(path, fShortcutsSettings) == 0)) {

				dev_t device;
				ino_t node;
				if (fNodeRef == NULL
					|| message->FindInt32("device", &device) != B_OK
					|| message->FindInt64("node", &node) != B_OK
					|| device != fNodeRef->device
					|| node != fNodeRef->node)
					break;
			}
			ApplyShortcutPreferences(true);
			break;
		}
		case B_NODE_MONITOR:
			PRINT(("will update addon shortcuts\n"));
			InitAddonsList(true);
			ApplyShortcutPreferences(true);
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}
