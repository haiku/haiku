/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*!	\class KPPPLCP
	\brief The LCP protocol.

	Every PPP interface \e must have an LCP protocol. It is used for establishing
	and terminating connections. \n
	When establishing a connecition the LCP protocol determines connection-specific
	settings like the packet MRU. These settings are handled by the KPPPOptionHandler
	class. Additional LCP codes like the PPP Multilink-Protocol uses them should
	be implemented through the KPPPLCPExtension class. \n
*/

#include <KPPPInterface.h>
#include <KPPPDevice.h>
#include <KPPPLCPExtension.h>
#include <KPPPOptionHandler.h>

#include <netinet/in.h>

#include <net_buffer.h>
#include <sys/socket.h>

extern net_buffer_module_info *gBufferModule;


//!	Creates a new LCP protocol for the given interface.
KPPPLCP::KPPPLCP(KPPPInterface& interface)
	:
	KPPPProtocol("LCP", PPP_ESTABLISHMENT_PHASE, PPP_LCP_PROTOCOL,
		PPP_PROTOCOL_LEVEL, AF_UNSPEC, 0, interface, NULL, PPP_ALWAYS_ALLOWED),
	fStateMachine(interface.StateMachine()),
	fTarget(NULL)
{
	SetUpRequested(false);
		// the state machine does everything for us
}


//!	Deletes all added option handlers and LCP extensions.
KPPPLCP::~KPPPLCP()
{
	while (CountOptionHandlers())
		delete OptionHandlerAt(0);
	while (CountLCPExtensions())
		delete LCPExtensionAt(0);
}


/*!	\brief Adds a new option handler.

	NOTE: You can only add option handlers in \c PPP_DOWN_PHASE. \n
	There may only be one handler per option type!
*/
bool
KPPPLCP::AddOptionHandler(KPPPOptionHandler *optionHandler)
{
	if (!optionHandler || &optionHandler->Interface() != &Interface())
		return false;

	if (Interface().Phase() != PPP_DOWN_PHASE
			|| OptionHandlerFor(optionHandler->Type()))
		return false;
			// a running connection may not change and there may only be
			// one handler per option type

	return fOptionHandlers.AddItem(optionHandler);
}


