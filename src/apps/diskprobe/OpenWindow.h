/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef OPEN_WINDOW_H
#define OPEN_WINDOW_H


#include <Window.h>

class BMenu;


class OpenWindow : public BWindow {
	public:
		OpenWindow();
		virtual ~OpenWindow();

		virtual void MessageReceived(BMessage *message);
		virtual bool QuitRequested();

		static void CollectDevices(BMenu *menu, BEntry *startEntry = NULL);

	private:
		BMenu	*fDevicesMenu;
};

#endif	/* OPEN_WINDOW_H */
