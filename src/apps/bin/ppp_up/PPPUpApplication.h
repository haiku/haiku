/*
 * Copyright 2005, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef PPP_UP_APPLICATION__H
#define PPP_UP_APPLICATION__H

#include <Application.h>
#include <PPPDefs.h>

class ConnectionWindow;

#define APP_SIGNATURE "application/x-vnd.haiku.ppp_up"


class PPPUpApplication : public BApplication {
	public:
		PPPUpApplication(const char *name, ppp_interface_id id,
			thread_id replyThread);
		
		virtual void ReadyToRun();
		
		const char *Name() const
			{ return fName; }
		ppp_interface_id ID() const
			{ return fID; }
		thread_id ReplyThread() const
			{ return fReplyThread; }

	private:
		const char *fName;
		ppp_interface_id fID;
		thread_id fReplyThread;
		ConnectionWindow *fWindow;
};


#endif
