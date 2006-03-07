/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TYPE_LIST_WINDOW_H
#define TYPE_LIST_WINDOW_H


#include <Messenger.h>
#include <Window.h>

class BButton;
class BMenu;
class BTextControl;

class MimeTypeListView;


class TypeListWindow : public BWindow {
	public:
		TypeListWindow(const char* currentType, uint32 what, BWindow* target);
		virtual ~TypeListWindow();

		virtual void MessageReceived(BMessage* message);

	private:
		BMessenger			fTarget;
		uint32				fWhat;
		MimeTypeListView*	fListView;
		BButton*			fSelectButton;
};

#endif	// TYPE_LIST_WINDOW_H
