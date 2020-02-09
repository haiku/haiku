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


#include "InfoWindow.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Catalog.h>
#include <Debug.h>
#include <Directory.h>
#include <File.h>
#include <Font.h>
#include <Locale.h>
#include <MenuField.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StringFormat.h>
#include <SymLink.h>
#include <TabView.h>
#include <TextView.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "Attributes.h"
#include "AttributesView.h"
#include "AutoLock.h"
#include "Commands.h"
#include "DialogPane.h"
#include "FSUtils.h"
#include "GeneralInfoView.h"
#include "IconCache.h"
#include "IconMenuItem.h"
#include "Model.h"
#include "NavMenu.h"
#include "PoseView.h"
#include "StringForSize.h"
#include "Tracker.h"
#include "WidgetAttributeText.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InfoWindow"


const uint32 kNewTargetSelected = 'selc';

//	#pragma mark - BInfoWindow


BInfoWindow::BInfoWindow(Model* model, int32 group_index,
	LockingList<BWindow>* list)
	:
	BWindow(BInfoWindow::InfoWindowRect(),
		"InfoWindow", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS,
		B_CURRENT_WORKSPACE),
	fModel(model),
	fStopCalc(false),
	fIndex(group_index),
	fCalcThreadID(-1),
	fWindowList(list),
	fPermissionsView(NULL),
	fFilePanel(NULL),
	fFilePanelOpen(false)
{
	SetPulseRate(1000000);
		// we use pulse to check freebytes on volume

	TTracker::WatchNode(model->NodeRef(), B_WATCH_ALL | B_WATCH_MOUNT, this);

	// window list is Locked by Tracker around this constructor
	if (list != NULL)
		list->AddItem(this);

	AddShortcut('E', 0, new BMessage(kEditItem));
	AddShortcut('O', 0, new BMessage(kOpenSelection));
	AddShortcut('U', 0, new BMessage(kUnmountVolume));
	AddShortcut('P', 0, new BMessage(kPermissionsSelected));

	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 0);
	SetLayout(layout);

	BModelOpener modelOpener(TargetModel());
	if (TargetModel()->InitCheck() != B_OK)
		return;

	fHeaderView = new HeaderView(TargetModel());
	AddChild(fHeaderView);
	BTabView* tabView = new BTabView("tabs");
	tabView->SetBorder(B_NO_BORDER);
	AddChild(tabView);

	fGeneralInfoView = new GeneralInfoView(TargetModel());
	tabView->AddTab(fGeneralInfoView);

	BRect permissionsBounds(0,
		fGeneralInfoView->Bounds().bottom,
		fGeneralInfoView->Bounds().right,
		fGeneralInfoView->Bounds().bottom + 103);

	fPermissionsView = new FilePermissionsView(
		permissionsBounds, fModel);
	tabView->AddTab(fPermissionsView);

	tabView->AddTab(new AttributesView(TargetModel()));

	// This window accepts messages before being shown, so let's start the
	// looper immediately.
	Run();
}


BInfoWindow::~BInfoWindow()
{
	// Check to make sure the file panel is destroyed
	delete fFilePanel;
	delete fModel;
}


BRect
BInfoWindow::InfoWindowRect()
{
	// starting size of window
	return BRect(70, 50, 385, 240);
}


void
BInfoWindow::Quit()
{
	stop_watching(this);

	if (fWindowList) {
		AutoLock<LockingList<BWindow> > lock(fWindowList);
		fWindowList->RemoveItem(this);
	}

	fStopCalc = true;

	// wait until CalcSize thread has terminated before closing window
	status_t result;
	wait_for_thread(fCalcThreadID, &result);

	_inherited::Quit();
}


bool
BInfoWindow::IsShowing(const node_ref* node) const
{
	return *TargetModel()->NodeRef() == *node;
}


