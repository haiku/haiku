/*
 * Copyright 2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Zhuowei Zhang
 */
#ifndef CONSOLE_WINDOW_H
#define CONSOLE_WINDOW_H


#include <String.h>
#include <Window.h>


class BListView;
class BButton;


class ConsoleWindow : public BWindow {
public:
								ConsoleWindow(BRect frame);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
private:
			BListView*			fMessagesListView;
			BButton* 			fClearMessagesButton;
};


#endif // CONSOLE_WINDOW_H
