/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*!	\class KPPPLayer
	\brief An abstract layer that can encapsulate/send and receive packets.
	
	All packet handlers should derive from this class. It does not define a protocol
	number like KPPPProtocol does. It only has a header overhead and an encapsulation
	level that is used to determine the order at which the packets get encapsulated
	by the layers. If this layer does not encapsulate PPP packets you should use
	PPP_PROTOCOL_LEVEL.
*/

#ifdef _KERNEL_MODE
	#include <kernel_cpp.h>
#endif

#include <KPPPLayer.h>

#include <cstdlib>
#include <cstring>
#include <core_funcs.h>


/*!	\brief Creates a new layer.
	
	If an error occurs in the constructor you should set \c fInitStatus.
*/
KPPPLayer::KPPPLayer(const char *name, ppp_level level, uint32 overhead)
	: fInitStatus(B_OK),
	fOverhead(overhead),
	fName(NULL),
	fLevel(level),
	fNext(NULL)
{
	SetName(name);
}


//!	Only frees the name.
KPPPLayer::~KPPPLayer()
{
	free(fName);
}


//!	Returns \c fInitStatus. May be overridden to return status-dependend errors.
status_t
KPPPLayer::InitCheck() const
{
	return fInitStatus;
}


//!	Sends a packet to the next layer in the chain.
status_t
KPPPLayer::SendToNext(struct mbuf *packet, uint16 protocolNumber) const
{
	if (!packet)
		return B_ERROR;
	
	// Find the next possible handler for this packet.
	// Normal protocols (Level() >= PPP_PROTOCOL_LEVEL) do not encapsulate anything.
	if (Next()) {
		if (Next()->IsAllowedToSend() && Next()->Level() < PPP_PROTOCOL_LEVEL)
			return Next()->Send(packet, protocolNumber);
		else
			return Next()->SendToNext(packet, protocolNumber);
	} else {
		ERROR("KPPPLayer: SendToNext() failed because there is no next handler!\n");
		m_freem(packet);
		return B_ERROR;
	}
}


/*!	\brief You may override this for periodic tasks.
	
	This method gets called every \c PPP_PULSE_RATE microseconds.
*/
void
KPPPLayer::Pulse()
{
	// do nothing by default
}


//!	Allows changing the name of this layer.
void
KPPPLayer::SetName(const char *name)
{
	free(fName);
	
	if (name)
		fName = strdup(name);
	else
		fName = NULL;
}
