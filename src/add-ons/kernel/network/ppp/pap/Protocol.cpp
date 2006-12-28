/*
 * Copyright 2003-2006, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "Protocol.h"
#include <KPPPConfigurePacket.h>
#include <KPPPInterface.h>

#include <cstring>
#include <netinet/in.h>
#include <core_funcs.h>
#include <sys/sockio.h>


static const bigtime_t kPAPTimeout = 3000000;
	// 3 seconds

// PAPHandler
static const uint8 kAuthenticationType = 0x3;
static const char *kAuthenticatorTypeString = "Authenticator";

typedef struct authentication_item {
	uint8 type;
	uint8 length;
	uint16 protocolNumber;
} _PACKED authentication_item;


PAPHandler::PAPHandler(PAP& owner, KPPPInterface& interface)
	: KPPPOptionHandler("PAP", kAuthenticationType, interface, NULL),
	fOwner(owner)
{
}


status_t
PAPHandler::AddToRequest(KPPPConfigurePacket& request)
{
	// only local authenticators send requests to peer
	if(Owner().Side() != PPP_PEER_SIDE)
		return B_OK;
	
	authentication_item item;
	item.type = kAuthenticationType;
	item.length = sizeof(item);
	item.protocolNumber = htons(PAP_PROTOCOL);
	
	request.AddItem((ppp_configure_item*) &item);
	
	return B_OK;
}


status_t
PAPHandler::ParseRequest(const KPPPConfigurePacket& request,
	int32 index, KPPPConfigurePacket& nak, KPPPConfigurePacket& reject)
{
	// only local authenticators handle requests from peer
	if(Owner().Side() != PPP_LOCAL_SIDE)
		return B_OK;
	
	// we merely check if the values are correct
	authentication_item *item = (authentication_item*) request.ItemAt(index);
	if(item->type != kAuthenticationType
			|| item->length != 4 || ntohs(item->protocolNumber) != PAP_PROTOCOL)
		return B_ERROR;
	
	return B_OK;
}


// PAP
PAP::PAP(KPPPInterface& interface, driver_parameter *settings)
	: KPPPProtocol("PAP", PPP_AUTHENTICATION_PHASE, PAP_PROTOCOL, PPP_PROTOCOL_LEVEL,
		AF_UNSPEC, 0, interface, settings, PPP_ALWAYS_ALLOWED,
		kAuthenticatorTypeString, new PAPHandler(*this, interface)),
	fState(INITIAL),
	fID(system_time() & 0xFF),
	fMaxRequest(3),
	fRequestID(0),
	fNextTimeout(0)
{
}


PAP::~PAP()
{
}


status_t
PAP::InitCheck() const
{
	if(Side() != PPP_LOCAL_SIDE && Side() != PPP_PEER_SIDE)
		return B_ERROR;
	
	return KPPPProtocol::InitCheck();
}


bool
PAP::Up()
{
	TRACE("PAP: Up() state=%d\n", State());
	
	switch(State()) {
		case INITIAL:
			if(Side() == PPP_LOCAL_SIDE) {
				NewState(REQ_SENT);
				InitializeRestartCount();
				SendRequest();
			} else if(Side() == PPP_PEER_SIDE) {
				NewState(WAITING_FOR_REQ);
				InitializeRestartCount();
				fNextTimeout = system_time() + kPAPTimeout;
			} else {
				UpFailedEvent();
				return false;
			}
		break;
		
		default:
			;
	}
	
	return true;
}


bool
PAP::Down()
{
	TRACE("PAP: Down() state=%d\n", State());
	
	switch(Interface().Phase()) {
		case PPP_DOWN_PHASE:
			// interface finished terminating
		case PPP_ESTABLISHED_PHASE:
			// terminate this NCP individually (block until we finished terminating)
			NewState(INITIAL);
			DownEvent();
		break;
		
/*		case PPP_TERMINATION_PHASE:
			// interface is terminating
		break;
		
		case PPP_ESTABLISHMENT_PHASE:
			// interface is reconfiguring
		break;
*/		
		default:
			;
	}
	
	return true;
}


status_t
PAP::Send(struct mbuf *packet, uint16 protocolNumber)
{
	// we do not encapsulate PAP packets
	m_freem(packet);
	return B_ERROR;
}


