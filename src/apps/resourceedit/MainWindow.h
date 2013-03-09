/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include "UndoContext.h"

#include <Window.h>


class ImageButton;
class ResourceListView;
class ResourceRow;
class SettingsFile;

class BBox;
class BEntry;
class BFilePanel;
class BMenu;
class BMenuBar;
class BMenuField;
class BMenuItem;
class BMessage;
class BPopUpMenu;
class BStringView;
class BTextControl;


class MainWindow : public BWindow {
public:
						MainWindow(BRect frame, BEntry* entry, SettingsFile* settings);
						~MainWindow();

	bool				QuitRequested();
	void				MouseDown(BPoint point);
	void				MessageReceived(BMessage* msg);
	void				SelectionChanged();

	void				AdaptSettings();

private:
	BEntry*				fAssocEntry;
	SettingsFile*		fSettings;

	BFilePanel*			fSavePanel;
	bool				fUnsavedChanges;

	UndoContext*		fUndoContext;

	BMenuBar*			fMenuBar;

	BMenu*				fFileMenu;
	BMenuItem*			fNewItem;
	BMenuItem*			fOpenItem;
	BMenuItem*			fCloseItem;
	BMenuItem*			fSaveItem;
	BMenuItem*			fSaveAsItem;
	BMenuItem*			fSaveAllItem;
	BMenuItem*			fMergeWithItem;
	BMenuItem*			fQuitItem;

	BMenu*				fEditMenu;
	BMenuItem*			fUndoItem;
	BMenuItem*			fRedoItem;
	BMenuItem*			fCutItem;
	BMenuItem*			fCopyItem;
	BMenuItem*			fPasteItem;
	BMenuItem*			fClearItem;
	BMenuItem*			fSelectAllItem;

	BMenu*				fToolsMenu;
	BMenuItem*			fAddAppResourcesItem;
	BMenuItem*			fSettingsItem;

	BMenu*				fHelpMenu;

	BTextControl*		fResourceIDText;
	BPopUpMenu*			fResourceTypePopUp;
	BMenuField*			fResourceTypeMenu;

	BView*				fToolbarView;
	ImageButton*		fAddButton;
	ImageButton*		fRemoveButton;
	ImageButton*		fMoveUpButton;
	ImageButton*		fMoveDownButton;

	ResourceListView*	fResourceList;
	BBox*				fStatsBox;
	BStringView*		fStatsString;

	void				_SetTitleFromEntry();
	void				_SaveAs();
	void				_Save(BEntry* entry = NULL);
	void				_Load();

	int32				_NextResourceID();

	void				_RefreshStats();
	void				_RefreshUndoRedo();
	void				_Do(UndoContext::Action* action);

	class AddAction : public UndoContext::Action {
	public:
							AddAction(const BString& label,
								ResourceListView* list, BList* rows);
							~AddAction();

		void 				Do();
		void 				Undo();

	private:
		ResourceListView*	fList;
		BList*				fRows;
		bool				fAdded;

	};

	class RemoveAction : public UndoContext::Action {
	public:
							RemoveAction(const BString& label,
								ResourceListView* list, BList* rows);
							~RemoveAction();

		void				Do();
		void				Undo();

	private:
		ResourceListView*	fList;
		BList*				fRows;
		bool				fRemoved;

	};

	class EditAction : public UndoContext::Action {
	public:
							EditAction(const BString& label,
								ResourceListView* list,
								ResourceRow* rowold, ResourceRow* rownew);
							~EditAction();

		void				Do();
		void				Undo();

	private:
		ResourceListView*	fList;
		ResourceRow*		fRowOld;
		ResourceRow*		fRowNew;

	};

	class MoveUpAction : public UndoContext::Action {
	public:
							MoveUpAction(const BString& label,
								ResourceListView* list, BList* rows);
							~MoveUpAction();

		void				Do();
		void				Undo();

	private:
		ResourceListView*	fList;
		BList*				fRows;

	};

	class MoveDownAction : public UndoContext::Action {
	public:
							MoveDownAction(const BString& label,
								ResourceListView* list, BList* rows);
							~MoveDownAction();

		void				Do();
		void				Undo();

	private:
		ResourceListView*	fList;
		BList*				fRows;

	};
};


#endif
