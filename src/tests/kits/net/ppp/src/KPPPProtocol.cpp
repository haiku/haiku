//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPInterface.h>
#include <KPPPOptionHandler.h>

#include <PPPControl.h>
#include "settings_tools.h"

#include <cstring>


PPPProtocol::PPPProtocol(const char *name, ppp_phase phase, uint16 protocol,
		int32 addressFamily, PPPInterface& interface,
		driver_parameter *settings, int32 flags = PPP_NO_FLAGS,
		const char *type = NULL, PPPOptionHandler *optionHandler = NULL)
	: fPhase(phase),
	fProtocol(protocol),
	fAddressFamily(addressFamily),
	fInterface(interface),
	fSettings(settings),
	fFlags(flags),
	fOptionHandler(optionHandler),
	fEnabled(true),
	fUpRequested(true),
	fConnectionStatus(PPP_DOWN_PHASE)
{
	if(name)
		fName = strdup(name);
	else
		fName = strdup("Unknown");
	
	if(type)
		fType = strdup(type);
	else
		fType = strdup("Unknown");
	
	const char *sideString = get_parameter_value("side", settings);
	if(sideString)
		fSide = get_side_string_value(sideString, PPP_LOCAL_SIDE);
	else {
		if(interface.Mode() == PPP_CLIENT_MODE)
			fSide = PPP_LOCAL_SIDE;
		else
			fSide = PPP_PEER_SIDE;
	}
	
	fInitStatus = interface.AddProtocol(this) ? B_OK : B_ERROR;
}


PPPProtocol::~PPPProtocol()
{
	free(fName);
	free(fType);
	
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
			strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
			info->settings = Settings();
			info->phase = Phase();
			info->addressFamily = AddressFamily();
			info->flags = Flags();
			info->side = Side();
			info->protocol = Protocol();
			info->isEnabled = IsEnabled();
			info->isUpRequested = IsUpRequested();
			info->connectionStatus = fConnectionStatus;
			strncpy(info->type, Type(), PPP_HANDLER_NAME_LENGTH_LIMIT);
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
PPPProtocol::StackControl(uint32 op, void *data)
{
	switch(op) {
		default:
			return B_BAD_VALUE;
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
