//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "ACFCHandler.h"

#include <KPPPConfigurePacket.h>


#define ACFC_TYPE				0x8


ACFCHandler::ACFCHandler(uint32 options, PPPInterface& interface)
	: PPPOptionHandler("ACFC Handler", ACFC_TYPE, interface, NULL),
	fOptions(options),
	fLocalState(ACFC_DISABLED),
	fPeerState(ACFC_DISABLED)
{
}


status_t
ACFCHandler::AddToRequest(PPPConfigurePacket& request)
{
	// is ACFC not requested or was it rejected?
	if(fLocalState == ACFC_REJECTED
			|| (Options() & REQUEST_ACFC) == 0)
		return B_OK;
	
	// add ACFC request
	ppp_configure_item item;
	item.type = ACFC_TYPE;
	item.length = 2;
	return request.AddItem(&item) ? B_OK : B_ERROR;
}


status_t
ACFCHandler::ParseNak(const PPPConfigurePacket& nak)
{
	// naks do not contain ACFC items
	if(nak.ItemWithType(ACFC_TYPE))
		return B_ERROR;
	
	return B_OK;
}


status_t
ACFCHandler::ParseReject(const PPPConfigurePacket& reject)
{
	if(reject.ItemWithType(ACFC_TYPE)) {
		fLocalState = ACFC_REJECTED;
		
		if(Options() & FORCE_ACFC_REQUEST)
			return B_ERROR;
	}
	
	return B_OK;
}


status_t
ACFCHandler::ParseAck(const PPPConfigurePacket& ack)
{
	if(ack.ItemWithType(ACFC_TYPE))
		fLocalState = ACFC_ACCEPTED;
	else {
		fLocalState = ACFC_DISABLED;
		
		if(Options() & FORCE_ACFC_REQUEST)
			return B_ERROR;
	}
	
	return B_OK;
}


status_t
ACFCHandler::ParseRequest(const PPPConfigurePacket& request,
	int32 index, PPPConfigurePacket& nak, PPPConfigurePacket& reject)
{
	if(!request.ItemWithType(ACFC_TYPE))
		return B_OK;
	
	if((Options() & ALLOW_ACFC) == 0) {
		ppp_configure_item item;
		item.type = ACFC_TYPE;
		item.length = 2;
		return reject.AddItem(&item) ? B_OK : B_ERROR;
	}
	
	return B_OK;
}


status_t
ACFCHandler::SendingAck(const PPPConfigurePacket& ack)
{
	ppp_configure_item *item = ack.ItemWithType(ACFC_TYPE);
	
	if(item && (Options() & ALLOW_ACFC) == 0)
		return B_ERROR;
	
	if(item)
		fPeerState = ACFC_ACCEPTED;
	else
		fPeerState = ACFC_DISABLED;
	
	return B_OK;
}


void
ACFCHandler::Reset()
{
	fLocalState = fPeerState = ACFC_DISABLED;
}
