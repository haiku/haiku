//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _PPP_SERVER__H
#define _PPP_SERVER__H

#include <Handler.h>
#include <PPPInterfaceListener.h>

class SimpleMessageFilter;


class PPPServer : public BHandler {
	public:
		PPPServer();
		virtual ~PPPServer();
		
		virtual void MessageReceived(BMessage *message);
			// the SimpleMessageFilter routes ppp_server messages to this handler

	private:
		void InitInterfaces();
		bool AskBeforeDialing(ppp_interface_id id);
		
		void HandleReportMessage(BMessage *message);

	private:
		SimpleMessageFilter *fFilter;
		PPPInterfaceListener *fListener;
};


#endif
