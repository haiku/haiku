//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include "_KPPPAuthenticationHandler.h"

#include <KPPPConfigurePacket.h>
#include <KPPPDevice.h>

#include <netinet/in.h>


#define AUTHENTICATION_TYPE				0x3
#define AUTHENTICATOR_TYPE_STRING		"Authenticator"

typedef struct authentication_item {
	uint8 type;
	uint8 length;
	uint16 protocolNumber _PACKED;
} authentication_item;


_PPPAuthenticationHandler::_PPPAuthenticationHandler(PPPInterface& interface)
	: PPPOptionHandler("Authentication Handler", AUTHENTICATION_TYPE, interface, NULL),
	fLocalAuthenticator(NULL),
	fPeerAuthenticator(NULL),
	fSuggestedLocalAuthenticator(NULL),
	fSuggestedPeerAuthenticator(NULL),
	fPeerAuthenticatorRejected(false)
{
}


PPPProtocol*
_PPPAuthenticationHandler::NextAuthenticator(const PPPProtocol *start,
	ppp_side side) const
{
	// find the next authenticator for side, beginning at start
	PPPProtocol *current = start ? start->NextProtocol() : Interface().FirstProtocol();
	
	for(; current; current = current->NextProtocol()) {
		if(!strcasecmp(current->Type(), AUTHENTICATOR_TYPE_STRING)
				&& current->OptionHandler() && current->Side() == side)
			return current;
	}
	
	return NULL;
}


status_t
_PPPAuthenticationHandler::AddToRequest(PPPConfigurePacket& request)
{
	// AddToRequest(): Check if peer must authenticate itself and
	// add an authentication request if needed. This request is added
	// by the authenticator's OptionHandler.
	
	if(fPeerAuthenticator)
		fPeerAuthenticator->SetEnabled(false);
	if(fSuggestedPeerAuthenticator)
		fSuggestedPeerAuthenticator->SetEnabled(false);
	
	PPPProtocol *authenticator;
	if(fPeerAuthenticatorRejected) {
		if(!fSuggestedPeerAuthenticator) {
			// This happens when the protocol is rejected, but no alternative
			// protocol is supplied to us or the suggested protocol is not supported.
			// We can use this chance to increase fPeerIndex to the next authenticator.
			authenticator = NextAuthenticator(fPeerAuthenticator, PPP_PEER_SIDE);
		} else
			authenticator = fSuggestedPeerAuthenticator;
		
		fPeerAuthenticatorRejected = false;
	} else {
		if(!fPeerAuthenticator) {
			// there is no authenticator selected, so find one for us
			authenticator = NextAuthenticator(fPeerAuthenticator, PPP_PEER_SIDE);
		} else
			authenticator = fPeerAuthenticator;
	}
	
	// check if all authenticators were rejected or if no authentication needed
	if(!authenticator) {
		if(fPeerAuthenticator)
			return B_ERROR;
				// all authenticators were denied
		else
			return B_OK;
				// no peer authentication needed
	}
	
	if(!authenticator || !authenticator->OptionHandler())
		return B_ERROR;
	
	fPeerAuthenticator = authenticator;
		// this could omit some authenticators when we get a suggestion, but that is
		// no problem because the suggested authenticator will be accepted (hopefully)
	
#if DEBUG
	dprintf("PPPAuthHandler: AddToRequest(%X)\n", authenticator->ProtocolNumber());
#endif
	
	authenticator->SetEnabled(true);
	return authenticator->OptionHandler()->AddToRequest(request);
		// let protocol add its request
}


status_t
_PPPAuthenticationHandler::ParseNak(const PPPConfigurePacket& nak)
{
	// The authenticator's OptionHandler is not notified.
	
	authentication_item *item =
		(authentication_item*) nak.ItemWithType(AUTHENTICATION_TYPE);
	
	if(!item)
		return B_OK;
			// the request was not rejected
	if(item->length < 4)
		return B_ERROR;
	
	if(fSuggestedPeerAuthenticator) {
		fSuggestedPeerAuthenticator->SetEnabled(false);
		// if no alternative protocol is supplied we will choose a new one in
		// AddToRequest()
		if(ntohs(item->protocolNumber) ==
				fSuggestedPeerAuthenticator->ProtocolNumber()) {
			fSuggestedPeerAuthenticator = NULL;
			return B_OK;
		}
	}
	
	fPeerAuthenticatorRejected = true;
	PPPProtocol *authenticator = Interface().ProtocolFor(ntohs(item->protocolNumber));
	if(authenticator
			&& !strcasecmp(authenticator->Type(), AUTHENTICATOR_TYPE_STRING)
			&& authenticator->OptionHandler())
		fSuggestedPeerAuthenticator = authenticator;
	else
		fSuggestedPeerAuthenticator = NULL;
	
	return B_OK;
}


