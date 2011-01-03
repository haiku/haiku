/*
 * Copyright 2004, pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef EDIT_SERVER_LIST_WINDOW_H
#define EDIT_SERVER_LIST_WINDOW_H


#include <Window.h>

class BTextView;


class EditServerListWindow : public BWindow {
	public:
		EditServerListWindow(BRect position, const BMessage &settings);
		virtual ~EditServerListWindow();

		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *message);

	private:
		void UpdateDefaultServerView(const BMessage &message);
		void UpdateServerView(const BMessage &message);

		void UpdateServerList();

		const BMessage	&fSettings;
		BTextView		*fServerView;
};

#endif	/* EDIT_SERVER_LIST_WINDOW_H */
