/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*!	\class KPPPProtocol.
	\brief Represents a PPP protocol object.
	
	This class is the base for all protocols, encapsulators, and authenticators.
*/

#include <KPPPInterface.h>
#include <KPPPUtils.h>
#include <PPPControl.h>
#include "settings_tools.h"

#include <cstring>


/*!	\brief Constructs a PPP protocol.
	
	If you are creating a normal protocol use a level of \c PPP_PROTOCOL_LEVEL.
	Encapsulators like compression protocols should use a different level. \n
	Authenticators are identified by a type string equal to "Authenticator" and they
	have an optionHandler that implements only the following methods:
	- AddToRequest
	- ParseRequest
	
	\param name The protocol name.
	\param activationPhase Our activation phase.
	\param protocolNumber Our protocol number.
	\param level The level at which we get inserted into the list of protocols.
	\param addressFamily The address family.  Values < 0 and > 0xFF are ignored.
	\param overhead The protocol's header size.
	\param interface The owner.
	\param settings Our settings.
	\param flags Optional flags. See \c ppp_protocol_flags for more information.
	\param type Optional type string. Used by authenticators, for example.
	\param optionHandler Optional handler associated with this protocol.
*/
KPPPProtocol::KPPPProtocol(const char *name, ppp_phase activationPhase,
	uint16 protocolNumber, ppp_level level, int32 addressFamily,
	uint32 overhead, KPPPInterface& interface,
	driver_parameter *settings, int32 flags,
	const char *type, KPPPOptionHandler *optionHandler)
	:
	KPPPLayer(name, level, overhead),
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
	if (type)
		fType = strdup(type);
	else
		fType = NULL;
	
	const char *sideString = get_parameter_value("side", settings);
	if (sideString)
		fSide = get_side_string_value(sideString, PPP_LOCAL_SIDE);
	else {
		if (interface.Mode() == PPP_CLIENT_MODE)
			fSide = PPP_LOCAL_SIDE;
		else
			fSide = PPP_PEER_SIDE;
	}
}


//!	Removes this protocol from the interface and frees the type.
KPPPProtocol::~KPPPProtocol()
{
	Interface().RemoveProtocol(this);
	free(fType);
}


/*!	\brief This should uninit the protocol before it gets deleted.
	
	You may want to remove all routes of this protocol.
*/
void
KPPPProtocol::Uninit()
{
	// do nothing by default
}


/*!	\brief Allows private extensions.
	
	If you override this method you must call the parent's method for unknown ops.
*/
status_t
KPPPProtocol::Control(uint32 op, void *data, size_t length)
{
	switch (op) {
		case PPPC_GET_PROTOCOL_INFO:
		{
			if (length < sizeof(ppp_protocol_info_t) || !data)
				return B_ERROR;
			
			ppp_protocol_info *info = (ppp_protocol_info*) data;
			memset(info, 0, sizeof(ppp_protocol_info_t));
			if (Name())
				strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
			if (Type())
				strncpy(info->type, Type(), PPP_HANDLER_NAME_LENGTH_LIMIT);
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
			break;
		}

		case PPPC_ENABLE:
			if (length < sizeof(uint32) || !data)
				return B_ERROR;
			
			SetEnabled(*((uint32*)data));
			break;

		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


//!	Stack ioctl handler.
status_t
KPPPProtocol::StackControl(uint32 op, void *data)
{
	switch (op) {
		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


/*!	\brief Enables or disables this protocol.
	
	A disabled protocol is ignored and Up() is not called!
*/
void
KPPPProtocol::SetEnabled(bool enabled)
{
	fEnabled = enabled;
	
	if (!enabled) {
		if (IsUp() || IsGoingUp())
			Down();
	} else if (!IsUp() && !IsGoingUp() && IsUpRequested() && Interface().IsUp())
		Up();
}


//!	Returns if this protocol is allowed to send a packet (enabled, up, etc.).
bool
KPPPProtocol::IsAllowedToSend() const
{
	return IsEnabled() && IsUp() && IsProtocolAllowed(*this);
}


/*!	\brief Report that the protocol is going up.
	
	Called by Up(). \n
	From now on, the connection attempt can may be aborted by calling Down().
*/
void
KPPPProtocol::UpStarted()
{
	fConnectionPhase = PPP_ESTABLISHMENT_PHASE;
}


/*!	\brief Report that the protocol is going down.
	
	Called by Down().
*/
void
KPPPProtocol::DownStarted()
{
	fConnectionPhase = PPP_TERMINATION_PHASE;
}


/*!	\brief Reports that we failed going up. May only be called after Up() was called.
	
	Authenticators \e must call the state machine's authentication notification first!
*/
void
KPPPProtocol::UpFailedEvent()
{
	fConnectionPhase = PPP_DOWN_PHASE;
	Interface().StateMachine().UpFailedEvent(this);
}


/*!	\brief Reports that we went up. May only be called after Up() was called.
	
	Authenticators \e must call the state machine's authentication notification first!
*/
void
KPPPProtocol::UpEvent()
{
	fConnectionPhase = PPP_ESTABLISHED_PHASE;
	Interface().StateMachine().UpEvent(this);
}


/*!	\brief Reports that we went down. This may be called to indicate connection loss.
	
	Authenticators \e must call the state machine's authentication notification first!
*/
void
KPPPProtocol::DownEvent()
{
	fConnectionPhase = PPP_DOWN_PHASE;
	Interface().StateMachine().DownEvent(this);
}
