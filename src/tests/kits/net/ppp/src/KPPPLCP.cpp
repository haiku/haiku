//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPInterface.h>
#include <KPPPDevice.h>
#include <KPPPEncapsulator.h>
#include <KPPPLCPExtension.h>
#include <KPPPOptionHandler.h>

#include <LockerHelper.h>

#include <netinet/in.h>
#include <mbuf.h>
#include <sys/socket.h>


#define PPP_PROTOCOL_OVERHEAD				2


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


bool
PPPLCP::AddLCPExtension(PPPLCPExtension *extension)
{
	if(!extension)
		return false;
	
	LockerHelper locker(StateMachine().Locker());
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	return fLCPExtensions.AddItem(extension);
}


bool
PPPLCP::RemoveLCPExtension(PPPLCPExtension *extension)
{
	LockerHelper locker(StateMachine().Locker());
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	return fLCPExtensions.RemoveItem(extension);
}


PPPLCPExtension*
PPPLCP::LCPExtensionAt(int32 index) const
{
	PPPLCPExtension *extension = fLCPExtensions.ItemAt(index);
	
	if(extension == fLCPExtensions.GetDefaultItem())
		return NULL;
	
	return extension;
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
	if(!packet)
		return B_ERROR;
	
	if(protocol != PPP_LCP_PROTOCOL)
		return PPP_UNHANDLED;
	
	struct mbuf *copy = m_gethdr(MT_DATA);
	if(copy) {
		copy->m_data += AdditionalOverhead();
		copy->m_len = packet->m_len;
		memcpy(copy->m_data, packet->m_data, copy->m_len);
	}
	
	ppp_lcp_packet *data = mtod(packet, ppp_lcp_packet*);
	
	if(ntohs(data->length) < 4)
		return B_ERROR;
	
	bool handled = true;
	
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
		case PPP_ECHO_REPLY:
		case PPP_DISCARD_REQUEST:
			StateMachine().RXREvent(packet);
		break;
		
		default:
			m_freem(packet);
			handled = false;
	}
	
	if(!copy)
		return handled ? B_OK : B_ERROR;
	
	packet = copy;
	copy = NULL;
	
	status_t result = B_OK;
	
	// Try to find LCP extensions that can handle this code.
	// We must duplicate the packet in order to ask all handlers.
	PPPLCPExtension *extension;
	for(int32 index = 0; index < CountLCPExtensions(); index++) {
		extension = LCPExtensionAt(index);
		if(extension->IsEnabled() && extension->Code() == data->code) {
			if(!copy) {
				copy = m_gethdr(MT_DATA);
				if(!copy)
					return B_ERROR;
				
				copy->m_data += AdditionalOverhead();
				copy->m_len = packet->m_len;
				memcpy(copy->m_data, packet->m_data, copy->m_len);
			}
			
			result = extension->Receive(copy, data->code);
			
			if(result == B_OK) {
				copy = NULL;
				handled = true;
			} else if(result != PPP_UNHANDLED)
				return result;
		}
	}
	
	if(copy)
		m_freem(copy);
	
	if(!handled) {
		StateMachine().RUCEvent(packet, PPP_LCP_PROTOCOL, PPP_CODE_REJECT);
		return PPP_REJECTED;
	}
	
	if(packet)
		m_freem(packet);
	
	return result;
}


void
PPPLCP::Pulse()
{
	StateMachine().TimerEvent();
	
	for(int32 index = 0; index < CountLCPExtensions(); index++)
		LCPExtensionAt(index)->Pulse();
}