status_t
PAP::Receive(struct mbuf *packet, uint16 protocolNumber)
{
	if(!packet)
		return B_ERROR;
	
	if(protocolNumber != PAP_PROTOCOL)
		return PPP_UNHANDLED;
	
	ppp_lcp_packet *data = mtod(packet, ppp_lcp_packet*);
	
	// check if the packet is meant for us:
	// only peer authenticators handle requests
	if(data->code == PPP_CONFIGURE_REQUEST && Side() != PPP_PEER_SIDE)
		return PPP_UNHANDLED;
	// only local authenticators handle acks and naks
	if((data->code == PPP_CONFIGURE_ACK || data->code == PPP_CONFIGURE_NAK)
			&& Side() != PPP_LOCAL_SIDE)
		return PPP_UNHANDLED;
	
	// remove padding
	int32 length = packet->m_len;
	if(packet->m_flags & M_PKTHDR)
		length = packet->m_pkthdr.len;
	length -= ntohs(data->length);
	if(length)
		m_adj(packet, -length);
	
	if(ntohs(data->length) < 4)
		return B_ERROR;
	
	// packet is freed by event methods
	// code values are the same as for LCP (but very reduced)
	switch(data->code) {
		case PPP_CONFIGURE_REQUEST:
			RREvent(packet);
		break;
		
		case PPP_CONFIGURE_ACK:
			RAEvent(packet);
		break;
		
		case PPP_CONFIGURE_NAK:
			RNEvent(packet);
		break;
		
		default:
			return PPP_UNHANDLED;
	}
	
	return B_OK;
}


void
PAP::Pulse()
{
	if(fNextTimeout == 0 || fNextTimeout > system_time())
		return;
	fNextTimeout = 0;
	
	switch(State()) {
		case REQ_SENT:
		case WAITING_FOR_REQ:
			if(fRequestCounter <= 0)
				TOBadEvent();
			else
				TOGoodEvent();
		break;
		
		default:
			;
	}
}


uint8
PAP::NextID()
{
	return (uint8) atomic_add(&fID, 1);
}


void
PAP::NewState(pap_state next)
{
	TRACE("PAP: NewState(%d) state=%d\n", next, State());
	
	//  state changes
	if(State() == INITIAL && next != State()) {
		if(Side() == PPP_LOCAL_SIDE)
			Interface().StateMachine().LocalAuthenticationRequested();
		else if(Side() == PPP_PEER_SIDE)
			Interface().StateMachine().PeerAuthenticationRequested();
		
		UpStarted();
	} else if(State() == ACCEPTED && next != State())
		DownStarted();
	
	// maybe we do not need the timer anymore
	if(next == INITIAL || next == ACCEPTED)
		fNextTimeout = 0;
	
	fState = next;
}


void
PAP::TOGoodEvent()
{
	TRACE("PAP: TOGoodEvent() state=%d\n", State());
	
	switch(State()) {
		case REQ_SENT:
			SendRequest();
		break;
		
		case WAITING_FOR_REQ:
			fNextTimeout = system_time() + kPAPTimeout;
		break;
		
		default:
			;
	}
}


void
PAP::TOBadEvent()
{
	TRACE("PAP: TOBadEvent() state=%d\n", State());
	
	switch(State()) {
		case REQ_SENT:
		case WAITING_FOR_REQ:
			NewState(INITIAL);
			if(State() == REQ_SENT)
				Interface().StateMachine().LocalAuthenticationDenied(
					Interface().Username());
			else
				Interface().StateMachine().PeerAuthenticationDenied(
					Interface().Username());
			
			UpFailedEvent();
		break;
		
		default:
			;
	}
}


