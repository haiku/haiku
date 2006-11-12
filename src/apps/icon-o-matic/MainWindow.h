/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Entry.h>
#include <Window.h>

#include "Observer.h"

class BMenu;
class BMenuBar;
class BMenuItem;
class CanvasView;
class Document;
class Icon;
class IconObjectListView;
class IconEditorApp;
class IconView;
class PathListView;
class ShapeListView;
class StyleListView;
class StyleView;
class SwatchGroup;
class TransformerListView;

class MultipleManipulatorState;

enum {
	MSG_SET_ICON	= 'sicn',
};

class MainWindow : public BWindow,
				   public Observer {
 public:
								MainWindow(IconEditorApp* app,
										   Document* document);
	virtual						~MainWindow();

	// BWindow interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
	virtual	void				WorkspaceActivated(int32 workspace,
												   bool active);
	virtual	void				WorkspacesChanged(uint32 oldWorkspaces,
												  uint32 newWorkspaces);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// MainWindow
			void				MakeEmpty();
			void				SetIcon(Icon* icon);

			void				StoreSettings(BMessage* archive);
			void				RestoreSettings(BMessage* archive);

 private:
			void				_Init();
			void				_CreateGUI(BRect frame);
			BMenuBar*			_CreateMenuBar(BRect frame);

	IconEditorApp*				fApp;
	Document*					fDocument;
	Icon*						fIcon;

	BMenu*						fPathMenu;
	BMenu*						fStyleMenu;
	BMenu*						fShapeMenu;
	BMenu*						fTransformerMenu;
	BMenu*						fPropertyMenu;
	BMenu*						fSwatchMenu;

	BMenuItem*					fUndoMI;
	BMenuItem*					fRedoMI;

	CanvasView*					fCanvasView;
	SwatchGroup*				fSwatchGroup;
	StyleView*					fStyleView;

	IconView*					fIconPreview16Folder;
	IconView*					fIconPreview16Menu;
	IconView*					fIconPreview32Folder;
	IconView*					fIconPreview32Desktop;
	IconView*					fIconPreview48;
	IconView*					fIconPreview64;

	PathListView*				fPathListView;
	StyleListView*				fStyleListView;

	ShapeListView*				fShapeListView;
	TransformerListView*		fTransformerListView;
	IconObjectListView*			fPropertyListView;

	// TODO: for testing only...
	MultipleManipulatorState*	fState;
};

#endif // MAIN_WINDOW_H
