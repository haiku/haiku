//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPEncapsulator.h>


PPPEncapsulator::PPPEncapsulator(const char *name, PPP_PHASE phase,
		PPP_ENCAPSULATION_LEVEL level, uint16 protocol,
		int32 addressFamily, uint32 overhead,
		PPPInterface *interface, driver_parameter *settings,
		int32 flags = PPP_NO_FLAGS)
	: fOverhead(overhead), fPhase(phase), fLevel(level), fProtocol(protocol),
	fAddressFamily(addressFamily), fInterface(interface),
	fSettings(settings), fFlags(flags), fEnabled(true),
	fUpRequested(true), fConnectionStatus(PPP_DOWN_PHASE)
{
	fName = name ? strdup(name) : NULL;
	
	if(interface)
		interface->AddEncapsulator(this);
}


PPPEncapsulator::~PPPEncapsulator()
{
	free(fName);
	
	if(Interface())
		Interface()->RemoveEncapsulator(this);
}


status_t
PPPEncapsulator::InitCheck() const
{
	if(!Interface() || !Settings())
		return B_ERROR;
	
	return B_OK;
}


void
PPPEncapsulator::SetEnabled(bool enabled = true)
{
	fEnabled = enabled;
	
	if(!Interface())
		return;
	
	if(!enabled) {
		if(IsUp() || IsGoingUp())
			Down();
	} else if(!IsUp() && !IsGoingUp() && IsUpRequested() && Interface()->IsUp())
		Up();
}


status_t
PPPEncapsulator::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		// TODO:
		// get:
		// - name
		// - level, protocol, address family, overhead
		// - status (Is(Going)Up/Down/UpRequested/Enabled)
		// set:
		// - enabled
		
		default:
			return B_ERROR;
	}
	
	return B_OK;
}


status_t
PPPEncapsulator::SendToNext(struct mbuf *packet, uint16 protocol) const
{
	if(Next())
		return Next()->Send(packet, protocol);
	else if(Interface())
		return Interface()->SendToDevice(packet, protocol);
	else
		return B_ERROR;
}


void
PPPEncapsulator::Pulse()
{
	// do nothing by default
}


void
PPPEncapsulator::UpStarted()
{
	fConnectionStatus = PPP_ESTABLISHMENT_PHASE;
}


void
PPPEncapsulator::DownStarted()
{
	fConnectionStatus = PPP_TERMINATION_PHASE;
}


void
PPPEncapsulator::UpFailedEvent()
{
	fConnectionStatus = PPP_DOWN_PHASE;
	
	if(!Interface())
		return;
	
	Interface()->StateMachine().UpFailedEvent(this);
}


void
PPPEncapsulator::UpEvent()
{
	fConnectionStatus = PPP_ESTABLISHED_PHASE;
	
	if(!Interface())
		return;
	
	Interface()->StateMachine().UpEvent(this);
}


void
PPPEncapsulator::DownEvent()
{
	fConnectionStatus = PPP_DOWN_PHASE;
	
	if(!Interface())
		return;
	
	Interface()->StateMachine().DownEvent(this);
}
