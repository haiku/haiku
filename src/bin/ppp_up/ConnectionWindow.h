/*
 * Copyright 2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef CONNECTION_WINDOW__H
#define CONNECTION_WINDOW__H

#include <Window.h>
#include "ConnectionView.h"


class ConnectionWindow : public BWindow {
	public:
		ConnectionWindow(BRect frame, const char *name, ppp_interface_id id,
			thread_id replyThread);
		
		virtual bool QuitRequested();
		void RequestReply()
			{ fConnectionView->fReplyRequested = true; }
		void UpdateStatus(BMessage& message);
		bool ResponseTest();

	private:
		ConnectionView *fConnectionView;
};


#endif
