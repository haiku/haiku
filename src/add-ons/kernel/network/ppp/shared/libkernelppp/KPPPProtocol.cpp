//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPInterface.h>
#include <KPPPUtils.h>
#include <PPPControl.h>
#include "settings_tools.h"

#include <cstring>


PPPProtocol::PPPProtocol(const char *name, ppp_phase activationPhase,
		uint16 protocolNumber, ppp_level level, int32 addressFamily,
		uint32 overhead, PPPInterface& interface,
		driver_parameter *settings, int32 flags = PPP_NO_FLAGS,
		const char *type = NULL, PPPOptionHandler *optionHandler = NULL)
	: PPPLayer(name, level),
	fActivationPhase(activationPhase),
	fProtocolNumber(protocolNumber),
	fAddressFamily(addressFamily),
	fInterface(interface),
	fSettings(settings),
	fFlags(flags),
	fOptionHandler(optionHandler),
	fNextProtocol(NULL),
	fEnabled(true),
	fUpRequested(true),
	fConnectionPhase(PPP_DOWN_PHASE)
{
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
	
	fInitStatus = interface.AddProtocol(this) && fInitStatus == B_OK ? B_OK : B_ERROR;
}


PPPProtocol::~PPPProtocol()
{
	Interface().RemoveProtocol(this);
	
	free(fType);
}


status_t
PPPProtocol::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		case PPPC_GET_PROTOCOL_INFO: {
			if(length < sizeof(ppp_protocol_info_t) || !data)
				return B_ERROR;
			
			ppp_protocol_info *info = (ppp_protocol_info*) data;
			memset(info, 0, sizeof(ppp_protocol_info_t));
			strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
			strncpy(info->type, Type(), PPP_HANDLER_NAME_LENGTH_LIMIT);
			info->settings = Settings();
			info->activationPhase = ActivationPhase();
			info->addressFamily = AddressFamily();
			info->flags = Flags();
			info->side = Side();
			info->level = Level();
			info->overhead = Overhead();
			info->connectionPhase = fConnectionPhase;
			info->protocolNumber = ProtocolNumber();
			info->isEnabled = IsEnabled();
			info->isUpRequested = IsUpRequested();
		} break;
		
		case PPPC_ENABLE:
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
PPPProtocol::SetEnabled(bool enabled = true)
{
	fEnabled = enabled;
	
	if(!enabled) {
		if(IsUp() || IsGoingUp())
			Down();
	} else if(!IsUp() && !IsGoingUp() && IsUpRequested() && Interface().IsUp())
		Up();
}


bool
PPPProtocol::IsAllowedToSend() const
{
	return IsEnabled() && IsUp() && IsProtocolAllowed(*this);
}


void
PPPProtocol::UpStarted()
{
	fConnectionPhase = PPP_ESTABLISHMENT_PHASE;
}


void
PPPProtocol::DownStarted()
{
	fConnectionPhase = PPP_TERMINATION_PHASE;
}


void
PPPProtocol::UpFailedEvent()
{
	fConnectionPhase = PPP_DOWN_PHASE;
	
	Interface().StateMachine().UpFailedEvent(this);
}


void
PPPProtocol::UpEvent()
{
	fConnectionPhase = PPP_ESTABLISHED_PHASE;
	
	Interface().StateMachine().UpEvent(this);
}


void
PPPProtocol::DownEvent()
{
	fConnectionPhase = PPP_DOWN_PHASE;
	
	Interface().StateMachine().DownEvent(this);
}
