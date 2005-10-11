/*
 * Copyright 2004-2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef _PPP_SERVER__H
#define _PPP_SERVER__H

#include <Handler.h>
#include <PPPInterfaceListener.h>


class PPPServer : public BHandler {
	public:
		PPPServer();
		virtual ~PPPServer();
		
		virtual void MessageReceived(BMessage *message);
			// the SimpleMessageFilter routes ppp_server messages to this handler

	private:
		void InitInterfaces();
		void UninitInterfaces();
		
		void HandleReportMessage(BMessage *message);
		void CreateConnectionRequestWindow(ppp_interface_id id);

	private:
		PPPInterfaceListener fListener;
};


#endif
