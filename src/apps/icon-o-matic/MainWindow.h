/*
 * Copyright 2006-2010, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include <Entry.h>
#include <Window.h>

#include "IconBuild.h"
#include "Observer.h"


class BMenu;
class BMenuBar;
class BMenuItem;
class CanvasView;
class CurrentColor;
class Document;
class DocumentSaver;
class IconObjectListView;
class IconEditorApp;
class IconView;
class PathListView;
class ShapeListView;
class StyleListView;
class StyleView;
class SwatchGroup;
class TransformerListView;

_BEGIN_ICON_NAMESPACE
	class Icon;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


class MultipleManipulatorState;

enum {
	MSG_OPEN						= 'open',
	MSG_APPEND						= 'apnd',
	MSG_SAVE						= 'save',
	MSG_EXPORT						= 'xprt',
};


class MainWindow : public BWindow, public Observer {
public:
								MainWindow(BRect frame, IconEditorApp* app,
									const BMessage* settings);
	virtual						~MainWindow();

	// BWindow interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Show();
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

			void				Open(const entry_ref& ref,
									bool append = false);
			void				Open(const BMessenger& externalObserver,
									const uint8* data, size_t size);

			void				StoreSettings(BMessage* archive);
			void				RestoreSettings(const BMessage* archive);

private:
			void				_Init();
			void				_CreateGUI();
			BMenuBar*			_CreateMenuBar();

			void				_ImproveScrollBarLayout(BView* target);

			bool				_CheckSaveIcon(const BMessage* currentMessage);
			void				_PickUpActionBeforeSave();

			void				_MakeIconEmpty();
			DocumentSaver*		_CreateSaver(const entry_ref& ref,
									uint32 exportMode);

			const char*			_FileName(bool preferExporter) const;
			void				_UpdateWindowTitle();

private:
			IconEditorApp*		fApp;
			Document*			fDocument;
			CurrentColor*		fCurrentColor;
			Icon*				fIcon;

			BMessage*			fMessageAfterSave;
		
			BMenu*				fPathMenu;
			BMenu*				fStyleMenu;
			BMenu*				fShapeMenu;
			BMenu*				fTransformerMenu;
			BMenu*				fPropertyMenu;
			BMenu*				fSwatchMenu;
		
			BMenuItem*			fUndoMI;
			BMenuItem*			fRedoMI;
			BMenuItem*			fMouseFilterOffMI;
			BMenuItem*			fMouseFilter64MI;
			BMenuItem*			fMouseFilter32MI;
			BMenuItem*			fMouseFilter16MI;
		
			CanvasView*			fCanvasView;
			SwatchGroup*		fSwatchGroup;
			StyleView*			fStyleView;
		
			IconView*			fIconPreview16Folder;
			IconView*			fIconPreview16Menu;
			IconView*			fIconPreview32Folder;
			IconView*			fIconPreview32Desktop;
			IconView*			fIconPreview48;
			IconView*			fIconPreview64;
		
			PathListView*		fPathListView;
			StyleListView*		fStyleListView;
		
			ShapeListView*		fShapeListView;
			TransformerListView* fTransformerListView;
			IconObjectListView*	fPropertyListView;
		
			// TODO: for testing only...
			MultipleManipulatorState* fState;
};


#endif // MAIN_WINDOW_H
