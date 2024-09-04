/*
 * Copyright 2020-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _SHORTCUTS_H
#define _SHORTCUTS_H


#include <SupportDefs.h>

#include "ContainerWindow.h"


class BMenu;
class BMenuItem;


namespace BPrivate {

class BPoseView;


class TShortcuts {
public:
								TShortcuts();
								TShortcuts(BContainerWindow* window);

			// build item

			BMenuItem*			AddOnsItem();
			const char*			AddOnsLabel();

			BMenuItem*			ArrangeByItem();
			const char*			ArrangeByLabel();

			BMenuItem*			AddPrinterItem();
			const char*			AddPrinterLabel();

			BMenuItem*			CleanupItem();
			const char*			CleanupLabel();
			int32				CleanupCommand();

			BMenuItem*			CloseItem();
			const char*			CloseLabel();
			int32				CloseCommand();

			BMenuItem*			CloseAllInWorkspaceItem();
			const char*			CloseAllInWorkspaceLabel();

			BMenuItem*			CopyItem();
			const char*			CopyLabel();
			int32				CopyCommand();

			BMenuItem*			CopyToItem();
			BMenuItem*			CopyToItem(BMenu*);
			const char*			CopyToLabel();

			BMenuItem*			CreateLinkItem();
			BMenuItem*			CreateLinkItem(BMenu*);
			const char*			CreateLinkLabel();
			int32				CreateLinkCommand();

			BMenuItem*			CreateLinkHereItem();
			const char*			CreateLinkHereLabel();
			int32				CreateLinkHereCommand();

			BMenuItem*			CutItem();
			const char*			CutLabel();
			int32				CutCommand();

			BMenuItem*			DeleteItem();
			const char*			DeleteLabel();

			BMenuItem*			DuplicateItem();
			const char*			DuplicateLabel();

			BMenuItem*			EditNameItem();
			const char*			EditNameLabel();

			BMenuItem*			EditQueryItem();
			const char*			EditQueryLabel();

			BMenuItem*			EmptyTrashItem();
			const char*			EmptyTrashLabel();

			BMenuItem*			FindItem();
			const char*			FindLabel();

			BMenuItem*			GetInfoItem();
			const char*			GetInfoLabel();

			BMenuItem*			IdentifyItem();
			const char*			IdentifyLabel();

			BMenuItem*			InvertSelectionItem();
			const char*			InvertSelectionLabel();

			BMenuItem*			MakeActivePrinterItem();
			const char*			MakeActivePrinterLabel();

			BMenuItem*			MountItem();
			BMenuItem*			MountItem(BMenu*);
			const char*			MountLabel();

			BMenuItem*			MoveToItem();
			BMenuItem*			MoveToItem(BMenu*);
			const char*			MoveToLabel();

			BMenuItem*			MoveToTrashItem();
			const char*			MoveToTrashLabel();
			int32				MoveToTrashCommand();

			BMenuItem*			NewFolderItem();
			const char*			NewFolderLabel();

			BMenuItem*			NewTemplatesItem();
			BMenuItem*			NewTemplatesItem(BMenu*);
			const char*			NewTemplatesLabel();

			BMenuItem*			OpenItem();
			const char*			OpenLabel();

			BMenuItem*			OpenParentItem();
			const char*			OpenParentLabel();

			BMenuItem*			OpenWithItem();
			BMenuItem*			OpenWithItem(BMenu*);
			const char*			OpenWithLabel();

			BMenuItem*			PasteItem();
			const char*			PasteLabel();
			int32				PasteCommand();

			BMenuItem*			RestoreItem();
			const char*			RestoreLabel();

			BMenuItem*			ReverseOrderItem();
			const char*			ReverseOrderLabel();

			BMenuItem*			ResizeToFitItem();
			const char*			ResizeToFitLabel();

			BMenuItem*			SelectItem();
			const char*			SelectLabel();

			BMenuItem*			SelectAllItem();
			const char*			SelectAllLabel();

			BMenuItem*			UnmountItem();
			const char*			UnmountLabel();

			// update item

			void				Update(BMenu*);

			void				UpdateAddOnsItem(BMenuItem*);
			void				UpdateAddPrinterItem(BMenuItem*);
			void				UpdateArrangeByItem(BMenuItem*);
			void				UpdateCleanupItem(BMenuItem*);
			void				UpdateCloseAllInWorkspaceItem(BMenuItem*);
			void				UpdateCloseItem(BMenuItem*);
			void				UpdateCopyItem(BMenuItem*);
			void				UpdateCopyToItem(BMenuItem*);
			void				UpdateCreateLinkItem(BMenuItem*);
			void				UpdateCreateLinkHereItem(BMenuItem*);
			void				UpdateCutItem(BMenuItem*);
			void				UpdateDeleteItem(BMenuItem*);
			void				UpdateDuplicateItem(BMenuItem*);
			void				UpdateEditNameItem(BMenuItem*);
			void				UpdateEditQueryItem(BMenuItem*);
			void				UpdateEmptyTrashItem(BMenuItem*);
			void				UpdateFindItem(BMenuItem*);
			void				UpdateGetInfoItem(BMenuItem*);
			void				UpdateIdentifyItem(BMenuItem*);
			void				UpdateInvertSelectionItem(BMenuItem*);
			void				UpdateMakeActivePrinterItem(BMenuItem*);
			void				UpdateMoveToItem(BMenuItem*);
			void				UpdateMoveToTrashItem(BMenuItem*);
			void				UpdateNewFolderItem(BMenuItem*);
			void				UpdateNewTemplatesItem(BMenuItem*);
			void				UpdateOpenItem(BMenuItem*);
			void				UpdateOpenParentItem(BMenuItem*);
			void				UpdateOpenWithItem(BMenuItem*);
			void				UpdatePasteItem(BMenuItem*);
			void				UpdateResizeToFitItem(BMenuItem*);
			void				UpdateRestoreItem(BMenuItem*);
			void				UpdateReverseOrderItem(BMenuItem*);
			void				UpdateSelectItem(BMenuItem*);
			void				UpdateSelectAllItem(BMenuItem*);
			void				UpdateUnmountItem(BMenuItem*);

			// convenience methods

			BMenuItem*			FindItem(BMenu* menu, int32 command1, int32 command2);
			BPoseView*			PoseView() const;

			bool				IsCurrentFocusOnTextView() const;
			bool				IsDesktop() const;
			bool				IsQuery() const;
			bool				IsQueryTemplate() const;
			bool				IsRoot() const;
			bool				InTrash() const;
			bool				IsTrash() const;
			bool				IsVirtualDirectory() const;
			bool				IsVolume() const;
			bool				HasSelection() const;
			bool				SelectionIsReadOnly() const;
			bool				TargetIsReadOnly() const;

private:
			BContainerWindow*	fContainerWindow;
			bool				fInWindow;
};


inline BPoseView*
TShortcuts::PoseView() const
{
	return fInWindow ? fContainerWindow->PoseView() : NULL;
}

} // namespace BPrivate

using namespace BPrivate;


#endif // _SHORTCUTS_H
