/*
 * Copyright 2004-2018, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef OPEN_WINDOW_H
#define OPEN_WINDOW_H


#include <Window.h>


class BEntry;
class BMenu;


class OpenWindow : public BWindow {
public:
								OpenWindow();
	virtual						~OpenWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	static	void				CollectDevices(BMenu* menu,
									BEntry* startEntry = NULL);

private:
			BMenu*				fDevicesMenu;
};


#endif	/* OPEN_WINDOW_H */