void
PAP::RREvent(struct mbuf *packet)
{
	TRACE("PAP: RREvent() state=%d\n", State());
	
	ppp_lcp_packet *request = mtod(packet, ppp_lcp_packet*);
	int32 length = ntohs(request->length);
	uint8 *data = request->data;
	uint8 *userLength = data;
	uint8 *passwordLength = data + 1 + data[0];
	
	// make sure the length values are all okay
	if(6 + *userLength + *passwordLength > length) {
		m_freem(packet);
		return;
	}
	
	char *peerUsername = (char*) userLength + 1,
		*peerPassword = (char*) passwordLength + 1;
	const char *username = Interface().Username(), *password = Interface().Password();
	
	if(*userLength == strlen(username) && *passwordLength == strlen(password)
			&& !strncmp(peerUsername, username, *userLength)
			&& !strncmp(peerPassword, password, *passwordLength)) {
		NewState(ACCEPTED);
		Interface().StateMachine().PeerAuthenticationAccepted(username);
		UpEvent();
		SendAck(packet);
	} else {
		NewState(INITIAL);
		Interface().StateMachine().PeerAuthenticationDenied(username);
		UpFailedEvent();
		SendNak(packet);
	}
}


void
PAP::RAEvent(struct mbuf *packet)
{
	TRACE("PAP: RAEvent() state=%d\n", State());
	
	if(fRequestID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO: log this event
		m_freem(packet);
		return;
	}
	
	switch(State()) {
		case REQ_SENT:
			NewState(ACCEPTED);
			Interface().StateMachine().LocalAuthenticationAccepted(
				Interface().Username());
			UpEvent();
		break;
		
		default:
			;
	}
	
	m_freem(packet);
}


void
PAP::RNEvent(struct mbuf *packet)
{
	TRACE("PAP: RNEvent() state=%d\n", State());
	
	if(fRequestID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO: log this event
		m_freem(packet);
		return;
	}
	
	switch(State()) {
		case REQ_SENT:
			NewState(INITIAL);
			Interface().StateMachine().LocalAuthenticationDenied(
				Interface().Username());
			UpFailedEvent();
		break;
		
		default:
			;
	}
	
	m_freem(packet);
}


void
PAP::InitializeRestartCount()
{
	fRequestCounter = fMaxRequest;
}


bool
PAP::SendRequest()
{
	TRACE("PAP: SendRequest() state=%d\n", State());
	
	--fRequestCounter;
	fNextTimeout = system_time() + kPAPTimeout;
	
	struct mbuf *packet = m_gethdr(MT_DATA);
	if(!packet)
		return false;
	
	const char *username = Interface().Username(), *password = Interface().Password();
	
	packet->m_pkthdr.len = packet->m_len = 6 + strlen(username) + strlen(password);
	
	// reserve some space for overhead (we are lazy and reserve too much)
	packet->m_data += Interface().PacketOverhead();
	
	ppp_lcp_packet *request = mtod(packet, ppp_lcp_packet*);
	request->code = PPP_CONFIGURE_REQUEST;
	request->id = fRequestID = NextID();
	request->length = htons(packet->m_len);
	uint8 *data = request->data;
	data[0] = strlen(username);
	memcpy(data + 1, username, strlen(username));
	data[1 + data[0]] = strlen(password);
	memcpy(data + 2 + data[0], password, strlen(password));
	
	return Interface().Send(packet, PAP_PROTOCOL) == B_OK;
}


bool
PAP::SendAck(struct mbuf *packet)
{
	TRACE("PAP: SendAck() state=%d\n", State());
	
	if(!packet)
		return false;
	
	ppp_lcp_packet *ack = mtod(packet, ppp_lcp_packet*);
	ack->code = PPP_CONFIGURE_ACK;
	packet->m_len = 5;
	if(packet->m_flags & M_PKTHDR)
		packet->m_pkthdr.len = 5;
	ack->length = htons(packet->m_len);
	
	ack->data[0] = 0;
	
	return Interface().Send(packet, PAP_PROTOCOL) == B_OK;
}


bool
PAP::SendNak(struct mbuf *packet)
{
	TRACE("PAP: SendNak() state=%d\n", State());
	
	if(!packet)
		return false;
	
	ppp_lcp_packet *nak = mtod(packet, ppp_lcp_packet*);
	nak->code = PPP_CONFIGURE_NAK;
	packet->m_len = 5;
	if(packet->m_flags & M_PKTHDR)
		packet->m_pkthdr.len = 5;
	nak->length = htons(packet->m_len);
	
	nak->data[0] = 0;
	
	return Interface().Send(packet, PAP_PROTOCOL) == B_OK;
}
