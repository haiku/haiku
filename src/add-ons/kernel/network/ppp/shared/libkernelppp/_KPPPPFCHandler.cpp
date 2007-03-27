/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "_KPPPPFCHandler.h"

#include <KPPPConfigurePacket.h>


static const uint8 kPFCType = 0x7;


_KPPPPFCHandler::_KPPPPFCHandler(ppp_pfc_state& localPFCState,
		ppp_pfc_state& peerPFCState, KPPPInterface& interface)
	: KPPPOptionHandler("PFC Handler", kPFCType, interface, NULL),
	fLocalPFCState(localPFCState),
	fPeerPFCState(peerPFCState)
{
}


status_t
_KPPPPFCHandler::AddToRequest(KPPPConfigurePacket& request)
{
	// is PFC not requested or was it rejected?
	if (fLocalPFCState == PPP_PFC_REJECTED
			|| (Interface().PFCOptions() & PPP_REQUEST_PFC) == 0)
		return B_OK;
	
	// add PFC request
	ppp_configure_item item;
	item.type = kPFCType;
	item.length = 2;
	return request.AddItem(&item) ? B_OK : B_ERROR;
}


status_t
_KPPPPFCHandler::ParseNak(const KPPPConfigurePacket& nak)
{
	// naks do not contain PFC items
	if (nak.ItemWithType(kPFCType))
		return B_ERROR;
	
	return B_OK;
}


status_t
_KPPPPFCHandler::ParseReject(const KPPPConfigurePacket& reject)
{
	if (reject.ItemWithType(kPFCType)) {
		fLocalPFCState = PPP_PFC_REJECTED;
		
		if (Interface().PFCOptions() & PPP_FORCE_PFC_REQUEST)
			return B_ERROR;
	}
	
	return B_OK;
}


status_t
_KPPPPFCHandler::ParseAck(const KPPPConfigurePacket& ack)
{
	if (ack.ItemWithType(kPFCType))
		fLocalPFCState = PPP_PFC_ACCEPTED;
	else {
		fLocalPFCState = PPP_PFC_DISABLED;
		
		if (Interface().PFCOptions() & PPP_FORCE_PFC_REQUEST)
			return B_ERROR;
	}
	
	return B_OK;
}


status_t
_KPPPPFCHandler::ParseRequest(const KPPPConfigurePacket& request,
	int32 index, KPPPConfigurePacket& nak, KPPPConfigurePacket& reject)
{
	if (!request.ItemWithType(kPFCType))
		return B_OK;
	
	if ((Interface().PFCOptions() & PPP_ALLOW_PFC) == 0) {
		ppp_configure_item item;
		item.type = kPFCType;
		item.length = 2;
		return reject.AddItem(&item) ? B_OK : B_ERROR;
	}
	
	return B_OK;
}


status_t
_KPPPPFCHandler::SendingAck(const KPPPConfigurePacket& ack)
{
	ppp_configure_item *item = ack.ItemWithType(kPFCType);
	
	if (item && (Interface().PFCOptions() & PPP_ALLOW_PFC) == 0)
		return B_ERROR;
	
	if (item)
		fPeerPFCState = PPP_PFC_ACCEPTED;
	else
		fPeerPFCState = PPP_PFC_DISABLED;
	
	return B_OK;
}


void
_KPPPPFCHandler::Reset()
{
	fLocalPFCState = fPeerPFCState = PPP_PFC_DISABLED;
}
