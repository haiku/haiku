/*
 * Copyright 2002-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 *		Jonas Sundström
 */
#ifndef STYLED_EDIT_WINDOW_H
#define STYLED_EDIT_WINDOW_H


#include <Alert.h>
#include <Window.h>
#include <String.h>
#include <Message.h>


struct entry_ref;

class BMenu;
class BMessage;
class BMenuBar;
class BMenuItem;
class BFilePanel;
class BScrollView;
class StyledEditView;


class StyledEditWindow : public BWindow {
public:
								StyledEditWindow(BRect frame, int32 id,
									uint32 encoding = 0);
								StyledEditWindow(BRect frame, entry_ref* ref,
									uint32 encoding = 0);
	virtual						~StyledEditWindow();

	virtual void				Quit();
	virtual bool				QuitRequested();
	virtual void				MessageReceived(BMessage* message);
	virtual void				MenusBeginning();

			status_t			Save(BMessage* message = NULL);
			status_t			SaveAs(BMessage* message = NULL);
			void				OpenFile(entry_ref* ref);
			status_t			PageSetup(const char* documentName);
			void				Print(const char* documentName);
			void				SearchAllWindows(BString find, BString replace,
									bool caseSensitive);
			bool				IsDocumentEntryRef(const entry_ref* ref);

private:
			void				_InitWindow(uint32 encoding = 0);
			void				_LoadAttrs();
			void				_SaveAttrs();
			status_t			_LoadFile(entry_ref* ref);
			void				_RevertToSaved();
			bool				_Search(BString searchFor, bool caseSensitive,
									bool wrap, bool backSearch,
									bool scrollToOccurence = true);
			void				_FindSelection();
			bool				_Replace(BString findThis, BString replaceWith,
									bool caseSensitive, bool wrap,
									bool backSearch);
			void				_ReplaceAll(BString find, BString replace,
									bool caseSensitive);
			void				_SetFontSize(float fontSize);
			void				_SetFontColor(const rgb_color* color);
			void				_SetFontStyle(const char* fontFamily,
									const char* fontStyle);
			int32				_ShowStatistics();
			void				_UpdateCleanUndoRedoSaveRevert();
			int32				_ShowAlert(const BString& text,
									const BString& label, const BString& label2,
									const BString& label3,
									alert_type type) const;

private:
			BMenuBar*			fMenuBar;
			BMessage*			fPrintSettings;
			BMessage*			fSaveMessage;
			BMenu*				fRecentMenu;

			BMenu*				fFontMenu;
			BMenu*				fFontSizeMenu;
			BMenu*				fFontColorMenu;
			BMenuItem*			fCurrentFontItem;
			BMenuItem*			fCurrentStyleItem;

			BMenuItem*			fSaveItem;
			BMenuItem*			fRevertItem;

			BMenuItem*			fUndoItem;
			BMenuItem*			fCutItem;
			BMenuItem*			fCopyItem;

			BMenuItem*			fFindAgainItem;
			BMenuItem*			fReplaceSameItem;

			BMenuItem*			fBlackItem;
			BMenuItem*			fRedItem;
			BMenuItem*			fGreenItem;
			BMenuItem*			fBlueItem;
			BMenuItem*			fCyanItem;
			BMenuItem*			fMagentaItem;
			BMenuItem*			fYellowItem;

			BMenuItem*			fBoldItem;
			BMenuItem*			fItalicItem;

			BMenuItem*			fWrapItem;
			BMenuItem*			fAlignLeft;
			BMenuItem*			fAlignCenter;
			BMenuItem*			fAlignRight;

			BString				fStringToFind;
			BString				fReplaceString;

			// undo modes
			bool				fUndoFlag;	// we just did an undo action
			bool				fCanUndo;	// we can do an undo action next
			bool 				fRedoFlag;	// we just did a redo action
			bool				fCanRedo;	// we can do a redo action next

			// clean modes
			bool				fUndoCleans;
				// an undo action will put us in a clean state
			bool				fRedoCleans;
				// a redo action will put us in a clean state
			bool				fClean;		// we are in a clean state

			bool				fCaseSensitive;
			bool				fWrapAround;
			bool				fBackSearch;

			StyledEditView*		fTextView;
			BScrollView*		fScrollView;

			BFilePanel*			fSavePanel;
			BMenu*				fSavePanelEncodingMenu;
};


#endif	// STYLED_EDIT_WINDOW_H
