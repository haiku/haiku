//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _PPP_SERVER__H
#define _PPP_SERVER__H

#include <Handler.h>

class SimpleMessageFilter;


class PPPServer : public BHandler {
	public:
		PPPServer();
		virtual ~PPPServer();
		
		virtual void MessageReceived(BMessage *message);
			// the SimpleMessageFilter routes ppp_server messages to this handler

	private:
		SimpleMessageFilter *fFilter;
};


#endif
