//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPEncapsulator.h>
#include <KPPPUtils.h>

#include <PPPControl.h>


PPPEncapsulator::PPPEncapsulator(const char *name, PPP_PHASE phase,
		PPP_ENCAPSULATION_LEVEL level, uint16 protocol,
		int32 addressFamily, uint32 overhead,
		PPPInterface& interface, driver_parameter *settings,
		int32 flags = PPP_NO_FLAGS)
	: fOverhead(overhead), fPhase(phase), fLevel(level), fProtocol(protocol),
	fAddressFamily(addressFamily), fInterface(interface),
	fSettings(settings), fFlags(flags), fEnabled(true),
	fUpRequested(true), fConnectionStatus(PPP_DOWN_PHASE)
{
	if(name) {
		strncpy(fName, name, PPP_HANDLER_NAME_LENGTH_LIMIT);
		fName[PPP_HANDLER_NAME_LENGTH_LIMIT] = 0;
	} else
		strcpy(fName, "???");
	
	interface.AddEncapsulator(this);
}


PPPEncapsulator::~PPPEncapsulator()
{
	Interface().RemoveEncapsulator(this);
}


status_t
PPPEncapsulator::InitCheck() const
{
	if(!Settings())
		return B_ERROR;
	
	return B_OK;
}


void
PPPEncapsulator::SetEnabled(bool enabled = true)
{
	fEnabled = enabled;
	
	if(!enabled) {
		if(IsUp() || IsGoingUp())
			Down();
	} else if(!IsUp() && !IsGoingUp() && IsUpRequested() && Interface().IsUp())
		Up();
}


status_t
PPPEncapsulator::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		case PPPC_GET_HANDLER_INFO: {
			if(length < sizeof(ppp_handler_info_t) || !data)
				return B_ERROR;
			
			ppp_handler_info *info = (ppp_handler_info*) data;
			memset(info, 0, sizeof(ppp_handler_info_t));
			strcpy(info->name, Name());
			info->settings = Settings();
			info->phase = Phase();
			info->addressFamily = AddressFamily();
			info->flags = Flags();
			info->protocol = Protocol();
			info->isEnabled = IsEnabled();
			info->isUpRequested = IsUpRequested();
			info->connectionStatus = fConnectionStatus;
			info->level = Level();
			info->overhead = Overhead();
		} break;
		
		case PPPC_SET_ENABLED:
			if(length < sizeof(uint32) || !data)
				return B_ERROR;
			
			SetEnabled(*((uint32*)data));
		break;
		
		default:
			return PPP_UNHANDLED;
	}
	
	return B_OK;
}


status_t
PPPEncapsulator::SendToNext(struct mbuf *packet, uint16 protocol) const
{
	// Find the next possible handler for this packet.
	// This handler should be:
	// - enabled
	// - allowed to send
	if(Next()) {
		if(Next()->IsEnabled()
				&& is_handler_allowed(*Next(), Interface().State(), Interface().Phase()))
			return Next()->Send(packet, protocol);
		else
			return Next()->SendToNext(packet, protocol);
	} else
		return Interface().SendToDevice(packet, protocol);
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
	
	Interface().StateMachine().UpFailedEvent(this);
}


void
PPPEncapsulator::UpEvent()
{
	fConnectionStatus = PPP_ESTABLISHED_PHASE;
	
	Interface().StateMachine().UpEvent(this);
}


void
PPPEncapsulator::DownEvent()
{
	fConnectionStatus = PPP_DOWN_PHASE;
	
	Interface().StateMachine().DownEvent(this);
}
