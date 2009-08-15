/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
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

	virtual void				FrameResized(float width, float height);

	static	void				CollectDevices(BMenu* menu,
									BEntry* startEntry = NULL);

private:
			BMenu*				fDevicesMenu;
			bool				fCentered;
};

#endif	/* OPEN_WINDOW_H */
