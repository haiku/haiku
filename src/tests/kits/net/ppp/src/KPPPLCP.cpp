//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPInterface.h>
#include <KPPPDevice.h>
#include <KPPPEncapsulator.h>
#include <KPPPOptionHandler.h>

#include <LockerHelper.h>

#include <netinet/in.h>
#include <mbuf.h>
#include <sys/socket.h>


#define PPP_PROTOCOL_OVERHEAD				2

// TODO:
// - add LCP extension handlers


PPPLCP::PPPLCP(PPPInterface& interface)
	: PPPProtocol("LCP", PPP_ESTABLISHMENT_PHASE, PPP_LCP_PROTOCOL,
		AF_UNSPEC, interface, NULL, PPP_ALWAYS_ALLOWED),
	fStateMachine(interface.StateMachine()), fTarget(NULL)
{
	SetUpRequested(false);
		// the state machine does everything for us
}


PPPLCP::~PPPLCP()
{
	while(CountOptionHandlers())
		delete OptionHandlerAt(0);
}


bool
PPPLCP::AddOptionHandler(PPPOptionHandler *handler)
{
	if(!handler)
		return false;
	
	LockerHelper locker(StateMachine().Locker());
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	return fOptionHandlers.AddItem(handler);
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
	
	if(handler == fOptionHandlers.GetDefaultItem())
		return NULL;
	
	return handler;
}

uint32
PPPLCP::AdditionalOverhead() const
{
	uint32 overhead = PPP_PROTOCOL_OVERHEAD;
	
	if(Target())
		overhead += Target()->Overhead();
	
	return overhead;
}


bool
PPPLCP::Up()
{
	return true;
}


bool
PPPLCP::Down()
{
	return true;
}


status_t
PPPLCP::Send(struct mbuf *packet)
{
	if(Target())
		return Target()->Send(packet, PPP_LCP_PROTOCOL);
	else
		return Interface().Send(packet, PPP_LCP_PROTOCOL);
}


status_t
PPPLCP::Receive(struct mbuf *packet, uint16 protocol)
{
	if(protocol != PPP_LCP_PROTOCOL)
		return PPP_UNHANDLED;
	
	ppp_lcp_packet *data = mtod(packet, ppp_lcp_packet*);
	
	if(ntohs(data->length) < 4)
		return B_ERROR;
	
	switch(data->code) {
		case PPP_CONFIGURE_REQUEST:
			StateMachine().RCREvent(packet);
		break;
		
		case PPP_CONFIGURE_ACK:
			StateMachine().RCAEvent(packet);
		break;
		
		case PPP_CONFIGURE_NAK:
		case PPP_CONFIGURE_REJECT:
			StateMachine().RCNEvent(packet);
		break;
		
		case PPP_TERMINATE_REQUEST:
			StateMachine().RTREvent(packet);
		break;
		
		case PPP_TERMINATE_ACK:
			StateMachine().RTAEvent(packet);
		break;
		
		case PPP_CODE_REJECT:
			StateMachine().RXJEvent(packet);
		break;
		
		case PPP_PROTOCOL_REJECT:
			StateMachine().RXJEvent(packet);
		break;
		
		case PPP_ECHO_REQUEST:
			StateMachine().RXREvent(packet);
		break;
		
		case PPP_ECHO_REPLY:
		case PPP_DISCARD_REQUEST:
			// do nothing
		break;
		
		default:
			StateMachine().RUCEvent(packet, PPP_LCP_PROTOCOL, PPP_CODE_REJECT);
	}
	
	return B_OK;
}


void
PPPLCP::Pulse()
{
	StateMachine().TimerEvent();
}
