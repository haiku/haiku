//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include "_KPPPPFCHandler.h"

#include <KPPPConfigurePacket.h>


#define PFC_TYPE				0x7


_PPPPFCHandler::_PPPPFCHandler(ppp_pfc_state& localPFCState,
		ppp_pfc_state& peerPFCState, PPPInterface& interface)
	: PPPOptionHandler("PFC Handler", PFC_TYPE, interface, NULL),
	fLocalPFCState(localPFCState),
	fPeerPFCState(peerPFCState)
{
}


status_t
_PPPPFCHandler::AddToRequest(PPPConfigurePacket& request)
{
	// is PFC not requested or was it rejected?
	if(fLocalPFCState == PPP_PFC_REJECTED
			|| (Interface().PFCOptions() & PPP_REQUEST_PFC) == 0)
		return B_OK;
	
	// add PFC request
	ppp_configure_item item;
	item.type = PFC_TYPE;
	item.length = 2;
	return request.AddItem(&item) ? B_OK : B_ERROR;
}


status_t
_PPPPFCHandler::ParseNak(const PPPConfigurePacket& nak)
{
	// naks do not contain PFC items
	if(nak.ItemWithType(PFC_TYPE))
		return B_ERROR;
	
	return B_OK;
}


status_t
_PPPPFCHandler::ParseReject(const PPPConfigurePacket& reject)
{
	if(reject.ItemWithType(PFC_TYPE)) {
		fLocalPFCState = PPP_PFC_REJECTED;
		
		if(Interface().PFCOptions() & PPP_FORCE_PFC_REQUEST)
			return B_ERROR;
	}
	
	return B_OK;
}


status_t
_PPPPFCHandler::ParseAck(const PPPConfigurePacket& ack)
{
	if(ack.ItemWithType(PFC_TYPE))
		fLocalPFCState = PPP_PFC_ACCEPTED;
	else {
		fLocalPFCState = PPP_PFC_DISABLED;
		
		if(Interface().PFCOptions() & PPP_FORCE_PFC_REQUEST)
			return B_ERROR;
	}
	
	return B_OK;
}


status_t
_PPPPFCHandler::ParseRequest(const PPPConfigurePacket& request,
	int32 index, PPPConfigurePacket& nak, PPPConfigurePacket& reject)
{
	if(!request.ItemWithType(PFC_TYPE))
		return B_OK;
	
	if((Interface().PFCOptions() & PPP_ALLOW_PFC) == 0) {
		ppp_configure_item item;
		item.type = PFC_TYPE;
		item.length = 2;
		return reject.AddItem(&item) ? B_OK : B_ERROR;
	}
	
	return B_OK;
}


status_t
_PPPPFCHandler::SendingAck(const PPPConfigurePacket& ack)
{
	ppp_configure_item *item = ack.ItemWithType(PFC_TYPE);
	
	if(item && (Interface().PFCOptions() & PPP_ALLOW_PFC) == 0)
		return B_ERROR;
	
	if(item)
		fPeerPFCState = PPP_PFC_ACCEPTED;
	else
		fPeerPFCState = PPP_PFC_DISABLED;
	
	return B_OK;
}


void
_PPPPFCHandler::Reset()
{
	fLocalPFCState = fPeerPFCState = PPP_PFC_DISABLED;
}
