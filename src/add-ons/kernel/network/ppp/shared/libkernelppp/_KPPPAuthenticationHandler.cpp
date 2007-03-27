/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "_KPPPAuthenticationHandler.h"

#include <KPPPConfigurePacket.h>
#include <KPPPDevice.h>

#include <netinet/in.h>


static const uint8 kAuthenticationType = 0x3;
static const char *kAuthenticatorTypeString = "Authenticator";

typedef struct authentication_item {
	uint8 type;
	uint8 length;
	uint16 protocolNumber;
} _PACKED authentication_item;


_KPPPAuthenticationHandler::_KPPPAuthenticationHandler(KPPPInterface& interface)
	: KPPPOptionHandler("Authentication Handler", kAuthenticationType, interface, NULL),
	fLocalAuthenticator(NULL),
	fPeerAuthenticator(NULL),
	fSuggestedLocalAuthenticator(NULL),
	fSuggestedPeerAuthenticator(NULL),
	fPeerAuthenticatorRejected(false)
{
}


KPPPProtocol*
_KPPPAuthenticationHandler::NextAuthenticator(const KPPPProtocol *start,
	ppp_side side) const
{
	// find the next authenticator for side, beginning at start
	KPPPProtocol *current = start ? start->NextProtocol() : Interface().FirstProtocol();
	
	for (; current; current = current->NextProtocol()) {
		if (current->Type() && !strcasecmp(current->Type(), kAuthenticatorTypeString)
				&& current->OptionHandler() && current->Side() == side)
			return current;
	}
	
	return NULL;
}


status_t
_KPPPAuthenticationHandler::AddToRequest(KPPPConfigurePacket& request)
{
	// AddToRequest(): Check if peer must authenticate itself and
	// add an authentication request if needed. This request is added
	// by the authenticator's OptionHandler.
	
	if (fPeerAuthenticator)
		fPeerAuthenticator->SetEnabled(false);
	if (fSuggestedPeerAuthenticator)
		fSuggestedPeerAuthenticator->SetEnabled(false);
	
	KPPPProtocol *authenticator;
	if (fPeerAuthenticatorRejected) {
		if (!fSuggestedPeerAuthenticator) {
			// This happens when the protocol is rejected, but no alternative
			// protocol is supplied to us or the suggested protocol is not supported.
			// We can use this chance to increase fPeerIndex to the next authenticator.
			authenticator = NextAuthenticator(fPeerAuthenticator, PPP_PEER_SIDE);
		} else
			authenticator = fSuggestedPeerAuthenticator;
		
		fPeerAuthenticatorRejected = false;
	} else {
		if (!fPeerAuthenticator) {
			// there is no authenticator selected, so find one for us
			authenticator = NextAuthenticator(fPeerAuthenticator, PPP_PEER_SIDE);
		} else
			authenticator = fPeerAuthenticator;
	}
	
	// check if all authenticators were rejected or if no authentication needed
	if (!authenticator) {
		if (fPeerAuthenticator)
			return B_ERROR;
				// all authenticators were denied
		else
			return B_OK;
				// no peer authentication needed
	}
	
	if (!authenticator || !authenticator->OptionHandler())
		return B_ERROR;
	
	fPeerAuthenticator = authenticator;
		// this could omit some authenticators when we get a suggestion, but that is
		// no problem because the suggested authenticator will be accepted (hopefully)
	
	TRACE("KPPPAuthHandler: AddToRequest(%X)\n", authenticator->ProtocolNumber());
	
	authenticator->SetEnabled(true);
	return authenticator->OptionHandler()->AddToRequest(request);
		// let protocol add its request
}


status_t
_KPPPAuthenticationHandler::ParseNak(const KPPPConfigurePacket& nak)
{
	// The authenticator's OptionHandler is not notified.
	
	authentication_item *item =
		(authentication_item*) nak.ItemWithType(kAuthenticationType);
	
	if (!item)
		return B_OK;
			// the request was not rejected
	if (item->length < 4)
		return B_ERROR;
	
	if (fSuggestedPeerAuthenticator) {
		fSuggestedPeerAuthenticator->SetEnabled(false);
		// if no alternative protocol is supplied we will choose a new one in
		// AddToRequest()
		if (ntohs(item->protocolNumber) ==
				fSuggestedPeerAuthenticator->ProtocolNumber()) {
			fSuggestedPeerAuthenticator = NULL;
			return B_OK;
		}
	}
	
	fPeerAuthenticatorRejected = true;
	KPPPProtocol *authenticator = Interface().ProtocolFor(ntohs(item->protocolNumber));
	if (authenticator && authenticator->Type()
			&& !strcasecmp(authenticator->Type(), kAuthenticatorTypeString)
			&& authenticator->OptionHandler())
		fSuggestedPeerAuthenticator = authenticator;
	else
		fSuggestedPeerAuthenticator = NULL;
	
	return B_OK;
}


