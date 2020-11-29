/*
 * Copyright 2020-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "LiveMenu.h"

#include <string.h>

#include <Application.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Window.h>

#include "Commands.h"
#include "TrackerSettings.h"


//	#pragma mark - TLiveMixin


TLiveMixin::TLiveMixin(const BContainerWindow* window)
{
	fWindow = window;
}


void
TLiveMixin::UpdateFileMenu(BMenu* menu)
{
	if (menu == NULL)
		return;

	if (menu->Window()->LockLooper()) {
		int32 itemCount = menu->CountItems();
		for (int32 index = 0; index < itemCount; index++) {
			BMenuItem* item = menu->ItemAt(index);
			if (item == NULL || item->Message() == NULL)
				continue;

			switch (item->Message()->what) {
				// Move to Trash/Delete
				case kMoveSelectionToTrash:
				case kDeleteSelection:
					if (!fWindow->Shortcuts()->IsTrash())
						fWindow->Shortcuts()->UpdateMoveToTrashItem(item);
					break;

				// Create link/Create relative link
				case kCreateLink:
				case kCreateRelativeLink:
					fWindow->Shortcuts()->UpdateCreateLinkItem(item);
					break;

				// Cut/Cut more
				case B_CUT:
				case kCutMoreSelectionToClipboard:
					fWindow->Shortcuts()->UpdateCutItem(item);
					break;

				// Copy/Copy more
				case B_COPY:
				case kCopyMoreSelectionToClipboard:
					fWindow->Shortcuts()->UpdateCopyItem(item);
					break;

				// Paste/Paste links
				case B_PASTE:
				case kPasteLinksFromClipboard:
					fWindow->Shortcuts()->UpdatePasteItem(item);
					break;

				// Identify/Force identify
				case kIdentifyEntry:
					fWindow->Shortcuts()->UpdateIdentifyItem(item);
					break;
			}
		}

		menu->Window()->UnlockLooper();
	}
}


void
TLiveMixin::UpdateWindowMenu(BMenu* menu)
{
	if (menu == NULL)
		return;

	// update using the window version of TShortcuts class

	if (menu->Window()->LockLooper()) {
		int32 itemCount = menu->CountItems();
		for (int32 index = 0; index < itemCount; index++) {
			BMenuItem* item = menu->ItemAt(index);
			if (item == NULL || item->Message() == NULL)
				continue;

			switch (item->Message()->what) {
				// Clean up/Clean up all
				case kCleanup:
				case kCleanupAll:
					fWindow->Shortcuts()->UpdateCleanupItem(item);
					break;

				// Open parent
				case kOpenParentDir:
					fWindow->Shortcuts()->UpdateOpenParentItem(item);
					break;

				// Close/Close all
				case B_QUIT_REQUESTED:
				case kCloseAllWindows:
					fWindow->Shortcuts()->UpdateCloseItem(item);
					break;
			}
		}

		menu->Window()->UnlockLooper();
	}
}


//	#pragma mark - TLiveMenu


TLiveMenu::TLiveMenu(const char* label)
	:
	BMenu(label)
{
}


TLiveMenu::~TLiveMenu()
{
}


void
TLiveMenu::MessageReceived(BMessage* message)
{
	if (message != NULL && message->what == B_MODIFIERS_CHANGED)
		Update();
	else
		BMenu::MessageReceived(message);
}


void
TLiveMenu::Update()
{
	// hook method
}


//	#pragma mark - TLivePopUpMenu


TLivePopUpMenu::TLivePopUpMenu(const char* label, bool radioMode, bool labelFromMarked,
	menu_layout layout)
	:
	BPopUpMenu(label, radioMode, labelFromMarked, layout)
{
}


TLivePopUpMenu::~TLivePopUpMenu()
{
}


void
TLivePopUpMenu::MessageReceived(BMessage* message)
{
	if (message != NULL && message->what == B_MODIFIERS_CHANGED)
		Update();
	else
		BMenu::MessageReceived(message);
}


void
TLivePopUpMenu::Update()
{
	// hook method
}


//	#pragma mark - TLiveArrangeByMenu


TLiveArrangeByMenu::TLiveArrangeByMenu(const char* label, const BContainerWindow* window)
	:
	TLiveMenu(label),
	TLiveMixin(window)
{
}


TLiveArrangeByMenu::~TLiveArrangeByMenu()
{
}


void
TLiveArrangeByMenu::Update()
{
	// Clean up/Clean up all
	TShortcuts().UpdateCleanupItem(TShortcuts().FindItem(this, kCleanup, kCleanupAll));
}


//	#pragma mark - TLiveFileMenu


TLiveFileMenu::TLiveFileMenu(const char* label, const BContainerWindow* window)
	:
	TLiveMenu(label),
	TLiveMixin(window)
{
}


TLiveFileMenu::~TLiveFileMenu()
{
}


void
TLiveFileMenu::Update()
{
	UpdateFileMenu(this);
}


//	#pragma mark - TLivePosePopUpMenu


TLivePosePopUpMenu::TLivePosePopUpMenu(const char* label, const BContainerWindow* window,
	bool radioMode, bool labelFromMarked, menu_layout layout)
	:
	TLivePopUpMenu(label, radioMode, labelFromMarked, layout),
	TLiveMixin(window)
{
}


TLivePosePopUpMenu::~TLivePosePopUpMenu()
{
}


void
TLivePosePopUpMenu::Update()
{
	UpdateFileMenu(this);
}


//	#pragma mark - TLiveWindowMenu


TLiveWindowMenu::TLiveWindowMenu(const char* label, const BContainerWindow* window)
	:
	TLiveMenu(label),
	TLiveMixin(window)
{
}


TLiveWindowMenu::~TLiveWindowMenu()
{
}


void
TLiveWindowMenu::Update()
{
	UpdateWindowMenu(this);
}


//	#pragma mark - TLiveWindowPopUpMenu


TLiveWindowPopUpMenu::TLiveWindowPopUpMenu(const char* label, const BContainerWindow* window,
	bool radioMode, bool labelFromMarked, menu_layout layout)
	:
	TLivePopUpMenu(label, radioMode, labelFromMarked, layout),
	TLiveMixin(window)
{
}


TLiveWindowPopUpMenu::~TLiveWindowPopUpMenu()
{
}


void
TLiveWindowPopUpMenu::Update()
{
	UpdateWindowMenu(this);
}
