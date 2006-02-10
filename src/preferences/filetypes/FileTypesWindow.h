/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_TYPES_WINDOW_H
#define FILE_TYPES_WINDOW_H


#include <Window.h>


class FileTypesWindow : public BWindow {
	public:
		FileTypesWindow(BRect frame);
		virtual ~FileTypesWindow();

		virtual void MessageReceived(BMessage* message);

		virtual bool QuitRequested();

	private: 
};

#endif	// FILE_TYPES_WINDOW_H
