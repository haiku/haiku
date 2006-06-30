/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>

class CanvasView;
class Document;
class IconEditorApp;
class IconView;
class PathListView;
class ShapeListView;
class SwatchGroup;
class ViewState;

class MainWindow : public BWindow {
 public:
								MainWindow(IconEditorApp* app,
										   Document* document);
	virtual						~MainWindow();

	// BWindow interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

 private:
			void				_Init();
			BView*				_CreateGUI(BRect frame);

	IconEditorApp*				fApp;
	Document*					fDocument;

	CanvasView*					fCanvasView;
	SwatchGroup*				fSwatchGroup;

	IconView*					fIconPreview16;
	IconView*					fIconPreview32;
	IconView*					fIconPreview48;
	IconView*					fIconPreview64;

	PathListView*				fPathListView;
	ShapeListView*				fShapeListView;
	// TODO: for testing only...
	ViewState*					fState;
};

#endif // MAIN_WINDOW_H
