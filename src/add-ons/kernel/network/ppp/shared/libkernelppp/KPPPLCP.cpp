//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include <KPPPInterface.h>
#include <KPPPDevice.h>
#include <KPPPLCPExtension.h>
#include <KPPPOptionHandler.h>

#include <LockerHelper.h>

#include <netinet/in.h>
#include <core_funcs.h>
#include <sys/socket.h>


KPPPLCP::KPPPLCP(KPPPInterface& interface)
	: KPPPProtocol("LCP", PPP_ESTABLISHMENT_PHASE, PPP_LCP_PROTOCOL, PPP_PROTOCOL_LEVEL,
		AF_UNSPEC, 0, interface, NULL, PPP_ALWAYS_ALLOWED),
	fStateMachine(interface.StateMachine()),
	fTarget(NULL)
{
	SetUpRequested(false);
		// the state machine does everything for us
}


KPPPLCP::~KPPPLCP()
{
	while(CountOptionHandlers())
		delete OptionHandlerAt(0);
	while(CountLCPExtensions())
		delete LCPExtensionAt(0);
}


bool
KPPPLCP::AddOptionHandler(KPPPOptionHandler *optionHandler)
{
	if(!optionHandler || &optionHandler->Interface() != &Interface())
		return false;
	
	LockerHelper locker(StateMachine().fLock);
	
	if(Interface().Phase() != PPP_DOWN_PHASE || OptionHandlerFor(optionHandler->Type()))
		return false;
			// a running connection may not change and there may only be
			// one handler per option type
	
	return fOptionHandlers.AddItem(optionHandler);
}


