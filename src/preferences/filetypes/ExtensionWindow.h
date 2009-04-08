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
		ExtensionWindow(FileTypesWindow* target, BMimeType& type,
			const char* extension);
		virtual ~ExtensionWindow();

		virtual void MessageReceived(BMessage* message);

	private:
		BMessenger		fTarget;
		BMimeType		fMimeType;
		BString			fExtension;
		BTextControl*	fExtensionControl;
		BButton*		fAcceptButton;
};

extern status_t merge_extensions(BMimeType& type, const BList& newExtensions,
	const char* removeExtension = NULL);
extern status_t replace_extension(BMimeType& type, const char* newExtension,
	const char* oldExtension);

#endif	// EXTENSION_WINDOW_H
