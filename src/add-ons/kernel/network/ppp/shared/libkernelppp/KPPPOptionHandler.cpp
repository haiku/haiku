/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*!	\class KPPPOptionHandler
	\brief Handler for LCP configure packets.
	
	This class can be used to extend the supported LCP configure packets.
*/

#include <KPPPOptionHandler.h>

#include <PPPControl.h>


/*!	\brief Constructor.
	
	If an error occurs in the constructor you should set \c fInitStatus.
	
	\param name Name of the handler.
	\param type Request type you can handle.
	\param interface Owning interface.
	\param settings Settings for this handler.
*/
KPPPOptionHandler::KPPPOptionHandler(const char *name, uint8 type,
	KPPPInterface& interface, driver_parameter *settings)
	:
	fInitStatus(B_OK),
	fType(type),
	fInterface(interface),
	fSettings(settings),
	fEnabled(true)
{
	if (name)
		fName = strdup(name);
	else
		fName = NULL;
}


//!	Destructor. Frees the name and unregisters handler from LCP protocol.
KPPPOptionHandler::~KPPPOptionHandler()
{
	Interface().LCP().RemoveOptionHandler(this);
	
	free(fName);
}


//!	Returns \c fInitStatus. May be overridden to return status-dependend errors.
status_t
KPPPOptionHandler::InitCheck() const
{
	return fInitStatus;
}


//!	Allows controlling this handler from userlevel.
status_t
KPPPOptionHandler::Control(uint32 op, void *data, size_t length)
{
	switch (op) {
		case PPPC_GET_SIMPLE_HANDLER_INFO:
		{
			if (length < sizeof(ppp_simple_handler_info_t) || !data)
				return B_ERROR;

			ppp_simple_handler_info *info = (ppp_simple_handler_info*) data;
			memset(info, 0, sizeof(ppp_simple_handler_info_t));
			if (Name())
				strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
			info->isEnabled = IsEnabled();
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
KPPPOptionHandler::StackControl(uint32 op, void *data)
{
	switch (op) {
		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


/*!	\brief Add request item.
	
	What you do here depends on the connection side (client or server). \n
	Received nak and reject packets influence which value gets added to the
	request, too.
*/
status_t
KPPPOptionHandler::AddToRequest(KPPPConfigurePacket& request)
{
	return B_OK;
}


/*!	\brief Parse a nak received from the peer.
	
	This method is called only once for each option handler. You must find the item
	yourself (no index is given).
*/
status_t
KPPPOptionHandler::ParseNak(const KPPPConfigurePacket& nak)
{
	return B_OK;
}


/*!	\brief Parse a reject received from the peer.
	
	This method is called only once for each option handler. You must find the item
	yourself (no index is given).
*/
status_t
KPPPOptionHandler::ParseReject(const KPPPConfigurePacket& reject)
{
	return B_OK;
}


/*!	\brief Parse an ack received from the peer.
	
	This method is called only once for each option handler. You must find the item
	yourself (no index is given).
*/
status_t
KPPPOptionHandler::ParseAck(const KPPPConfigurePacket& ack)
{
	return B_OK;
}


/*!	\brief Handler for configure requests sent by peer.
	
	Index may be behind the last item which means additional values can be
	appended.

	\param request The requested values.
	\param index Index of item in \a request.
	\param nak Values for the nak should be added here.
	\param reject Values for the reject should be added here.
*/
status_t
KPPPOptionHandler::ParseRequest(const KPPPConfigurePacket& request,
	int32 index, KPPPConfigurePacket& nak, KPPPConfigurePacket& reject)
{
	return B_OK;
}


//!	Notification that we ack the values in \a ack.
status_t
KPPPOptionHandler::SendingAck(const KPPPConfigurePacket& ack)
{
	return B_OK;
}


//!	Reset internal state (e.g.: remove list of rejected values).
void
KPPPOptionHandler::Reset()
{
	// do nothing by default
}