bool
KPPPLCP::RemoveOptionHandler(KPPPOptionHandler *optionHandler)
{
	LockerHelper locker(StateMachine().fLock);
	
	if(Interface().Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	return fOptionHandlers.RemoveItem(optionHandler);
}


KPPPOptionHandler*
KPPPLCP::OptionHandlerAt(int32 index) const
{
	KPPPOptionHandler *optionHandler = fOptionHandlers.ItemAt(index);
	
	if(optionHandler == fOptionHandlers.GetDefaultItem())
		return NULL;
	
	return optionHandler;
}


KPPPOptionHandler*
KPPPLCP::OptionHandlerFor(uint8 type, int32 *start = NULL) const
{
	// The iteration style in this method is strange C/C++.
	// Explanation: I use this style because it makes extending all XXXFor
	// methods simpler as that they look very similar, now.
	
	int32 index = start ? *start : 0;
	
	if(index < 0)
		return NULL;
	
	KPPPOptionHandler *current = OptionHandlerAt(index);
	
	for(; current; current = OptionHandlerAt(++index)) {
		if(current->Type() == type) {
			if(start)
				*start = index;
			return current;
		}
	}
	
	return NULL;
}


bool
KPPPLCP::AddLCPExtension(KPPPLCPExtension *lcpExtension)
{
	if(!lcpExtension || &lcpExtension->Interface() != &Interface())
		return false;
	
	LockerHelper locker(StateMachine().fLock);
	
	if(Interface().Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	return fLCPExtensions.AddItem(lcpExtension);
}


bool
KPPPLCP::RemoveLCPExtension(KPPPLCPExtension *lcpExtension)
{
	LockerHelper locker(StateMachine().fLock);
	
	if(Interface().Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	return fLCPExtensions.RemoveItem(lcpExtension);
}


KPPPLCPExtension*
KPPPLCP::LCPExtensionAt(int32 index) const
{
	KPPPLCPExtension *lcpExtension = fLCPExtensions.ItemAt(index);
	
	if(lcpExtension == fLCPExtensions.GetDefaultItem())
		return NULL;
	
	return lcpExtension;
}


KPPPLCPExtension*
KPPPLCP::LCPExtensionFor(uint8 code, int32 *start = NULL) const
{
	// The iteration style in this method is strange C/C++.
	// Explanation: I use this style because it makes extending all XXXFor
	// methods simpler as that they look very similar, now.
	
	int32 index = start ? *start : 0;
	
	if(index < 0)
		return NULL;
	
	KPPPLCPExtension *current = LCPExtensionAt(index);
	
	for(; current; current = LCPExtensionAt(++index)) {
		if(current->Code() == code) {
			if(start)
				*start = index;
			return current;
		}
	}
	
	return NULL;
}


uint32
KPPPLCP::AdditionalOverhead() const
{
	uint32 overhead = Interface().Overhead();
	
	if(Target())
		overhead += Target()->Overhead();
	
	if(Interface().Device())
		overhead += Interface().Device()->Overhead();
	
	return overhead;
}


void
KPPPLCP::ProfileChanged()
{
	KPPPLCPExtension *extension;
	KPPPOptionHandler *handler;
	
	for(int32 index = 0; index < CountLCPExtensions(); index++) {
		extension = LCPExtensionAt(index);
		if(extension)
			extension->ProfileChanged();
	}
	
	for(int32 index = 0; index < CountOptionHandlers(); index++) {
		handler = OptionHandlerAt(index);
		if(handler)
			handler->ProfileChanged();
	}
}


bool
KPPPLCP::Up()
{
	return true;
}


bool
KPPPLCP::Down()
{
	return true;
}


status_t
KPPPLCP::Send(struct mbuf *packet, uint16 protocolNumber = PPP_LCP_PROTOCOL)
{
	if(Target())
		return Target()->Send(packet, PPP_LCP_PROTOCOL);
	else
		return Interface().Send(packet, PPP_LCP_PROTOCOL);
}


status_t
KPPPLCP::Receive(struct mbuf *packet, uint16 protocolNumber)
{
	if(!packet)
		return B_ERROR;
	
	if(protocolNumber != PPP_LCP_PROTOCOL) {
		dprintf("KPPPLCP::Receive(): wrong protocol number!\n");
		return PPP_UNHANDLED;
	}
	
	ppp_lcp_packet *data = mtod(packet, ppp_lcp_packet*);
	
	// remove padding
	int32 length = packet->m_len;
	if(packet->m_flags & M_PKTHDR)
		length = packet->m_pkthdr.len;
	length -= ntohs(data->length);
	if(length)
		m_adj(packet, -length);
	
	struct mbuf *copy = m_gethdr(MT_DATA);
	if(copy) {
		copy->m_data += AdditionalOverhead();
		copy->m_pkthdr.len = copy->m_len = packet->m_len;
		memcpy(copy->m_data, packet->m_data, copy->m_len);
	}
	
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
	
	packet = copy;
	
	if(!packet)
		return handled ? B_OK : B_ERROR;
	
	status_t result = B_OK;
	
	// Try to find LCP extensions that can handle this code.
	// We must duplicate the packet in order to ask all handlers.
	int32 index = 0;
	KPPPLCPExtension *lcpExtension = LCPExtensionFor(data->code, &index);
	for(; lcpExtension; lcpExtension = LCPExtensionFor(data->code, &(++index))) {
		if(!lcpExtension->IsEnabled())
			continue;
		
		result = lcpExtension->Receive(packet, data->code);
		
		// check return value and return it on error
		if(result == B_OK)
			handled = true;
		else if(result != PPP_UNHANDLED) {
			m_freem(packet);
			return result;
		}
	}
	
	if(!handled) {
		StateMachine().RUCEvent(packet, PPP_LCP_PROTOCOL, PPP_CODE_REJECT);
		return PPP_REJECTED;
	}
	
	m_freem(packet);
	
	return result;
}


void
KPPPLCP::Pulse()
{
	StateMachine().TimerEvent();
	
	for(int32 index = 0; index < CountLCPExtensions(); index++)
		LCPExtensionAt(index)->Pulse();
}
