/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXTENSION_WINDOW_H
#define EXTENSION_WINDOW_H


#include <Messenger.h>
#include <Mime.h>
#include <String.h>
#include <Window.h>

class BButton;
class BTextControl;

class FileTypesWindow;


class ExtensionWindow : public BWindow {
	public:
		ExtensionWindow(FileTypesWindow* target, BMimeType& type, const char* extension);
		virtual ~ExtensionWindow();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

	private:
		BMessenger		fTarget;
		BMimeType		fMimeType;
		BString			fExtension;
		BTextControl*	fExtensionControl;
		BButton*		fAcceptButton;
};

#endif	// EXTENSION_WINDOW_H