status_t
_KPPPAuthenticationHandler::ParseReject(const KPPPConfigurePacket& reject)
{
	// an authentication request must not be rejected!
	if (reject.ItemWithType(kAuthenticationType))
		return B_ERROR;
	
	return B_OK;
}


status_t
_KPPPAuthenticationHandler::ParseAck(const KPPPConfigurePacket& ack)
{
	authentication_item *item =
		(authentication_item*) ack.ItemWithType(kAuthenticationType);
	
	if (!item) {
		if (fPeerAuthenticator)
			return B_ERROR;
				// the ack does not contain our request
		else
			return B_OK;
				// no authentication needed
	} else if (!fPeerAuthenticator
			|| ntohs(item->protocolNumber) != fPeerAuthenticator->ProtocolNumber())
		return B_ERROR;
			// this item was never requested
	
	return fPeerAuthenticator->OptionHandler()->ParseAck(ack);
		// this should enable the authenticator
}


status_t
_KPPPAuthenticationHandler::ParseRequest(const KPPPConfigurePacket& request,
	int32 index, KPPPConfigurePacket& nak, KPPPConfigurePacket& reject)
{
	if (fLocalAuthenticator)
		fLocalAuthenticator->SetEnabled(false);
	
	authentication_item *item = (authentication_item*) request.ItemAt(index);
	if (!item)
		return B_OK;
			// no authentication requested by peer (index > request.CountItems())
	
	TRACE("KPPPAuthHandler: ParseRequest(%X)\n", ntohs(item->protocolNumber));
	
	// try to find the requested protocol
	fLocalAuthenticator = Interface().ProtocolFor(ntohs(item->protocolNumber));
	if (fLocalAuthenticator && fLocalAuthenticator->Type()
			&& !strcasecmp(fLocalAuthenticator->Type(), kAuthenticatorTypeString)
			&& fLocalAuthenticator->OptionHandler())
		return fLocalAuthenticator->OptionHandler()->ParseRequest(request, index,
			nak, reject);
	
	// suggest another authentication protocol
	KPPPProtocol *nextAuthenticator =
		NextAuthenticator(fSuggestedLocalAuthenticator, PPP_LOCAL_SIDE);
	
	if (!nextAuthenticator) {
		if (!fSuggestedLocalAuthenticator) {
			// reject the complete authentication option
			reject.AddItem((ppp_configure_item*) item);
			return B_OK;
		} else
			nextAuthenticator = fSuggestedLocalAuthenticator;
				// try the old one again as it was not rejected until now
	}
	
	fSuggestedLocalAuthenticator = nextAuthenticator;
	fLocalAuthenticator = NULL;
		// no authenticator selected
	
	// nak this authenticator and suggest an alternative
	authentication_item suggestion;
	suggestion.type = kAuthenticationType;
	suggestion.length = 4;
	suggestion.protocolNumber = htons(nextAuthenticator->ProtocolNumber());
	return nak.AddItem((ppp_configure_item*) &suggestion) ? B_OK : B_ERROR;
}


status_t
_KPPPAuthenticationHandler::SendingAck(const KPPPConfigurePacket& ack)
{
	// do not insist on authenticating our side of the link ;)
	
	authentication_item *item =
		(authentication_item*) ack.ItemWithType(kAuthenticationType);
	
	if (!item)
		return B_OK;
			// no authentication needed
	
	fSuggestedLocalAuthenticator = NULL;
	
	if (!fLocalAuthenticator)
		return B_ERROR;
			// no authenticator selected (our suggestions must be requested, too)
	
	if (!fLocalAuthenticator)
		return B_ERROR;
	
	fLocalAuthenticator->SetEnabled(true);
	return fLocalAuthenticator->OptionHandler()->SendingAck(ack);
		// this should enable the authenticator
}


void
_KPPPAuthenticationHandler::Reset()
{
	if (fLocalAuthenticator) {
		fLocalAuthenticator->SetEnabled(false);
		fLocalAuthenticator->OptionHandler()->Reset();
	}
	if (fPeerAuthenticator) {
		fPeerAuthenticator->SetEnabled(false);
		fPeerAuthenticator->OptionHandler()->Reset();
	}
	if (fSuggestedPeerAuthenticator) {
		fSuggestedPeerAuthenticator->SetEnabled(false);
		fSuggestedPeerAuthenticator->OptionHandler()->Reset();
	}
	
	fLocalAuthenticator = fPeerAuthenticator = fSuggestedLocalAuthenticator =
		fSuggestedPeerAuthenticator = NULL;
	fPeerAuthenticatorRejected = false;
}
