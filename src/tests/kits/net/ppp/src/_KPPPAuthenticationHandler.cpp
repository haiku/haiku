//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "_KPPPAuthenticationHandler.h"

#include <KPPPConfigurePacket.h>
#include <KPPPDevice.h>

#include <netinet/in.h>

#define AUTHENTICATION_TYPE				0x3
#define AUTHENTICATOR_TYPE_STRING		"Authenticator"

typedef struct authentication_item {
	uint8 type;
	uint8 length;
	uint16 protocol;
} authentication_item;


_PPPAuthenticationHandler::_PPPAuthenticationHandler(PPPInterface& interface)
	: PPPOptionHandler("Authentication Handler", AUTHENTICATION_TYPE, interface, NULL),
	fLocalIndex(-1),
	fPeerIndex(-1),
	fSuggestedLocalIndex(-1),
	fSuggestedPeerIndex(-1),
	fPeerAuthenticatorRejected(false)
{
}


int32
_PPPAuthenticationHandler::NextAuthenticator(int32 start, ppp_side side)
{
	// find the next authenticator for side, beginning at start
	
	PPPProtocol *protocol;
	for(int32 index = start; index < Interface().CountProtocols(); index++) {
		protocol = Interface().ProtocolAt(index);
		if(protocol && !strcasecmp(protocol->Type(), AUTHENTICATOR_TYPE_STRING)
				&& protocol->OptionHandler()
				&& protocol->Side() == side)
			return index;
	}
	
	return -1;
}


status_t
_PPPAuthenticationHandler::AddToRequest(PPPConfigurePacket& request)
{
	// AddToRequest(): Check if peer must authenticate itself and
	// add an authentication request if needed. This request is added
	// by the authenticator's OptionHandler.
	
	int32 index;
	PPPProtocol *protocol;
	if(fPeerIndex != -1 && (protocol = Interface().ProtocolAt(fPeerIndex)))
		protocol->SetEnabled(false);
	if(fSuggestedPeerIndex != -1
			&& (protocol = Interface().ProtocolAt(fSuggestedPeerIndex)))
		protocol->SetEnabled(false);
	
	if(fPeerAuthenticatorRejected) {
		if(fSuggestedPeerIndex == -1) {
			// This happens when the protocol is rejected, but no alternative
			// protocol is supplied to us or the suggested protocol is not supported.
			// We can use this chance to increase fPeerIndex to the next authenticator.
			index = NextAuthenticator(fPeerIndex + 1, PPP_PEER_SIDE);
		} else
			index = fSuggestedPeerIndex;
		
		fPeerAuthenticatorRejected = false;
	} else {
		if(fPeerIndex == -1) {
			// there is no authenticator selected, so find one for us
			index = NextAuthenticator(fPeerIndex + 1, PPP_PEER_SIDE);
		} else
			index = fPeerIndex;
	}
	
	// check if all authenticators were rejected or if no authentication needed
	if(index == -1) {
		if(fPeerIndex == -1)
			return B_OK;
				// no peer authentication needed
		else
			return B_ERROR;
				// all authenticators were denied
	}
	
	protocol = Interface().ProtocolAt(index);
	if(!protocol || !protocol->OptionHandler())
		return B_ERROR;
	
	fPeerIndex = index;
		// this could omit some authenticators when we get a suggestion, but that is
		// no problem because the suggested authenticator will be accepted (hopefully)
	
	protocol->SetEnabled(true);
	return protocol->OptionHandler()->AddToRequest(request);
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
	
	PPPProtocol *protocol;
	if(fSuggestedPeerIndex != -1
			&& (protocol = Interface().ProtocolAt(fSuggestedPeerIndex))) {
		protocol->SetEnabled(false);
		// if no alternative protocol is supplied we will choose a new one in
		// AddToRequest()
		if(ntohs(item->protocol) == protocol->Protocol()) {
			fSuggestedPeerIndex = -1;
			return B_OK;
		}
	}
	
	fPeerAuthenticatorRejected = true;
	int32 index = 0;
	if((protocol = Interface().ProtocolFor(ntohs(item->protocol), &index))
			&& !strcasecmp(protocol->Type(), AUTHENTICATOR_TYPE_STRING)
			&& protocol->OptionHandler())
		fSuggestedPeerIndex = index;
	else
		fSuggestedPeerIndex = -1;
	
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
	
	PPPProtocol *protocol = Interface().ProtocolAt(fPeerIndex);
	if(fPeerIndex != -1 && !protocol)
		return B_ERROR;
			// could not find the protocol
	
	if(!item) {
		if(fPeerIndex != -1)
			return B_ERROR;
				// the ack does not contain our request
	} else if(fPeerIndex == -1 || ntohs(item->protocol) != protocol->Protocol())
		return B_ERROR;
			// this item was never requested
	
	return protocol->OptionHandler()->ParseAck(ack);
		// this should enable the authenticator
}


