/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PCWINDOW_H_
#define _PCWINDOW_H_


#include <Window.h>


class PCWindow : public BWindow {
	public:
						PCWindow();
						~PCWindow();

		virtual	bool	QuitRequested();

	private:
		bool			fDraggersAreDrawn;
};

#endif // _PCWINDOW_H_
