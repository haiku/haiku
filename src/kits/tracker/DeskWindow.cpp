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

#include <Catalog.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <fcntl.h>
#include <unistd.h>

#include "Attributes.h"
#include "AutoLock.h"
#include "BackgroundImage.h"
#include "Commands.h"
#include "DesktopPoseView.h"
#include "DeskWindow.h"
#include "FSUtils.h"
#include "IconMenuItem.h"
#include "MountMenu.h"
#include "PoseView.h"
#include "Tracker.h"
#include "TemplatesMenu.h"


const char *kShelfPath = "tracker_shelf";
	// replicant support


static void
WatchAddOnDir(directory_which dirName, BDeskWindow *window)
{
	BPath path;
	if (find_directory(dirName, &path) == B_OK) {
		path.Append("Tracker");
		BNode node(path.Path());
		node_ref nodeRef;
		node.GetNodeRef(&nodeRef);
		TTracker::WatchNode(&nodeRef, B_WATCH_DIRECTORY, window);
	}
}


struct AddOneShortcutParams {
	BDeskWindow *window;
	std::set<uint32> *currentAddonShortcuts;
};

static bool
AddOneShortcut(const Model *model, const char *, uint32 shortcut, bool /*primary*/, void *context)
{
	if (!shortcut)
		// no shortcut, bail
		return false;

	AddOneShortcutParams *params = (AddOneShortcutParams *)context;
	BMessage *runAddon = new BMessage(kLoadAddOn);
	runAddon->AddRef("refs", model->EntryRef());

	params->window->AddShortcut(shortcut, B_OPTION_KEY | B_COMMAND_KEY,
		runAddon);
	params->currentAddonShortcuts->insert(shortcut);
	PRINT(("adding new shortcut %c\n", (char)shortcut));

	return false;
}


// #pragma mark -

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeskWindow"

BDeskWindow::BDeskWindow(LockingList<BWindow> *windowList)
	:
	BContainerWindow(windowList, 0, kPrivateDesktopWindowLook,
		kPrivateDesktopWindowFeel, B_NOT_MOVABLE | B_WILL_ACCEPT_FIRST_CLICK
			| B_NOT_ZOOMABLE | B_NOT_CLOSABLE | B_NOT_MINIMIZABLE
			| B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS, B_ALL_WORKSPACES),
	fDeskShelf(0),
	fTrashContextMenu(0),
	fShouldUpdateAddonShortcuts(true)
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
BDeskWindow::Init(const BMessage *)
{
	//
	//	Set the size of the screen before calling the container window's
	//	Init() because it will add volume poses to this window and
	// 	they will be clipped otherwise
	//
	BScreen screen(this);
	fOldFrame = screen.Frame();

	PoseView()->SetShowHideSelection(false);
	ResizeTo(fOldFrame.Width(), fOldFrame.Height());

	entry_ref ref;
	BPath path;
	if (!BootedInSafeMode() && FSFindTrackerSettingsDir(&path) == B_OK) {
		path.Append(kShelfPath);
		close(open(path.Path(), O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
		if (get_ref_for_path(path.Path(), &ref) == B_OK)
			fDeskShelf = new BShelf(&ref, fPoseView);
		if (fDeskShelf)
			fDeskShelf->SetDisplaysZombies(true);
	}

	// watch add-on directories so that we can track the addons with
	// corresponding shortcuts
	WatchAddOnDir(B_USER_ADDONS_DIRECTORY, this);
	WatchAddOnDir(B_COMMON_ADDONS_DIRECTORY, this);
	WatchAddOnDir(B_SYSTEM_ADDONS_DIRECTORY, this);

	_inherited::Init();
}


void
BDeskWindow::MenusBeginning()
{
	_inherited::MenusBeginning();

	if (fShouldUpdateAddonShortcuts) {
		PRINT(("updating addon shortcuts\n"));
		fShouldUpdateAddonShortcuts = false;

		// remove all current addon shortcuts
		for (std::set<uint32>::iterator it= fCurrentAddonShortcuts.begin();
			it != fCurrentAddonShortcuts.end(); it++) {
			PRINT(("removing shortcut %c\n", (int)*it));
			RemoveShortcut(*it, B_OPTION_KEY | B_COMMAND_KEY);
		}

		fCurrentAddonShortcuts.clear();

		AddOneShortcutParams params;
		params.window = this;
		params.currentAddonShortcuts = &fCurrentAddonShortcuts;
		EachAddon(&AddOneShortcut, &params);
	}
}


void
BDeskWindow::Quit()
{
	if (fNavigationItem) {
		// this duplicates BContainerWindow::Quit because
		// fNavigationItem can be part of fTrashContextMenu
		// and would get deleted with it
		BMenu *menu = fNavigationItem->Menu();
		if (menu)
			menu->RemoveItem(fNavigationItem);
		delete fNavigationItem;
		fNavigationItem = 0;
	}

	delete fTrashContextMenu;
	fTrashContextMenu = NULL;

	delete fDeskShelf;
	_inherited::Quit();
}


BPoseView *
BDeskWindow::NewPoseView(Model *model, BRect rect, uint32 viewMode)
{
	return new DesktopPoseView(model, rect, viewMode);
}


void
BDeskWindow::CreatePoseView(Model *model)
{
	fPoseView = NewPoseView(model, Bounds(), kIconMode);
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

	AddChild(fPoseView);

	PoseView()->StartSettingsWatch();
}


void
BDeskWindow::AddWindowContextMenus(BMenu *menu)
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
		new BMessage(B_PASTE), 'V'));
	menu->AddItem(pasteItem);
	menu->AddSeparatorItem();
#endif
	menu->AddItem(new BMenuItem(B_TRANSLATE("Clean up"), new BMessage(kCleanup),
		'K'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Select"B_UTF8_ELLIPSIS),
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
BDeskWindow::MessageReceived(BMessage *message)
{
	if (message->WasDropped()) {
		const rgb_color *color;
		int32 size;
		// handle "roColour"-style color drops
		if (message->FindData("RGBColor", 'RGBC',
			(const void **)&color, &size) == B_OK) {
			BScreen(this).SetDesktopColor(*color);
			fPoseView->SetViewColor(*color);
			fPoseView->SetLowColor(*color);
			return;
		}
	}

	switch (message->what) {
		case B_NODE_MONITOR:
			PRINT(("will update addon shortcuts\n"));
			fShouldUpdateAddonShortcuts = true;
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}

