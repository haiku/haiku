//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "KPPPLCP.h"


// TODO:
// - add LCP extension handlers


PPPLCP::PPPLCP(PPPInterface& interface)
	: PPPProtocol("LCP", PPP_ESTABLISHMENT_PHASE, PPP_LCP_PROTOCOL
		, AF_UNSPEC, &interface, NULL, PPP_ALWAYS_ALLOWED),
	fTarget(NULL)
{
}


PPPLCP::~PPPLCP()
{
}


bool
PPPLCP::AddOptionHandler(PPPOptionHandler *handler)
{
	if(!handler)
		return false
	
	LockerHelper locker(StateMachine().Locker());
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	fOptionHandlers.AddItem(handler);
}


bool
PPPLCP::RemoveOptionHandler(PPPOptionHandler *handler)
{
	LockerHelper locker(StateMachine().Locker());
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	return fOptionHandlers.RemoveItem(handler);
}


PPPOptionHandler*
PPPLCP::OptionHandlerAt(int32 index) const
{
	PPPOptionHandler *handler = fOptionHandlers.ItemAt(index);
	
	if(handler == fOptionHandlers.DefaultItem())
		return NULL;
	
	return handler;
}