status_t
_PPPAuthenticationHandler::ParseReject(const PPPConfigurePacket& reject)
{
	// an authentication request must not be rejected!
	if(reject.ItemWithType(AUTHENTICATION_TYPE))
		return B_ERROR;
	
	return B_OK;
}


status_t
_PPPAuthenticationHandler::ParseAck(const PPPConfigurePacket& ack)
{
	authentication_item *item =
		(authentication_item*) ack.ItemWithType(AUTHENTICATION_TYPE);
	
	if(!item) {
		if(fPeerAuthenticator)
			return B_ERROR;
				// the ack does not contain our request
		else
			return B_OK;
				// no authentication needed
	} else if(!fPeerAuthenticator
			|| ntohs(item->protocolNumber) != fPeerAuthenticator->ProtocolNumber())
		return B_ERROR;
			// this item was never requested
	
	return fPeerAuthenticator->OptionHandler()->ParseAck(ack);
		// this should enable the authenticator
}


status_t
_PPPAuthenticationHandler::ParseRequest(const PPPConfigurePacket& request,
	int32 index, PPPConfigurePacket& nak, PPPConfigurePacket& reject)
{
	if(fLocalAuthenticator)
		fLocalAuthenticator->SetEnabled(false);
	
	authentication_item *item = (authentication_item*) request.ItemAt(index);
	if(!item)
		return B_OK;
			// no authentication requested by peer (index > request.CountItems())
	
#if DEBUG
	dprintf("PPPAuthHandler: ParseRequest(%X)\n", ntohs(item->protocolNumber));
#endif
	
	// try to find the requested protocol
	fLocalAuthenticator = Interface().ProtocolFor(ntohs(item->protocolNumber));
	if(fLocalAuthenticator &&
			!strcasecmp(fLocalAuthenticator->Type(), AUTHENTICATOR_TYPE_STRING)
			&& fLocalAuthenticator->OptionHandler())
		return fLocalAuthenticator->OptionHandler()->ParseRequest(request, index,
			nak, reject);
	
	// suggest another authentication protocol
	PPPProtocol *nextAuthenticator =
		NextAuthenticator(fSuggestedLocalAuthenticator, PPP_LOCAL_SIDE);
	
	if(!nextAuthenticator) {
		if(!fSuggestedLocalAuthenticator) {
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
	suggestion.type = AUTHENTICATION_TYPE;
	suggestion.length = 4;
	suggestion.protocolNumber = htons(nextAuthenticator->ProtocolNumber());
	return nak.AddItem((ppp_configure_item*) &suggestion) ? B_OK : B_ERROR;
}


status_t
_PPPAuthenticationHandler::SendingAck(const PPPConfigurePacket& ack)
{
	// do not insist on authenticating our side of the link ;)
	
	authentication_item *item =
		(authentication_item*) ack.ItemWithType(AUTHENTICATION_TYPE);
	
	if(!item)
		return B_OK;
			// no authentication needed
	
	fSuggestedLocalAuthenticator = NULL;
	
	if(!fLocalAuthenticator)
		return B_ERROR;
			// no authenticator selected (our suggestions must be requested, too)
	
	if(!fLocalAuthenticator)
		return B_ERROR;
	
	fLocalAuthenticator->SetEnabled(true);
	return fLocalAuthenticator->OptionHandler()->SendingAck(ack);
		// this should enable the authenticator
}


void
_PPPAuthenticationHandler::Reset()
{
	if(fLocalAuthenticator) {
		fLocalAuthenticator->SetEnabled(false);
		fLocalAuthenticator->OptionHandler()->Reset();
	}
	if(fPeerAuthenticator) {
		fPeerAuthenticator->SetEnabled(false);
		fPeerAuthenticator->OptionHandler()->Reset();
	}
	if(fSuggestedPeerAuthenticator) {
		fSuggestedPeerAuthenticator->SetEnabled(false);
		fSuggestedPeerAuthenticator->OptionHandler()->Reset();
	}
	
	fLocalAuthenticator = fPeerAuthenticator = fSuggestedLocalAuthenticator =
		fSuggestedPeerAuthenticator = NULL;
	fPeerAuthenticatorRejected = false;
}
