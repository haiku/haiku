/*
 * Copyright 2005, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef STATUS_WINDOW__H
#define STATUS_WINDOW__H

#include <Window.h>
#include <PPPDefs.h>


class StatusWindow : public BWindow {
	public:
		StatusWindow(BRect frame, ppp_interface_id id);
		
		virtual bool QuitRequested();
};


#endif