void
BInfoWindow::Show()
{
	if (TargetModel()->InitCheck() != B_OK) {
		Close();
		return;
	}

	AutoLock<BWindow> lock(this);

	// position window appropriately based on index
	BRect windRect(InfoWindowRect());
	if ((fIndex + 2) % 2 == 1) {
		windRect.OffsetBy(320, 0);
		fIndex--;
	}

	windRect.OffsetBy(fIndex * 8, fIndex * 8);

	// make sure window is visible on screen
	BScreen screen(this);
	if (!windRect.Intersects(screen.Frame()))
		windRect.OffsetTo(50, 50);

	MoveTo(windRect.LeftTop());

	// volume case is handled by view
	if (!TargetModel()->IsVolume() && !TargetModel()->IsRoot()) {
		if (TargetModel()->IsDirectory()) {
			// if this is a folder then spawn thread to calculate size
			SetSizeString(B_TRANSLATE("calculating" B_UTF8_ELLIPSIS));
			fCalcThreadID = spawn_thread(BInfoWindow::CalcSize, "CalcSize",
				B_NORMAL_PRIORITY, this);
			resume_thread(fCalcThreadID);
		} else {
			fGeneralInfoView->SetLastSize(TargetModel()->StatBuf()->st_size);

			BString sizeStr;
			GetSizeString(sizeStr, fGeneralInfoView->LastSize(), 0);
			SetSizeString(sizeStr.String());
		}
	}

	BString buffer(B_TRANSLATE_COMMENT("%name info", "InfoWindow Title"));
	buffer.ReplaceFirst("%name", TargetModel()->Name());
	SetTitle(buffer.String());

	lock.Unlock();
	_inherited::Show();
}


