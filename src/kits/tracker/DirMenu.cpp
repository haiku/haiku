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

// ToDo:
// get rid of fMenuBar, SetMenuBar and related mess


#include <Catalog.h>
#include <Debug.h>
#include <Directory.h>
#include <Locale.h>
#include <MenuBar.h>
#include <Path.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "Attributes.h"
#include "ContainerWindow.h"
#include "DirMenu.h"
#include "FSUtils.h"
#include "IconMenuItem.h"
#include "NavMenu.h"
#include "TrackerSettings.h"
#include "Utilities.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DirMenu"


//	#pragma mark - BDirMenu


BDirMenu::BDirMenu(BMenuBar* bar, BMessenger target, uint32 command,
	const char* entryName)
	:
	BPopUpMenu("directories"),
	fTarget(target),
	fMenuBar(bar),
	fCommand(command)
{
	SetFont(be_plain_font);
	if (entryName)
		fEntryName = entryName;
	else
		fEntryName = "refs";
}


BDirMenu::~BDirMenu()
{
}


void
BDirMenu::Populate(const BEntry* startEntry, BWindow* originatingWindow,
	bool includeStartEntry, bool select, bool reverse, bool addShortcuts,
	bool navMenuEntries)
{
	try {
		if (!startEntry)
			throw (status_t)B_ERROR;

		Model model(startEntry);
		ThrowOnInitCheckError(&model);

		ModelMenuItem* menu = NULL;

		if (fMenuBar) {
			menu = new ModelMenuItem(&model, this, true, true);
			fMenuBar->AddItem(menu);
		}

		BEntry entry(*startEntry);

		bool showDesktop, showDisksIcon;
		{
			TrackerSettings settings;
			showDesktop = settings.DesktopFilePanelRoot();
			showDisksIcon = settings.ShowDisksIcon();
		}

		// might start one level above startEntry
		if (!includeStartEntry) {
			BDirectory parent;
			BDirectory dir(&entry);

			if (!showDesktop && dir.InitCheck() == B_OK
				&& dir.IsRootDirectory()) {
				// if we're at the root directory skip "mnt" and
				// go straight to "/"
				parent.SetTo("/");
				parent.GetEntry(&entry);
			} else
				FSGetParentVirtualDirectoryAware(entry, entry);
		}

		BDirectory desktopDir;
		FSGetDeskDir(&desktopDir);
		BEntry desktopEntry;
		desktopDir.GetEntry(&desktopEntry);

		for (;;) {
			BNode node(&entry);
			ThrowOnInitCheckError(&node);

			PoseInfo info;
			ReadAttrResult result = ReadAttr(&node, kAttrPoseInfo,
				kAttrPoseInfoForeign, B_RAW_TYPE, 0, &info, sizeof(PoseInfo),
				&PoseInfo::EndianSwap);

			BEntry parentEntry;
			bool hitRoot = false;

			BDirectory dir(&entry);
			if (!showDesktop && dir.InitCheck() == B_OK
				&& dir.IsRootDirectory()) {
				// if we're at the root directory skip "mnt" and
				// go straight to "/"
				hitRoot = true;
				parentEntry.SetTo("/");
			} else
				FSGetParentVirtualDirectoryAware(entry, parentEntry);

			if (showDesktop) {
				BEntry root("/");
				// warp from "/" to Desktop properly
				if (entry == root) {
					if (showDisksIcon)
						AddDisksIconToMenu(reverse);
					entry = desktopEntry;
				}

				if (entry == desktopEntry)
					hitRoot = true;
			}

			if (result == kReadAttrFailed || !info.fInvisible
				|| (showDesktop && desktopEntry == entry)) {
				AddItemToDirMenu(&entry, originatingWindow, reverse,
					addShortcuts, navMenuEntries);
			}

			if (hitRoot) {
				if (!showDesktop && showDisksIcon && *startEntry != "/")
					AddDisksIconToMenu(reverse);
				break;
			}

			entry = parentEntry;
			if (entry.InitCheck() != B_OK)
				break;
		}

		// select last item in menu
		if (!select)
			return;

		ModelMenuItem* item
			= dynamic_cast<ModelMenuItem*>(ItemAt(CountItems() - 1));
		if (item != NULL) {
			item->SetMarked(true);
			if (menu) {
				entry.SetTo(item->TargetModel()->EntryRef());
				ThrowOnError(menu->SetEntry(&entry));
			}
		}
	} catch (status_t err) {
		PRINT(("BDirMenu::Populate: caught error %s\n", strerror(err)));
		if (!CountItems()) {
			BString error;
			error << "Error [" << strerror(err) << "] populating menu";
			AddItem(new BMenuItem(error.String(), 0));
		}
	}
}


void
BDirMenu::AddItemToDirMenu(const BEntry* entry, BWindow* originatingWindow,
	bool atEnd, bool addShortcuts, bool navMenuEntries)
{
	Model model(entry);
	if (model.InitCheck() != B_OK)
		return;

	BMessage* message = new BMessage(fCommand);
	message->AddRef(fEntryName.String(), model.EntryRef());

	// add reference to the container windows model so that we can
	// close the window if
	BContainerWindow* window = originatingWindow ?
		dynamic_cast<BContainerWindow*>(originatingWindow) : 0;
	if (window != NULL) {
		message->AddData("nodeRefsToClose", B_RAW_TYPE,
			window->TargetModel()->NodeRef(), sizeof (node_ref));
	}
	ModelMenuItem* item;
	if (navMenuEntries) {
		BNavMenu* subMenu = new BNavMenu(model.Name(), B_REFS_RECEIVED,
			fTarget, window);
		entry_ref ref;
		entry->GetRef(&ref);
		subMenu->SetNavDir(&ref);
		item = new ModelMenuItem(&model, subMenu);
		item->SetLabel(model.Name());
		item->SetMessage(message);
	} else {
		item = new ModelMenuItem(&model, model.Name(), message);
	}

	if (addShortcuts) {
		if (model.IsDesktop())
			item->SetShortcut('D', B_COMMAND_KEY);
		else if (FSIsHomeDir(entry))
			item->SetShortcut('H', B_COMMAND_KEY);
	}

	if (atEnd)
		AddItem(item);
	else
		AddItem(item, 0);

	item->SetTarget(fTarget);

	if (fMenuBar != NULL) {
		ModelMenuItem* menu
			= dynamic_cast<ModelMenuItem*>(fMenuBar->ItemAt(0));
		if (menu != NULL) {
			ThrowOnError(menu->SetEntry(entry));
			item->SetMarked(true);
		}
	}
}


void
BDirMenu::AddDisksIconToMenu(bool atEnd)
{
	BEntry entry("/");
	Model model(&entry);
	if (model.InitCheck() != B_OK)
		return;

	BMessage* message = new BMessage(fCommand);
	message->AddRef(fEntryName.String(), model.EntryRef());

	ModelMenuItem* item = new ModelMenuItem(&model,
		B_TRANSLATE(B_DISKS_DIR_NAME), message);
	if (atEnd)
		AddItem(item);
	else
		AddItem(item, 0);
}
