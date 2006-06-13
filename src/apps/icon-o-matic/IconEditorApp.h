/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_EDITOR_APP_H
#define ICON_EDITOR_APP_H

#include <Application.h>

class Document;
class MainWindow;

class IconEditorApp : public BApplication {
 public:
								IconEditorApp();
	virtual						~IconEditorApp();

	// BApplication interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				ReadyToRun();

	// IconEditorApp

 private:
			MainWindow*			fMainWindow;
			Document*			fDocument;
};

#endif // ICON_EDITOR_APP_H