void
BInfoWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kRestoreState:
			Show();
			break;

		case kOpenSelection:
		{
			BMessage refsMessage(B_REFS_RECEIVED);
			refsMessage.AddRef("refs", fModel->EntryRef());

			// add a messenger to the launch message that will be used to
			// dispatch scripting calls from apps to the PoseView
			refsMessage.AddMessenger("TrackerViewToken", BMessenger(this));
			be_app->PostMessage(&refsMessage);
			break;
		}

		case kEditItem:
		{
			BEntry entry(fModel->EntryRef());
			if (!fModel->HasLocalizedName()
				&& ConfirmChangeIfWellKnownDirectory(&entry, kRename)) {
				fHeaderView->BeginEditingTitle();
			}
			break;
		}

		case kIdentifyEntry:
		{
			bool force = (modifiers() & B_OPTION_KEY) != 0;
			BEntry entry;
			if (entry.SetTo(fModel->EntryRef(), true) == B_OK) {
				BPath path;
				if (entry.GetPath(&path) == B_OK)
					update_mime_info(path.Path(), true, false, force ? 2 : 1);
			}
			break;
		}

		case kRecalculateSize:
		{
			fStopCalc = true;
			// Wait until any current CalcSize thread has terminated before
			// starting a new one
			status_t result;
			wait_for_thread(fCalcThreadID, &result);

			// Start recalculating..
			fStopCalc = false;
			SetSizeString(B_TRANSLATE("calculating" B_UTF8_ELLIPSIS));
			fCalcThreadID = spawn_thread(BInfoWindow::CalcSize, "CalcSize",
				B_NORMAL_PRIORITY, this);
			resume_thread(fCalcThreadID);
			break;
		}

		case kSetLinkTarget:
			OpenFilePanel(fModel->EntryRef());
			break;

		// An item was dropped into the window
		case B_SIMPLE_DATA:
			// If we are not a SymLink, just ignore the request
			if (!fModel->IsSymLink())
				break;
			// supposed to fall through
		// An item was selected from the file panel
		// fall-through
		case kNewTargetSelected:
		{
			// Extract the BEntry, and set its full path to the string value
			BEntry targetEntry;
			entry_ref ref;
			BPath path;

			if (message->FindRef("refs", &ref) == B_OK
				&& targetEntry.SetTo(&ref, true) == B_OK
				&& targetEntry.Exists()) {
				// We now have to re-target the broken symlink. Unfortunately,
				// there's no way to change the target of an existing symlink.
				// So we have to delete the old one and create a new one.
				// First, stop watching the broken node
				// (we don't want this window to quit when the node
				// is removed.)
				stop_watching(this);

				// Get the parent
				BDirectory parent;
				BEntry tmpEntry(TargetModel()->EntryRef());
				if (tmpEntry.GetParent(&parent) != B_OK)
					break;

				// Preserve the name
				BString name(TargetModel()->Name());

				// Extract path for new target
				BEntry target(&ref);
				BPath targetPath;
				if (target.GetPath(&targetPath) != B_OK)
					break;

				// Preserve the original attributes
				AttributeStreamMemoryNode memoryNode;
				{
					BModelOpener opener(TargetModel());
					AttributeStreamFileNode original(TargetModel()->Node());
					memoryNode << original;
				}

				// Delete the broken node.
				BEntry oldEntry(TargetModel()->EntryRef());
				oldEntry.Remove();

				// Create new node
				BSymLink link;
				parent.CreateSymLink(name.String(), targetPath.Path(), &link);

				// Update our Model()
				BEntry symEntry(&parent, name.String());
				fModel->SetTo(&symEntry);

				BModelWriteOpener opener(TargetModel());

				// Copy the attributes back
				AttributeStreamFileNode newNode(TargetModel()->Node());
				newNode << memoryNode;

				// Start watching this again
				TTracker::WatchNode(TargetModel()->NodeRef(),
					B_WATCH_ALL | B_WATCH_MOUNT, this);

				// Tell the attribute view about this new model
				fGeneralInfoView->ReLinkTargetModel(TargetModel());
				fHeaderView->ReLinkTargetModel(TargetModel());
			}
			break;
		}

		case B_CANCEL:
			// File panel window has closed
			delete fFilePanel;
			fFilePanel = NULL;
			// It's no longer open
			fFilePanelOpen = false;
			break;

		case kUnmountVolume:
			// Sanity check that this isn't the boot volume
			// (The unmount menu item has been disabled in this
			// case, but the shortcut is still active)
			if (fModel->IsVolume()) {
				BVolume boot;
				BVolumeRoster().GetBootVolume(&boot);
				BVolume volume(fModel->NodeRef()->device);
				if (volume != boot) {
					TTracker* tracker = dynamic_cast<TTracker*>(be_app);
					if (tracker != NULL)
						tracker->SaveAllPoseLocations();

					BMessage unmountMessage(kUnmountVolume);
					unmountMessage.AddInt32("device_id", volume.Device());
					be_app->PostMessage(&unmountMessage);
				}
			}
			break;

		case kEmptyTrash:
			FSEmptyTrash();
			break;

		case B_NODE_MONITOR:
			switch (message->FindInt32("opcode")) {
				case B_ENTRY_REMOVED:
				{
					node_ref itemNode;
					message->FindInt32("device", &itemNode.device);
					message->FindInt64("node", &itemNode.node);
					// our window itself may be deleted
					if (*TargetModel()->NodeRef() == itemNode)
						Close();
					break;
				}

				case B_ENTRY_MOVED:
				case B_STAT_CHANGED:
				case B_ATTR_CHANGED:
					fGeneralInfoView->ModelChanged(TargetModel(), message);
						// must be called before the
						// FilePermissionView::ModelChanged()
						// call, because it changes the model...
						// (bad style!)
					fHeaderView->ModelChanged(TargetModel(), message);

					if (fPermissionsView != NULL)
						fPermissionsView->ModelChanged(TargetModel());
					break;

				case B_DEVICE_UNMOUNTED:
				{
					// We were watching a volume that is no longer
					// mounted, we might as well quit
					node_ref itemNode;
					// Only the device information is available
					message->FindInt32("device", &itemNode.device);
					if (TargetModel()->NodeRef()->device == itemNode.device)
						Close();
					break;
				}

				default:
					break;
			}
			break;

		case kPermissionsSelected:
		{
			BTabView* tabView = (BTabView*)FindView("tabs");
			tabView->Select(1);	
			break;
		}

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
BInfoWindow::GetSizeString(BString& result, off_t size, int32 fileCount)
{
	static BStringFormat sizeFormat(B_TRANSLATE(
		"{0, plural, one{(# byte)} other{(# bytes)}}"));
	static BStringFormat countFormat(B_TRANSLATE(
		"{0, plural, one{for # file} other{for # files}}"));

	char sizeBuffer[128];
	result << string_for_size((double)size, sizeBuffer, sizeof(sizeBuffer));

	if (size >= kKBSize) {
		result << " ";

		sizeFormat.Format(result, size);
			// "bytes" translation could come from string_for_size
			// which could be part of the localekit itself
	}

	if (fileCount != 0) {
		result << " ";
		countFormat.Format(result, fileCount);
	}
}


