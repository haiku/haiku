//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifdef _KERNEL_MODE
	#include <kernel_cpp.h>
#endif

#include <KPPPLayer.h>

#include <cstdlib>
#include <cstring>
#include <core_funcs.h>


KPPPLayer::KPPPLayer(const char *name, ppp_level level, uint32 overhead)
	: fInitStatus(B_OK),
	fOverhead(overhead),
	fLevel(level),
	fNext(NULL)
{
	SetName(name);
}


KPPPLayer::~KPPPLayer()
{
	free(fName);
}


status_t
KPPPLayer::InitCheck() const
{
	return fInitStatus;
}


status_t
KPPPLayer::SendToNext(struct mbuf *packet, uint16 protocolNumber) const
{
	if(!packet)
		return B_ERROR;
	
	// Find the next possible handler for this packet.
	// Normal protocols (Level() >= PPP_PROTOCOL_LEVEL) do not encapsulate anything.
	if(Next()) {
		if(Next()->IsAllowedToSend() && Next()->Level() < PPP_PROTOCOL_LEVEL)
			return Next()->Send(packet, protocolNumber);
		else
			return Next()->SendToNext(packet, protocolNumber);
	} else {
		dprintf("KPPPLayer: SendToNext() failed because there is no next handler!\n");
		m_freem(packet);
		return B_ERROR;
	}
}


void
KPPPLayer::Pulse()
{
	// do nothing by default
}


void
KPPPLayer::SetName(const char *name)
{
	free(fName);
	
	if(name)
		fName = strdup(name);
	else
		fName = strdup("Unknown");
}
