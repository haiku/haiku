//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPLayer.h>

#include <cstdlib>
#include <cstring>
#include <core_funcs.h>


#ifdef _KERNEL_MODE
	#include <kernel_cpp.h>
	#define spawn_thread spawn_kernel_thread
	#define printf dprintf
#else
	#include <cstdio>
#endif


PPPLayer::PPPLayer(const char *name, ppp_level level, uint32 overhead)
	: fInitStatus(B_OK),
	fOverhead(overhead),
	fLevel(level),
	fNext(NULL)
{
	if(name)
		fName = strdup(name);
	else
		fName = strdup("Unknown");
}


PPPLayer::~PPPLayer()
{
	free(fName);
}


status_t
PPPLayer::InitCheck() const
{
	return fInitStatus;
}


status_t
PPPLayer::SendToNext(struct mbuf *packet, uint16 protocolNumber) const
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
		printf("PPPLayer: SendToNext() failed because there is no next handler!\n");
		m_freem(packet);
		return B_ERROR;
	}
}


void
PPPLayer::Pulse()
{
	// do nothing by default
}