int32
BInfoWindow::CalcSize(void* castToWindow)
{
	BInfoWindow* window = static_cast<BInfoWindow*>(castToWindow);
	BDirectory dir(window->TargetModel()->EntryRef());
	BDirectory trashDir;
	FSGetTrashDir(&trashDir, window->TargetModel()->EntryRef()->device);
	if (dir.InitCheck() != B_OK) {
		if (window->StopCalc())
			return B_ERROR;

		AutoLock<BWindow> lock(window);
		if (!lock)
			return B_ERROR;

		window->SetSizeString(B_TRANSLATE("Error calculating folder size."));
		return B_ERROR;
	}

	BEntry dirEntry, trashEntry;
	dir.GetEntry(&dirEntry);
	trashDir.GetEntry(&trashEntry);

	BString sizeString;

	// check if user has asked for trash dir info
	if (dirEntry != trashEntry) {
		// if not, perform normal info calculations
		off_t size = 0;
		int32 fileCount = 0;
		int32 dirCount = 0;
		CopyLoopControl loopControl;
		FSRecursiveCalcSize(window, &loopControl, &dir, &size, &fileCount,
			&dirCount);

		// got the size value, update the size string
		GetSizeString(sizeString, size, fileCount);
	} else {
		// in the trash case, iterate through and sum up
		// size/counts for all present trash dirs
		off_t totalSize = 0, currentSize;
		int32 totalFileCount = 0, currentFileCount;
		int32 totalDirCount = 0, currentDirCount;
		BVolumeRoster volRoster;
		volRoster.Rewind();
		BVolume volume;
		while (volRoster.GetNextVolume(&volume) == B_OK) {
			if (!volume.IsPersistent())
				continue;

			currentSize = 0;
			currentFileCount = 0;
			currentDirCount = 0;

			BDirectory trashDir;
			if (FSGetTrashDir(&trashDir, volume.Device()) == B_OK) {
				CopyLoopControl loopControl;
				FSRecursiveCalcSize(window, &loopControl, &trashDir,
					&currentSize, &currentFileCount, &currentDirCount);
				totalSize += currentSize;
				totalFileCount += currentFileCount;
				totalDirCount += currentDirCount;
			}
		}
		GetSizeString(sizeString, totalSize, totalFileCount);
	}

	if (window->StopCalc()) {
		// window closed, bail
		return B_OK;
	}

	AutoLock<BWindow> lock(window);
	if (lock.IsLocked())
		window->SetSizeString(sizeString.String());

	return B_OK;
}


void
BInfoWindow::SetSizeString(const char* sizeString)
{
	fGeneralInfoView->SetSizeString(sizeString);
}


void
BInfoWindow::OpenFilePanel(const entry_ref* ref)
{
	// Open a file dialog box to allow the user to select a new target
	// for the sym link
	if (fFilePanel == NULL) {
		BMessenger runner(this);
		BMessage message(kNewTargetSelected);
		fFilePanel = new BFilePanel(B_OPEN_PANEL, &runner, ref,
			B_FILE_NODE | B_SYMLINK_NODE | B_DIRECTORY_NODE,
			false, &message);

		if (fFilePanel != NULL) {
			fFilePanel->SetButtonLabel(B_DEFAULT_BUTTON,
				B_TRANSLATE("Select"));
			fFilePanel->Window()->ResizeTo(500, 300);
			BString title(B_TRANSLATE_COMMENT("Link \"%name\" to:",
				"File dialog title for new sym link"));
			title.ReplaceFirst("%name", fModel->Name());
			fFilePanel->Window()->SetTitle(title.String());
			fFilePanel->Show();
			fFilePanelOpen = true;
		}
	} else if (!fFilePanelOpen) {
		fFilePanel->Show();
		fFilePanelOpen = true;
	} else {
		fFilePanelOpen = true;
		fFilePanel->Window()->Activate(true);
	}
}


