/*
 * Copyright 2004, pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef UPDATE_WINDOW_H
#define UPDATE_WINDOW_H


#include <Window.h>

class BStatusBar;
class BButton;
class BMenuItem;
class BMenu;


class UpdateWindow : public BWindow {
	public:
		UpdateWindow(BRect position, const BMessage &settings);
		virtual ~UpdateWindow();

		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *message);

		void BuildServerMenu();

	private:
		const BMessage	&fSettings;
		BStatusBar		*fStatusBar;
		BButton			*fButton;
		BMenuItem		*fAutoModeItem, *fTryAllServersItem, *fChooseDefaultServerItem;
		BMenu			*fServerMenu;
		bool			fAutoQuit;
};

#endif	/* UPDATE_WINDOW_H */
