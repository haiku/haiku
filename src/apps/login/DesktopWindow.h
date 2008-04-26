/*
 * Copyright 2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DESKTOPWINDOW_H_
#define _DESKTOPWINDOW_H_

#include <Shelf.h>
#include <Window.h>

class DesktopWindow : public BWindow {
	public:
					DesktopWindow(BRect frame, bool editMode);
		virtual		~DesktopWindow();

		bool		QuitRequested();
		void		DispatchMessage(BMessage *message, BHandler *handler);

	private:
		bool		fEditShelfMode;

		//TODO:
		BShelf*			fDesktopShelf;
};

#endif	// _DESKTOPWINDOW_H_