/*!	\brief Removes an option handler, but does not delete it.

	NOTE: You can only remove option handlers in \c PPP_DOWN_PHASE.
*/
bool
KPPPLCP::RemoveOptionHandler(KPPPOptionHandler *optionHandler)
{
	if (Interface().Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change

	return fOptionHandlers.RemoveItem(optionHandler);
}


//!	Returns the option handler at the given \a index.
KPPPOptionHandler*
KPPPLCP::OptionHandlerAt(int32 index) const
{
	dprintf("total optionhandler count %" B_PRId32 "\n", CountOptionHandlers());
	KPPPOptionHandler *optionHandler = fOptionHandlers.ItemAt(index);

	if (optionHandler == fOptionHandlers.GetDefaultItem())
		return NULL;

	return optionHandler;
}


//!	Returns the option handler that can handle options of a given \a type.
KPPPOptionHandler*
KPPPLCP::OptionHandlerFor(uint8 type, int32 *start) const
{
	// The iteration style in this method is strange C/C++.
	// Explanation: I use this style because it makes extending all XXXFor
	// methods simpler as that they look very similar, now.

	int32 index = start ? *start : 0;

	if (index < 0)
		return NULL;

	KPPPOptionHandler *current = OptionHandlerAt(index);

	for (; current; current = OptionHandlerAt(++index)) {
		if (current->Type() == type) {
			if (start)
				*start = index;
			return current;
		}
	}

	return NULL;
}


/*!	\brief Adds a new LCP extension.

	NOTE: You can only add LCP extensions in \c PPP_DOWN_PHASE.
*/
bool
KPPPLCP::AddLCPExtension(KPPPLCPExtension *lcpExtension)
{
	if (!lcpExtension || &lcpExtension->Interface() != &Interface())
		return false;

	if (Interface().Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change

	return fLCPExtensions.AddItem(lcpExtension);
}


/*!	\brief Removes an LCP extension, but does not delete it.

	NOTE: You can only remove LCP extensions in \c PPP_DOWN_PHASE.
*/
bool
KPPPLCP::RemoveLCPExtension(KPPPLCPExtension *lcpExtension)
{
	if (Interface().Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change

	return fLCPExtensions.RemoveItem(lcpExtension);
}


//!	Returns the LCP extension at the given \a index.
KPPPLCPExtension*
KPPPLCP::LCPExtensionAt(int32 index) const
{
	dprintf("LCPExtension count %" B_PRId32 "\n", CountLCPExtensions());
	KPPPLCPExtension *lcpExtension = fLCPExtensions.ItemAt(index);

	if (lcpExtension == fLCPExtensions.GetDefaultItem())
		return NULL;

	return lcpExtension;
}


//!	Returns the LCP extension that can handle LCP packets of a given \a code.
KPPPLCPExtension*
KPPPLCP::LCPExtensionFor(uint8 code, int32 *start) const
{
	// The iteration style in this method is strange C/C++.
	// Explanation: I use this style because it makes extending all XXXFor
	// methods simpler as that they look very similar, now.

	int32 index = start ? *start : 0;

	if (index < 0)
		return NULL;

	KPPPLCPExtension *current = LCPExtensionAt(index);

	for (; current; current = LCPExtensionAt(++index)) {
		if (current->Code() == code) {
			if (start)
				*start = index;
			return current;
		}
	}

	return NULL;
}


//!	Always returns \c true.
bool
KPPPLCP::Up()
{
	return true;
}


//!	Always returns \c true.
bool
KPPPLCP::Down()
{
	return true;
}


//!	Sends a packet to the target (if there is one) or to the interface.
status_t
KPPPLCP::Send(net_buffer *packet, uint16 protocolNumber)
{
	if (Target())
		return Target()->Send(packet, PPP_LCP_PROTOCOL);
	else
		return Interface().Send(packet, PPP_LCP_PROTOCOL);
}


//!	Decodes the LCP packet and passes it to the KPPPStateMachine or an LCP extension.
status_t
KPPPLCP::Receive(net_buffer *packet, uint16 protocolNumber)
{
	if (!packet)
		return B_ERROR;

	if (protocolNumber != PPP_LCP_PROTOCOL) {
		ERROR("KPPPLCP::Receive(): wrong protocol number!\n");
		return PPP_UNHANDLED;
	}

	NetBufferHeaderReader<ppp_lcp_packet> bufferHeader(packet);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	ppp_lcp_packet &data = bufferHeader.Data();

	// remove padding

	net_buffer *copy = gBufferModule->duplicate(packet);

	if (ntohs(data.length) < 4)
		return B_ERROR;

	bool handled = true;

	switch (data.code) {
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
			// gBufferModule->free(packet);
			handled = false;
	}

	packet = copy;

	if (!packet)
		return handled ? B_OK : B_ERROR;

	status_t result = B_OK;

	// Try to find LCP extensions that can handle this code.
	// We must duplicate the packet in order to ask all handlers.
	int32 index = 0;
	KPPPLCPExtension *lcpExtension = LCPExtensionFor(data.code, &index);
	for (; lcpExtension; lcpExtension = LCPExtensionFor(data.code, &(++index))) {
		if (!lcpExtension->IsEnabled())
			continue;

		result = lcpExtension->Receive(packet, data.code);

		// check return value and return it on error
		if (result == B_OK)
			handled = true;
		else if (result != PPP_UNHANDLED) {
			gBufferModule->free(packet);
			return result;
		}
	}

	if (!handled) {
		StateMachine().RUCEvent(packet, PPP_LCP_PROTOCOL, PPP_CODE_REJECT);
		return PPP_REJECTED;
	}

	gBufferModule->free(packet);

	return result;
}


//!	Calls \c Pulse() for each LCP extension.
void
KPPPLCP::Pulse()
{
	StateMachine().TimerEvent();

	for (int32 index = 0; index < CountLCPExtensions(); index++)
		LCPExtensionAt(index)->Pulse();
}
