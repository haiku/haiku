/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */
#ifndef STYLED_EDIT_WINDOW_H
#define STYLED_EDIT_WINDOW_H


#include <FilePanel.h>
#include <MenuBar.h>
#include <Message.h>
#include <Rect.h>
#include <String.h>
#include <TextView.h>
#include <Window.h>


class StyledEditView;

class StyledEditWindow : public BWindow {
	public:
						StyledEditWindow(BRect frame, int32 id, uint32 encoding = 0);
						StyledEditWindow(BRect frame, entry_ref *ref, uint32 encoding = 0);
		virtual			~StyledEditWindow();
	
		virtual void	Quit();
		virtual bool 	QuitRequested();
		virtual void 	MessageReceived(BMessage *message);
		virtual void	MenusBeginning();

		status_t 		Save(BMessage *message = 0);
		status_t		SaveAs(BMessage *message = 0);
		void			OpenFile(entry_ref *ref); 
		status_t		PageSetup(const char *documentname);
		void			Print(const char *documentname);
		void			SearchAllWindows(BString find, BString replace, bool casesens);

	private: 
		void			InitWindow(uint32 encoding = 0);
		bool			Search(BString searchfor, bool casesens, bool wrap, bool backsearch);
		void			FindSelection();
		bool			Replace(BString findthis, BString replacewith, bool casesens, bool wrap, bool backsearch);
		void			ReplaceAll(BString find, BString replace, bool casesens);
		void			SetFontSize(float fontSize);
		void			SetFontColor(const rgb_color *color);
		void			SetFontStyle(const char *fontFamily, const char *fontStyle);
		status_t		_LoadFile(entry_ref* ref);
		void			RevertToSaved();

		BMenuBar		*fMenuBar;
		BMessage		*fPrintSettings;
		BMessage		*fSaveMessage; 
		BMenu			*fRecentMenu;

		BMenu			*fFontMenu;
		BMenu			*fFontSizeMenu;
		BMenu			*fFontColorMenu;
		BMenuItem		*fCurrentFontItem;

		BMenuItem		*fSaveItem;
		BMenuItem		*fRevertItem;

		BMenuItem		*fUndoItem;
		BMenuItem		*fCutItem;
		BMenuItem		*fCopyItem;
		BMenuItem		*fClearItem;

		BMenuItem		*fFindAgainItem;
		BMenuItem		*fReplaceSameItem;

		BMenuItem		*fBlackItem;
		BMenuItem		*fRedItem;
		BMenuItem		*fGreenItem;
		BMenuItem		*fBlueItem;
		BMenuItem		*fCyanItem;
		BMenuItem		*fMagentaItem;
		BMenuItem		*fYellowItem;

		BMenuItem		*fWrapItem;
		BMenuItem		*fAlignLeft;
		BMenuItem		*fAlignCenter;
		BMenuItem		*fAlignRight;

		BString         fStringToFind;
		BString			fReplaceString;

		// undo modes
		bool 			fUndoFlag;   // we just did an undo action
		bool			fCanUndo;    // we can do an undo action next
		bool 			fRedoFlag;   // we just did a redo action
		bool			fCanRedo;    // we can do a redo action next

		// clean modes
		bool			fUndoCleans; // an undo action will put us in a clean state
		bool			fRedoCleans; // a redo action will put us in a clean state
		bool			fClean;      // we are in a clean state

		bool			fCaseSens;
		bool			fWrapAround;
		bool			fBackSearch;

		StyledEditView  *fTextView;
		BScrollView		*fScrollView;

		BFilePanel		*fSavePanel;
		BMenu			*fSavePanelEncodingMenu;
};

#endif	// STYLED_EDIT_WINDOW_H
