/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef STATUS_WINDOW__H
#define STATUS_WINDOW__H

#include <Window.h>
#include "PPPStatusView.h"


class PPPStatusWindow : public BWindow {
	public:
		PPPStatusWindow(BRect frame, ppp_interface_id id);
		
		PPPStatusView *StatusView()
			{ return fStatusView; }
		
		virtual bool QuitRequested();

	private:
		PPPStatusView *fStatusView;
};


#endif
