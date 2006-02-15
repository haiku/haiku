/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NEW_FILE_TYPE_WINDOW_H
#define NEW_FILE_TYPE_WINDOW_H


#include <Messenger.h>
#include <Window.h>

class BButton;
class BMenu;
class BTextControl;

class FileTypesWindow;


class NewFileTypeWindow : public BWindow {
	public:
		NewFileTypeWindow(FileTypesWindow* target, const char* currentType);
		virtual ~NewFileTypeWindow();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

	private:
		BMessenger		fTarget;
		BMenu*			fSupertypesMenu;
		BTextControl*	fNameControl;
		BButton*		fAddButton;
};

#endif	// NEW_FILE_TYPE_WINDOW_H
