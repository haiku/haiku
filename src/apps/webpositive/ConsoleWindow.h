/*
 * Copyright 2014-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Zhuowei Zhang
 *		Humdinger
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
			void				_CopyToClipboard();

private:
			BListView*			fMessagesListView;
			BButton* 			fClearMessagesButton;
			BButton* 			fCopyMessagesButton;
			BString				fPreviousText;
			int32				fRepeatCounter;
};


#endif // CONSOLE_WINDOW_H
