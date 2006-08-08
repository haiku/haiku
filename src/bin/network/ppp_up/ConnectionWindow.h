/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef CONNECTION_WINDOW__H
#define CONNECTION_WINDOW__H

#include <Window.h>
#include "ConnectionView.h"


class ConnectionWindow : public BWindow {
	public:
		ConnectionWindow(BRect frame, const BString& interfaceName);
		
		virtual bool QuitRequested();

	private:
		ConnectionView *fConnectionView;
};


#endif
