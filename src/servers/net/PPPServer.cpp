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
	fListener = new PPPInterfaceListener(this);
	fListener->WatchAllInterfaces();
	
	InitInterfaces();
}


PPPServer::~PPPServer()
{
	delete fListener;
	be_app->RemoveCommonFilter(fFilter);
	be_app->RemoveHandler(this);
	delete fFilter;
}


void
PPPServer::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case PPP_REPORT_MESSAGE:
			HandleReportMessage(message);
		break;
		
		// TODO: handle 
		
		default:
			BHandler::MessageReceived(message);
	}
}


void
PPPServer::InitInterfaces()
{
}


bool
PPPServer::AskBeforeDialing(ppp_interface_id id)
{
	return false;
}


void
PPPServer::HandleReportMessage(BMessage *message)
{
	thread_id sender;
	message->FindInt32("sender", &sender);
	
	ppp_interface_id id;
	if(message->FindInt32("interface", reinterpret_cast<int32*>(&id)) != B_OK) {
		send_data(sender, B_OK, NULL, 0);
		return;
	}
	
	int32 type, code;
	message->FindInt32("type", &type);
	message->FindInt32("code", &code);
	
	if(type == PPP_MANAGER_REPORT && code == PPP_REPORT_INTERFACE_CREATED) {
		// TODO: check if we need to add this interface to our watch-list
	} else if(type == PPP_CONNECTION_REPORT) {
		switch(code) {
			case PPP_REPORT_GOING_UP: {
				if(AskBeforeDialing(id)) {
//					OpenDialRequestWindow(id, sender);
					return;
				}
			} break;
		}
	} else if(type == PPP_DESTRUCTION_REPORT) {
		// TODO: check if this interface has DOD enabled. if so: create new!
	}
	
	send_data(sender, B_OK, NULL, 0);
}
