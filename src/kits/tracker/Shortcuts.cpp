/*
 * Copyright 2020-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "Shortcuts.h"

#include <Application.h>
#include <Catalog.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>

#include "Commands.h"
#include "FSClipboard.h"
#include "FSUtils.h"
#include "Model.h"
#include "Pose.h"
#include "Tracker.h"
#include "TrackerSettings.h"
#include "Utilities.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ContainerWindow"


TShortcuts::TShortcuts()
	:
	fContainerWindow(NULL),
	fInWindow(false)
{
	// the dumb version: for adding and limited updates
}


TShortcuts::TShortcuts(BContainerWindow* window)
	:
	fContainerWindow(window),
	fInWindow(window != NULL)
{
	// the smart version: update methods work
}


//	#pragma mark - Shortcuts build methods


BMenuItem*
TShortcuts::AddOnsItem()
{
	return new BMenuItem(AddOnsLabel(), NULL);
}


const char*
TShortcuts::AddOnsLabel()
{
	return B_TRANSLATE("Add-ons");
}


BMenuItem*
TShortcuts::AddPrinterItem()
{
	return new BMenuItem(AddPrinterLabel(), new BMessage(kAddPrinter));
}


const char*
TShortcuts::AddPrinterLabel()
{
	return B_TRANSLATE("Add printer" B_UTF8_ELLIPSIS);
}


BMenuItem*
TShortcuts::ArrangeByItem()
{
	return new BMenuItem(ArrangeByLabel(), NULL);
}


const char*
TShortcuts::ArrangeByLabel()
{
	return B_TRANSLATE("Arrange by");
}


BMenuItem*
TShortcuts::CleanupItem()
{
	return new BMenuItem(CleanupLabel(), new BMessage(kCleanup), 'K');
}


const char*
TShortcuts::CleanupLabel()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return B_TRANSLATE("Clean up all");
	else
		return B_TRANSLATE("Clean up");
}


int32
TShortcuts::CleanupCommand()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return kCleanupAll;
	else
		return kCleanup;
}


BMenuItem*
TShortcuts::CloseItem()
{
	return new BMenuItem(CloseLabel(), new BMessage(B_QUIT_REQUESTED), 'W');
}


const char*
TShortcuts::CloseLabel()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return B_TRANSLATE("Close all");
	else
		return B_TRANSLATE("Close");
}


int32
TShortcuts::CloseCommand()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return kCloseAllWindows;
	else
		return B_QUIT_REQUESTED;
}


BMenuItem*
TShortcuts::CloseAllInWorkspaceItem()
{
	return new BMenuItem(CloseAllInWorkspaceLabel(), new BMessage(kCloseAllInWorkspace), 'Q');
}


const char*
TShortcuts::CloseAllInWorkspaceLabel()
{
	return B_TRANSLATE("Close all in workspace");
}


BMenuItem*
TShortcuts::CopyItem()
{
	return new BMenuItem(CopyLabel(), new BMessage(B_COPY), 'C');
}


const char*
TShortcuts::CopyLabel()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return B_TRANSLATE("Copy more");
	else
		return B_TRANSLATE("Copy");
}


int32
TShortcuts::CopyCommand()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return kCopyMoreSelectionToClipboard;
	else
		return B_COPY;
}


BMenuItem*
TShortcuts::CopyToItem()
{
	return new BMenuItem(CopyToLabel(), new BMessage(kCopySelectionTo));
}


BMenuItem*
TShortcuts::CopyToItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kCopySelectionTo));
}


const char*
TShortcuts::CopyToLabel()
{
	return B_TRANSLATE("Copy to");
}


BMenuItem*
TShortcuts::CreateLinkItem()
{
	return new BMenuItem(CreateLinkLabel(), new BMessage(kCreateLink));
}


BMenuItem*
TShortcuts::CreateLinkItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kCreateLink));
}


const char*
TShortcuts::CreateLinkLabel()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return B_TRANSLATE("Create relative link");
	else
		return B_TRANSLATE("Create link");
}


int32
TShortcuts::CreateLinkCommand()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return kCreateRelativeLink;
	else
		return kCreateLink;
}


BMenuItem*
TShortcuts::CreateLinkHereItem()
{
	return new BMenuItem(CreateLinkHereLabel(), new BMessage(kCreateLink));
}


const char*
TShortcuts::CreateLinkHereLabel()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return B_TRANSLATE("Create relative link here");
	else
		return B_TRANSLATE("Create link here");
}


int32
TShortcuts::CreateLinkHereCommand()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return kCreateRelativeLink;
	else
		return kCreateLink;
}


BMenuItem*
TShortcuts::CutItem()
{
	return new BMenuItem(CutLabel(), new BMessage(B_CUT), 'X');
}


const char*
TShortcuts::CutLabel()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return B_TRANSLATE("Cut more");
	else
		return B_TRANSLATE("Cut");
}


int32
TShortcuts::CutCommand()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return kCutMoreSelectionToClipboard;
	else
		return B_CUT;
}


BMenuItem*
TShortcuts::DeleteItem()
{
	return new BMenuItem(DeleteLabel(), new BMessage(kDeleteSelection));
}


const char*
TShortcuts::DeleteLabel()
{
	return B_TRANSLATE("Delete");
}


BMenuItem*
TShortcuts::DuplicateItem()
{
	return new BMenuItem(DuplicateLabel(), new BMessage(kDuplicateSelection), 'D');
}


const char*
TShortcuts::DuplicateLabel()
{
	return B_TRANSLATE("Duplicate");
}


BMenuItem*
TShortcuts::EditNameItem()
{
	return new BMenuItem(EditNameLabel(), new BMessage(kEditName), 'E');
}


const char*
TShortcuts::EditNameLabel()
{
	return B_TRANSLATE("Edit name");
}


BMenuItem*
TShortcuts::EditQueryItem()
{
	return new BMenuItem(EditQueryLabel(), new BMessage(kEditQuery), 'G');
}


const char*
TShortcuts::EditQueryLabel()
{
	return B_TRANSLATE("Edit query");
}


BMenuItem*
TShortcuts::EmptyTrashItem()
{
	return new BMenuItem(EmptyTrashLabel(), new BMessage(kEmptyTrash));
}


const char*
TShortcuts::EmptyTrashLabel()
{
	return B_TRANSLATE("Empty Trash");
}


BMenuItem*
TShortcuts::FindItem()
{
	return new BMenuItem(FindLabel(), new BMessage(kFindButton), 'F');
}


const char*
TShortcuts::FindLabel()
{
	return B_TRANSLATE("Find" B_UTF8_ELLIPSIS);
}


BMenuItem*
TShortcuts::GetInfoItem()
{
	return new BMenuItem(GetInfoLabel(), new BMessage(kGetInfo), 'I');
}


const char*
TShortcuts::GetInfoLabel()
{
	return B_TRANSLATE("Get info");
}


BMenuItem*
TShortcuts::IdentifyItem()
{
	BMessage* message = new BMessage(kIdentifyEntry);
	BMenuItem* item = new BMenuItem(IdentifyLabel(), message);
	message->AddBool("force", (modifiers() & B_SHIFT_KEY) != 0);

	if (fInWindow)
		item->SetTarget(PoseView());

	return item;
}


const char*
TShortcuts::IdentifyLabel()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return B_TRANSLATE("Force identify");
	else
		return B_TRANSLATE("Identify");
}


BMenuItem*
TShortcuts::InvertSelectionItem()
{
	return new BMenuItem(InvertSelectionLabel(), new BMessage(kInvertSelection), 'S');
}


const char*
TShortcuts::InvertSelectionLabel()
{
	return B_TRANSLATE("Invert selection");
}


BMenuItem*
TShortcuts::MakeActivePrinterItem()
{
	return new BMenuItem(MakeActivePrinterLabel(), new BMessage(kMakeActivePrinter));
}


const char*
TShortcuts::MakeActivePrinterLabel()
{
	return B_TRANSLATE("Make active printer");
}


BMenuItem*
TShortcuts::MountItem()
{
	return new BMenuItem(MountLabel(), new BMessage(kMountVolume));
}


BMenuItem*
TShortcuts::MountItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kMountVolume));
}


const char*
TShortcuts::MountLabel()
{
	return B_TRANSLATE("Mount");
}


BMenuItem*
TShortcuts::MoveToItem()
{
	return new BMenuItem(MoveToLabel(), new BMessage(kMoveSelectionTo));
}


BMenuItem*
TShortcuts::MoveToItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kMoveSelectionTo));
}


const char*
TShortcuts::MoveToLabel()
{
	return B_TRANSLATE("Move to");
}


BMenuItem*
TShortcuts::MoveToTrashItem()
{
	return new BMenuItem(MoveToTrashLabel(), new BMessage(kMoveSelectionToTrash), B_DELETE,
		B_NO_COMMAND_KEY);
}


const char*
TShortcuts::MoveToTrashLabel()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return B_TRANSLATE("Delete");
	else
		return B_TRANSLATE("Move to Trash");
}


int32
TShortcuts::MoveToTrashCommand()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return kDeleteSelection;
	else
		return kMoveSelectionToTrash;
}


BMenuItem*
TShortcuts::NewFolderItem()
{
	return new BMenuItem(NewFolderLabel(), new BMessage(kNewFolder), 'N');
}


const char*
TShortcuts::NewFolderLabel()
{
	return B_TRANSLATE("New folder");
}


BMenuItem*
TShortcuts::NewTemplatesItem()
{
	return new BMenuItem(B_TRANSLATE("New"), new BMessage(kNewEntryFromTemplate));
}


BMenuItem*
TShortcuts::NewTemplatesItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kNewEntryFromTemplate));
}


const char*
TShortcuts::NewTemplatesLabel()
{
	return B_TRANSLATE("New");
}


BMenuItem*
TShortcuts::OpenItem()
{
	return new BMenuItem(OpenLabel(), new BMessage(kOpenSelection), 'O');
}


const char*
TShortcuts::OpenLabel()
{
	return B_TRANSLATE("Open");
}


BMenuItem*
TShortcuts::OpenParentItem()
{
	return new BMenuItem(OpenParentLabel(), new BMessage(kOpenParentDir), B_UP_ARROW);
}


const char*
TShortcuts::OpenParentLabel()
{
	return B_TRANSLATE("Open parent");
}


BMenuItem*
TShortcuts::OpenWithItem()
{
	return new BMenuItem(OpenWithLabel(), new BMessage(kOpenSelectionWith));
}


BMenuItem*
TShortcuts::OpenWithItem(BMenu* menu)
{
	return new BMenuItem(menu, new BMessage(kOpenSelectionWith));
}


const char*
TShortcuts::OpenWithLabel()
{
	return B_TRANSLATE("Open with" B_UTF8_ELLIPSIS);
}


BMenuItem*
TShortcuts::PasteItem()
{
	return new BMenuItem(PasteLabel(), new BMessage(B_PASTE), 'V');
}


const char*
TShortcuts::PasteLabel()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return B_TRANSLATE("Paste links");
	else
		return B_TRANSLATE("Paste");
}


int32
TShortcuts::PasteCommand()
{
	if ((modifiers() & B_SHIFT_KEY) != 0)
		return kPasteLinksFromClipboard;
	else
		return B_PASTE;
}


BMenuItem*
TShortcuts::ResizeToFitItem()
{
	return new BMenuItem(ResizeToFitLabel(), new BMessage(kResizeToFit), 'Y');
}


const char*
TShortcuts::ResizeToFitLabel()
{
	return B_TRANSLATE("Resize to fit");
}


BMenuItem*
TShortcuts::RestoreItem()
{
	return new BMenuItem(RestoreLabel(), new BMessage(kRestoreSelectionFromTrash));
}


const char*
TShortcuts::RestoreLabel()
{
	return B_TRANSLATE("Restore");
}


BMenuItem*
TShortcuts::ReverseOrderItem()
{
	return new BMenuItem(ReverseOrderLabel(), new BMessage(kArrangeReverseOrder));
}


const char*
TShortcuts::ReverseOrderLabel()
{
	return B_TRANSLATE("Reverse order");
}


BMenuItem*
TShortcuts::SelectItem()
{
	return new BMenuItem(SelectLabel(), new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY);
}


const char*
TShortcuts::SelectLabel()
{
	return B_TRANSLATE("Select" B_UTF8_ELLIPSIS);
}


BMenuItem*
TShortcuts::SelectAllItem()
{
	return new BMenuItem(SelectAllLabel(), new BMessage(B_SELECT_ALL), 'A');
}


const char*
TShortcuts::SelectAllLabel()
{
	return B_TRANSLATE("Select all");
}


BMenuItem*
TShortcuts::UnmountItem()
{
	return new BMenuItem(UnmountLabel(), new BMessage(kUnmountVolume), 'U');
}


const char*
TShortcuts::UnmountLabel()
{
	return B_TRANSLATE("Unmount");
}


//	#pragma mark - Shortcuts update methods


void
TShortcuts::Update(BMenu* menu)
{
	if (menu == NULL)
		return;

	int32 itemCount = menu->CountItems();
	for (int32 index = 0; index < itemCount; index++) {
		BMenuItem* item = menu->ItemAt(index);
		if (item == NULL || item->Message() == NULL)
			continue;

		switch (item->Message()->what) {
			case kAddPrinter:
				UpdateAddPrinterItem(item);
				break;

			case kCleanup:
			case kCleanupAll:
				UpdateCleanupItem(item);
				break;

			case B_COPY:
			case kCopyMoreSelectionToClipboard:
				UpdateCopyItem(item);
				break;

			case kCopySelectionTo:
				UpdateCopyToItem(item);
				break;

			case kCreateLink:
			case kCreateRelativeLink:
				UpdateCreateLinkItem(item);
				break;

			case B_CUT:
			case kCutMoreSelectionToClipboard:
				UpdateCutItem(item);
				break;

			case kDeleteSelection:
				// delete command used by a different item in Trash
				if (IsTrash() || InTrash())
					UpdateDeleteItem(item);
				else
					UpdateMoveToTrashItem(item);
				break;

			case kDuplicateSelection:
				UpdateDuplicateItem(item);
				break;

			case kEditName:
				UpdateEditNameItem(item);
				break;

			case kEditQuery:
				UpdateEditQueryItem(item);
				break;

			case kEmptyTrash:
				UpdateEmptyTrashItem(item);
				break;

			case kFindButton:
				UpdateFindItem(item);
				break;

			case kGetInfo:
				UpdateGetInfoItem(item);
				break;

			case kIdentifyEntry:
				UpdateIdentifyItem(item);
				break;

			case kInvertSelection:
				UpdateInvertSelectionItem(item);
				break;

			case kMakeActivePrinter:
				UpdateMakeActivePrinterItem(item);
				break;

			case kMoveSelectionTo:
				UpdateMoveToItem(item);
				break;

			case kMoveSelectionToTrash:
				UpdateMoveToTrashItem(item);
				break;

			case kNewFolder:
				UpdateNewFolderItem(item);
				break;

			case kNewEntryFromTemplate:
				UpdateNewTemplatesItem(item);
				break;

			case kOpenSelection:
				UpdateOpenItem(item);
				break;

			case kOpenParentDir:
				UpdateOpenParentItem(item);
				break;

			case B_PASTE:
			case kPasteLinksFromClipboard:
				UpdatePasteItem(item);
				break;

			case kResizeToFit:
				UpdateResizeToFitItem(item);
				break;

			case kRestoreSelectionFromTrash:
				UpdateRestoreItem(item);
				break;

			case kArrangeReverseOrder:
				UpdateReverseOrderItem(item);
				break;

			case B_SELECT_ALL:
				UpdateSelectAllItem(item);
				break;

			case kShowSelectionWindow:
				UpdateSelectItem(item);
				break;

			case B_QUIT_REQUESTED:
			case kCloseAllWindows:
				UpdateCloseItem(item);
				break;

			case kCloseAllInWorkspace:
				UpdateCloseAllInWorkspaceItem(item);
				break;

			case kUnmountVolume:
				UpdateUnmountItem(item);
				break;

			default:
				break;
		}
	}
}


void
TShortcuts::UpdateAddOnsItem(BMenuItem* item)
{
	if (item == NULL || item->Submenu() == NULL)
		return;

	item->SetEnabled(item->Submenu()->CountItems() > 0);

	if (fInWindow)
		item->Submenu()->SetTargetForItems(fContainerWindow);
}


void
TShortcuts::UpdateAddPrinterItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetEnabled(true);
	item->SetTarget(be_app);
}


void
TShortcuts::UpdateArrangeByItem(BMenuItem* item)
{
	if (item == NULL || item->Submenu() == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(PoseView()->ViewMode() != kListMode);
		item->Submenu()->SetTargetForItems(PoseView());
	}
}


void
TShortcuts::UpdateCleanupItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CleanupLabel());
	item->Message()->what = CleanupCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateCloseAllInWorkspaceItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetEnabled(true);
	item->SetTarget(be_app);
}


void
TShortcuts::UpdateCloseItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CloseLabel());
	item->Message()->what = CloseCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));

	if (fInWindow) {
		if ((modifiers() & B_SHIFT_KEY) != 0)
			item->SetTarget(be_app);
		else
			item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateCopyItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CopyLabel());
	item->Message()->what = CopyCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));

	if (fInWindow) {
		item->SetEnabled(IsCurrentFocusOnTextView() || HasSelection());
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateCopyToItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateCreateLinkItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CreateLinkLabel());
	item->Message()->what = CreateLinkCommand();

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateCreateLinkHereItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CreateLinkHereLabel());
	item->Message()->what = CreateLinkHereCommand();

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateCutItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(CutLabel());
	item->Message()->what = CutCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));

	if (fInWindow) {
		if (IsCurrentFocusOnTextView())
			item->SetEnabled(true);
		else if (IsRoot() || IsTrash() || IsVirtualDirectory())
			item->SetEnabled(false);
		else
			item->SetEnabled(HasSelection() && TargetIsReadOnly() == false);

		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateDeleteItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(B_TRANSLATE("Delete"));
	item->Message()->what = kDeleteSelection;
	if (item->Shortcut() != 0)
		item->SetShortcut(item->Shortcut(), B_NO_COMMAND_KEY);

	if (fInWindow) {
		item->SetEnabled(HasSelection() && !SelectionIsReadOnly());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateDuplicateItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(PoseView()->CanMoveToTrashOrDuplicate());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateEditNameItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(PoseView()->CanEditName());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateEditQueryItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateEmptyTrashItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(static_cast<TTracker*>(be_app)->TrashFull());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateFindItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateGetInfoItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateIdentifyItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(IdentifyLabel());
	item->Message()->ReplaceBool("force", (modifiers() & B_SHIFT_KEY) != 0);

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateInvertSelectionItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateMakeActivePrinterItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(PoseView()->CountSelected() == 1);
		item->SetTarget(be_app);
	}
}


void
TShortcuts::UpdateMoveToItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(HasSelection() && !SelectionIsReadOnly());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateMoveToTrashItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (item->Shortcut() == 0) {
		// make sure this is the real Move to Trash item, not the Delete item in Trash
		item->SetLabel(B_TRANSLATE("Delete"));
		item->Message()->what = kDeleteSelection;
	} else {
		item->SetLabel(MoveToTrashLabel());
		item->Message()->what = MoveToTrashCommand();
		item->SetShortcut(item->Shortcut(), B_NO_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));
	}

	if (fInWindow) {
		item->SetEnabled(HasSelection() && !SelectionIsReadOnly());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateNewFolderItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(!(IsRoot() || IsTrash() || InTrash()) && TargetIsReadOnly() == false);
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateNewTemplatesItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(TargetIsReadOnly() == false);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateOpenItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateOpenParentItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(PoseView()->CanOpenParent());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateOpenWithItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetShortcut('O', B_COMMAND_KEY | B_CONTROL_KEY);

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdatePasteItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(PasteLabel());
	item->Message()->what = PasteCommand();
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY | (modifiers() & B_SHIFT_KEY));

	if (fInWindow) {
		bool isPastable = FSClipboardHasRefs() && !SelectionIsReadOnly() && !IsTrash();
		item->SetEnabled(IsCurrentFocusOnTextView() || isPastable);

		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateResizeToFitItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateRestoreItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(HasSelection());
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateReverseOrderItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateSelectItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(PoseView());
	}
}


void
TShortcuts::UpdateSelectAllItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	if (fInWindow) {
		item->SetEnabled(true);
		item->SetTarget(fContainerWindow);
	}
}


void
TShortcuts::UpdateUnmountItem(BMenuItem* item)
{
	if (item == NULL)
		return;

	item->SetLabel(UnmountLabel());
	item->Message()->what = kUnmountVolume;
	item->SetShortcut(item->Shortcut(), B_COMMAND_KEY);

	if (fInWindow) {
		item->SetEnabled(PoseView()->CanUnmountSelection());
		item->SetTarget(PoseView());
	}
}


//	#pragma mark - Shortcuts convenience methods


BMenuItem*
TShortcuts::FindItem(BMenu* menu, int32 command1, int32 command2)
{
	// find menu item by either of a pair of commands
	BMenuItem* item1 = menu->FindItem(command1);
	BMenuItem* item2 = menu->FindItem(command2);
	if (item1 == NULL && item2 == NULL)
		return NULL;

	return item1 != NULL ? item1 : item2;
}


bool
TShortcuts::IsCurrentFocusOnTextView() const
{
	// used to redirect cut/copy/paste and other text-based shortcuts
	if (!fInWindow)
		return false;

	BWindow* window = fContainerWindow;
	return dynamic_cast<BTextView*>(window->CurrentFocus()) != NULL;
}


bool
TShortcuts::IsDesktop() const
{
	return fInWindow && PoseView()->TargetModel()->IsDesktop();
}


bool
TShortcuts::IsQuery() const
{
	return fInWindow && PoseView()->TargetModel()->IsQuery();
}


bool
TShortcuts::IsQueryTemplate() const
{
	return fInWindow && PoseView()->TargetModel()->IsQueryTemplate();
}


bool
TShortcuts::IsRoot() const
{
	return fInWindow && PoseView()->TargetModel()->IsRoot();
}


bool
TShortcuts::InTrash() const
{
	return fInWindow && PoseView()->TargetModel()->InTrash();
}


bool
TShortcuts::IsTrash() const
{
	return fInWindow && PoseView()->TargetModel()->IsTrash();
}


bool
TShortcuts::IsVirtualDirectory() const
{
	return PoseView()->TargetModel()->IsVirtualDirectory();
}


bool
TShortcuts::IsVolume() const
{
	return fInWindow && PoseView()->TargetModel()->IsVolume();
}


bool
TShortcuts::HasSelection() const
{
	return fInWindow && PoseView()->CountSelected() > 0;
}


bool
TShortcuts::SelectionIsReadOnly() const
{
	return fInWindow && PoseView()->SelectedVolumeIsReadOnly();
}


bool
TShortcuts::TargetIsReadOnly() const
{
	return fInWindow && PoseView()->TargetVolumeIsReadOnly();
}
