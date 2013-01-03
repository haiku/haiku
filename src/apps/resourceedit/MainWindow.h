/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include <Window.h>


class ImageButton;

class BColumnListView;
class BEntry;
class BFilePanel;
class BMenu;
class BMenuBar;
class BMenuField;
class BMenuItem;
class BMessage;
class BPopUpMenu;
class BTextControl;


class MainWindow : public BWindow {
public:
						MainWindow(BRect frame, BEntry* entry);
						~MainWindow();

	bool				QuitRequested();
	void				MessageReceived(BMessage* msg);
	void				SelectionChanged();

private:
	BEntry*				fAssocEntry;

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

	BMenu*				fHelpMenu;

	BTextControl*		fResourceIDText;
	BPopUpMenu*			fResourceTypePopUp;
	BMenuField*			fResourceTypeMenu;

	BView*				fToolbarView;
	ImageButton*		fAddButton;
	ImageButton*		fRemoveButton;
	ImageButton*		fMoveUpButton;
	ImageButton*		fMoveDownButton;

	BColumnListView*	fResourceList;

	BFilePanel*			fSavePanel;

	void				_SetTitleFromEntry();
	void				_SaveAs();
	void				_Save(BEntry* entry = NULL);
	void				_Load();

	int32				_NextResourceID();

};


#endif
