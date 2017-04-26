/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "ACFCHandler.h"

#include <KPPPConfigurePacket.h>


static const uint8 kACFCType = 0x8;


ACFCHandler::ACFCHandler(uint32 options, KPPPInterface& interface)
	: KPPPOptionHandler("ACFC Handler", kACFCType, interface, NULL),
	fOptions(options),
	fLocalState(ACFC_DISABLED),
	fPeerState(ACFC_DISABLED)
{
}


status_t
ACFCHandler::AddToRequest(KPPPConfigurePacket& request)
{
	// is ACFC not requested or was it rejected?
	if (fLocalState == ACFC_REJECTED
			|| (Options() & REQUEST_ACFC) == 0)
		return B_OK;

	// add ACFC request
	ppp_configure_item item;
	item.type = kACFCType;
	item.length = 2;
	return request.AddItem(&item) ? B_OK : B_ERROR;
}


status_t
ACFCHandler::ParseNak(const KPPPConfigurePacket& nak)
{
	// naks do not contain ACFC items
	if (nak.ItemWithType(kACFCType))
		return B_ERROR;

	return B_OK;
}


status_t
ACFCHandler::ParseReject(const KPPPConfigurePacket& reject)
{
	if (reject.ItemWithType(kACFCType)) {
		fLocalState = ACFC_REJECTED;

		if (Options() & FORCE_ACFC_REQUEST)
			return B_ERROR;
	}

	return B_OK;
}


status_t
ACFCHandler::ParseAck(const KPPPConfigurePacket& ack)
{
	if (ack.ItemWithType(kACFCType))
		fLocalState = ACFC_ACCEPTED;
	else {
		fLocalState = ACFC_DISABLED;

		if (Options() & FORCE_ACFC_REQUEST)
			return B_ERROR;
	}

	return B_OK;
}


status_t
ACFCHandler::ParseRequest(const KPPPConfigurePacket& request,
	int32 index, KPPPConfigurePacket& nak, KPPPConfigurePacket& reject)
{
	if (!request.ItemWithType(kACFCType))
		return B_OK;

	if ((Options() & ALLOW_ACFC) == 0) {
		ppp_configure_item item;
		item.type = kACFCType;
		item.length = 2;
		return reject.AddItem(&item) ? B_OK : B_ERROR;
	}

	return B_OK;
}


status_t
ACFCHandler::SendingAck(const KPPPConfigurePacket& ack)
{
	ppp_configure_item *item = ack.ItemWithType(kACFCType);

	if (item && (Options() & ALLOW_ACFC) == 0)
		return B_ERROR;

	if (item)
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
