//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPInterface.h>

#include <PPPControl.h>

#include <cstring>


PPPProtocol::PPPProtocol(const char *name, ppp_phase phase, uint16 protocol,
		int32 addressFamily, PPPInterface& interface,
		driver_parameter *settings, int32 flags = PPP_NO_FLAGS,
		ppp_authenticator_type authenticatorType = PPP_NO_AUTHENTICATOR)
	: fPhase(phase),
	fProtocol(protocol),
	fAddressFamily(addressFamily),
	fInterface(interface),
	fSettings(settings),
	fFlags(flags),
	fAuthenticatorType(authenticatorType),
	fEnabled(true),
	fUpRequested(true),
	fConnectionStatus(PPP_DOWN_PHASE)
{
	if(name) {
		strncpy(fName, name, PPP_HANDLER_NAME_LENGTH_LIMIT);
		fName[PPP_HANDLER_NAME_LENGTH_LIMIT] = 0;
	} else
		strcpy(fName, "???");
	
	if(authenticatorType != PPP_NO_AUTHENTICATOR)
		SetEnabled(false);
			// only the active authenticator should be enabled
	
	fInitStatus = interface.AddProtocol(this) ? B_OK : B_ERROR;
}


PPPProtocol::~PPPProtocol()
{
	Interface().RemoveProtocol(this);
}


status_t
PPPProtocol::InitCheck() const
{
	return fInitStatus;
}


void
PPPProtocol::SetEnabled(bool enabled = true)
{
	fEnabled = enabled;
	
	if(!enabled) {
		if(IsUp() || IsGoingUp())
			Down();
	} else if(!IsUp() && !IsGoingUp() && IsUpRequested() && Interface().IsUp())
		Up();
}


status_t
PPPProtocol::Control(uint32 op, void *data, size_t length)
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
			info->authenticatorType = AuthenticatorType();
		} break;
		
		case PPPC_SET_ENABLED:
			if(length < sizeof(uint32) || !data)
				return B_ERROR;
			
			SetEnabled(*((uint32*)data));
		break;
		
		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


status_t
PPPProtocol::SetupDialOnDemand()
{
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
	
	Interface().StateMachine().UpFailedEvent(this);
}


void
PPPProtocol::UpEvent()
{
	fConnectionStatus = PPP_ESTABLISHED_PHASE;
	
	Interface().StateMachine().UpEvent(this);
}


void
PPPProtocol::DownEvent()
{
	fConnectionStatus = PPP_DOWN_PHASE;
	
	Interface().StateMachine().DownEvent(this);
}