status_t
_PPPAuthenticationHandler::ParseRequest(const PPPConfigurePacket& request,
	int32 index, PPPConfigurePacket& nak, PPPConfigurePacket& reject)
{
	PPPProtocol *protocol;
	if(fLocalIndex != -1 && (protocol = Interface().ProtocolAt(fLocalIndex)))
		protocol->SetEnabled(false);
	
	authentication_item *item = (authentication_item*) request.ItemAt(index);
	if(!item)
		return B_OK;
			// no authentication requested by peer
	
	// try to find the requested protocol and select it by saving it in fLocalIndex
	fLocalIndex = 0;
	protocol = Interface().ProtocolFor(ntohs(item->protocol), &fLocalIndex);
	if(protocol && !strcasecmp(protocol->Type(), AUTHENTICATOR_TYPE_STRING)
			&& protocol->OptionHandler())
		return protocol->OptionHandler()->ParseRequest(request, index, nak, reject);
	
	// suggest another authentication protocol
	int32 nextIndex = NextAuthenticator(fSuggestedLocalIndex + 1, PPP_LOCAL_SIDE);
	
	if(nextIndex == -1) {
		if(fSuggestedLocalIndex == -1) {
			// reject the complete authentication option
			reject.AddItem((ppp_configure_item*) item);
			return B_OK;
		} else
			nextIndex = fSuggestedLocalIndex;
				// try the old one again as it was not rejected until now
	}
	
	protocol = Interface().ProtocolAt(nextIndex);
	if(!protocol)
		return B_ERROR;
	
	fSuggestedLocalIndex = nextIndex;
	fLocalIndex = -1;
		// no authenticator selected
	
	// nak this authenticator and suggest an alternative
	authentication_item suggestion;
	suggestion.type = AUTHENTICATION_TYPE;
	suggestion.length = 4;
	suggestion.protocol = htons(protocol->Protocol());
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
	
	fSuggestedLocalIndex = -1;
	
	if(fLocalIndex == -1)
		return B_ERROR;
			// no authenticator selected (our suggestions must be requested, too)
	
	PPPProtocol *protocol = Interface().ProtocolAt(fLocalIndex);
	if(!protocol)
		return B_ERROR;
	
	protocol->SetEnabled(true);
	return protocol->OptionHandler()->SendingAck(ack);
		// this should enable the authenticator
}


void
_PPPAuthenticationHandler::Reset()
{
	PPPProtocol *protocol = NULL;
	if(fLocalIndex != -1 && (protocol = Interface().ProtocolAt(fLocalIndex))) {
		protocol->SetEnabled(false);
		protocol->OptionHandler()->Reset();
	}
	if(fPeerIndex != -1 && (protocol = Interface().ProtocolAt(fPeerIndex))) {
		protocol->SetEnabled(false);
		protocol->OptionHandler()->Reset();
	}
	if(fSuggestedPeerIndex != -1
			&& (protocol = Interface().ProtocolAt(fSuggestedPeerIndex))) {
		protocol->SetEnabled(false);
		protocol->OptionHandler()->Reset();
	}
	
	fLocalIndex = fPeerIndex = fSuggestedLocalIndex = fSuggestedPeerIndex = -1;
	fPeerAuthenticatorRejected = false;
}
