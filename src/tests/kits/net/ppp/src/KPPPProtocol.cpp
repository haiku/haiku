//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPInterface.h>

#include <cstring>


PPPProtocol::PPPProtocol(const char *name, PPP_PHASE phase, uint16 protocol,
		int32 addressFamily, PPPInterface *interface,
		driver_parameter *settings, int32 flags = PPP_NO_FLAGS)
	: fPhase(phase), fProtocol(protocol), fAddressFamily(addressFamily),
	fInterface(interface), fSettings(settings), fFlags(flags),
	fEnabled(true), fUpRequested(true), fConnectionStatus(PPP_DOWN_PHASE)
{
	fName = name ? strdup(name) : NULL;
	
	if(interface)
		interface->AddProtocol(this);
}


PPPProtocol::~PPPProtocol()
{
	free(fName);
	
	if(Interface())
		Interface()->RemoveProtocol(this);
}


status_t
PPPProtocol::InitCheck() const
{
	if(!Interface() || !Settings())
		return B_ERROR;
	
	return B_OK;
}


void
PPPProtocol::SetEnabled(bool enabled = true)
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
PPPProtocol::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		// TODO:
		// get:
		// - name
		// - protocol, address family
		// - status (Is(Going)Up/Down/UpRequested/Enabled)
		// set:
		// - enabled
		
		default:
			return B_ERROR;
	}
	
	return B_OK;
}


void
PPPProtocol::Pulse()
{
	// do nothing by default
}


void
PPPProtocol::UpStarted()
{
	fConnectionStatus = PPP_ESTABLISHMENT_PHASE;
}


void
PPPProtocol::DownStarted()
{
	fConnectionStatus = PPP_TERMINATION_PHASE;
}


void
PPPProtocol::UpFailedEvent()
{
	fConnectionStatus = PPP_DOWN_PHASE;
	
	if(!Interface())
		return;
	
	Interface()->StateMachine().UpFailedEvent(this);
}


void
PPPProtocol::UpEvent()
{
	fConnectionStatus = PPP_ESTABLISHED_PHASE;
	
	if(!Interface())
		return;
	
	Interface()->StateMachine().UpEvent(this);
}


void
PPPProtocol::DownEvent()
{
	fConnectionStatus = PPP_DOWN_PHASE;
	
	if(!Interface())
		return;
	
	Interface()->StateMachine().DownEvent(this);
}
