//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include "PPPServer.h"
#include "SimpleMessageFilter.h"
#include <Application.h>


// the message constants that should be filtered
static const uint32 *kPPPWhatValues = {
	// PPPS_CONNECT,
	0 // end-of-list
};


PPPServer::PPPServer()
	: BHandler("PPPServer")
{
	be_app->AddHandler(this);
	fFilter = new SimpleMessageFilter(kPPPWhatValues, this);
	be_app->AddCommonFilter(fFilter);
}


PPPServer::~PPPServer()
{
	be_app->RemoveHandler(this);
	be_app->RemoveCommonFilter(fFilter);
	delete fFilter;
}


void
PPPServer::MessageReceived(BMessage *message)
{
	switch(message->what) {
		// TODO: handle PPP stack messages
		
		// TODO: handle requests from DialUpPreflet
		
		default:
			BHandler::MessageReceived(message);
	}
}
