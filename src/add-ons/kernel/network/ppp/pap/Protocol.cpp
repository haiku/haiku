//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include "Protocol.h"
#include <KPPPConfigurePacket.h>
#include <KPPPInterface.h>

#include <LockerHelper.h>

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
		AF_INET, 0, interface, settings, PPP_ALWAYS_ALLOWED,
		kAuthenticatorTypeString, new PAPHandler(*this, interface)),
	fState(INITIAL),
	fID(system_time() & 0xFF),
	fMaxRequest(3),
	fRequestID(0),
	fNextTimeout(0)
{
	ProfileChanged();
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


void
PAP::ProfileChanged()
{
	ParseSettings(Interface().Profile().SettingsFor("authenticator", "pap"));
}


bool
PAP::Up()
{
#if DEBUG
	dprintf("PAP: Up() state=%d\n", State());
#endif
	
	LockerHelper locker(fLock);
	
	switch(State()) {
		case INITIAL:
			if(Side() == PPP_LOCAL_SIDE) {
				NewState(REQ_SENT);
				InitializeRestartCount();
				locker.UnlockNow();
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
#if DEBUG
	dprintf("PAP: Down() state=%d\n", State());
#endif
	
	LockerHelper locker(fLock);
	
	switch(Interface().Phase()) {
		case PPP_DOWN_PHASE:
			// interface finished terminating
		case PPP_ESTABLISHED_PHASE:
			// terminate this NCP individually (block until we finished terminating)
			NewState(INITIAL);
			locker.UnlockNow();
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
	LockerHelper locker(fLock);
	if(fNextTimeout == 0 || fNextTimeout > system_time())
		return;
	fNextTimeout = 0;
	locker.UnlockNow();
	
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


bool
PAP::ParseSettings(const driver_parameter *requests)
{
	memset(fUser, 0, sizeof(fUser));
	memset(fPassword, 0, sizeof(fPassword));
	
	if(!requests)
		return false;
	
	// The following values are allowed:
	//  "User"
	//  "Password"
	
	for(int32 index = 0; index < requests->parameter_count; index++) {
		if(requests->parameters[index].value_count == 0)
			continue;
		
		// ignore user and password if too long (255 chars at max)
		if(!strcasecmp(requests->parameters[index].name, "User")
				&& strlen(requests->parameters[index].values[0]) < sizeof(fUser))
			strcpy(fUser, requests->parameters[index].values[0]);
		else if(!strcasecmp(requests->parameters[index].name, "Password")
				&& strlen(requests->parameters[index].values[0]) < sizeof(fPassword))
			strcpy(fPassword, requests->parameters[index].values[0]);
	}
	
	return true;
}


uint8
PAP::NextID()
{
	return (uint8) atomic_add(&fID, 1);
}


void
PAP::NewState(pap_state next)
{
#if DEBUG
	dprintf("PAP: NewState(%d) state=%d\n", next, State());
#endif
	
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
#if DEBUG
	dprintf("PAP: TOGoodEvent() state=%d\n", State());
#endif
	
	LockerHelper locker(fLock);
	
	switch(State()) {
		case REQ_SENT:
			locker.UnlockNow();
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
#if DEBUG
	dprintf("PAP: TOBadEvent() state=%d\n", State());
#endif
	
	LockerHelper locker(fLock);
	
	switch(State()) {
		case REQ_SENT:
		case WAITING_FOR_REQ:
			NewState(INITIAL);
			locker.UnlockNow();
			if(State() == REQ_SENT)
				Interface().StateMachine().LocalAuthenticationDenied(fUser);
			else
				Interface().StateMachine().PeerAuthenticationDenied(fUser);
			
			UpFailedEvent();
		break;
		
		default:
			;
	}
}


void
PAP::RREvent(struct mbuf *packet)
{
#if DEBUG
	dprintf("PAP: RREvent() state=%d\n", State());
#endif
	
	LockerHelper locker(fLock);
	
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
	
	char *user = (char*) userLength + 1, *password = (char*) passwordLength + 1;
	
	if(*userLength == strlen(fUser) && *passwordLength == strlen(fPassword)
			&& !strncmp(user, fUser, *userLength)
			&& !strncmp(password, fPassword, *passwordLength)) {
		NewState(ACCEPTED);
		locker.UnlockNow();
		Interface().StateMachine().PeerAuthenticationAccepted(user);
		UpEvent();
		SendAck(packet);
	} else {
		NewState(INITIAL);
		locker.UnlockNow();
		Interface().StateMachine().PeerAuthenticationDenied(user);
		UpFailedEvent();
		SendNak(packet);
	}
}


void
PAP::RAEvent(struct mbuf *packet)
{
#if DEBUG
	dprintf("PAP: RAEvent() state=%d\n", State());
#endif
	
	LockerHelper locker(fLock);
	
	if(fRequestID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO: log this event
		m_freem(packet);
		return;
	}
	
	switch(State()) {
		case REQ_SENT:
			NewState(ACCEPTED);
			locker.UnlockNow();
			Interface().StateMachine().LocalAuthenticationAccepted(fUser);
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
#if DEBUG
	dprintf("PAP: RNEvent() state=%d\n", State());
#endif
	
	LockerHelper locker(fLock);
	
	if(fRequestID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO: log this event
		m_freem(packet);
		return;
	}
	
	switch(State()) {
		case REQ_SENT:
			NewState(INITIAL);
			locker.UnlockNow();
			Interface().StateMachine().LocalAuthenticationDenied(fUser);
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
#if DEBUG
	dprintf("PAP: SendRequest() state=%d\n", State());
#endif
	
	LockerHelper locker(fLock);
	--fRequestCounter;
	fNextTimeout = system_time() + kPAPTimeout;
	locker.UnlockNow();
	
	struct mbuf *packet = m_gethdr(MT_DATA);
	if(!packet)
		return false;
	
	packet->m_pkthdr.len = packet->m_len = 6 + strlen(fUser) + strlen(fPassword);
	
	// reserve some space for overhead (we are lazy and reserve too much)
	packet->m_data += Interface().PacketOverhead();
	
	ppp_lcp_packet *request = mtod(packet, ppp_lcp_packet*);
	request->code = PPP_CONFIGURE_REQUEST;
	request->id = fRequestID = NextID();
	request->length = htons(packet->m_len);
	uint8 *data = request->data;
	data[0] = strlen(fUser);
	memcpy(data + 1, fUser, strlen(fUser));
	data[1 + data[0]] = strlen(fPassword);
	memcpy(data + 2 + data[0], fPassword, strlen(fPassword));
	
	return Interface().Send(packet, PAP_PROTOCOL) == B_OK;
}


bool
PAP::SendAck(struct mbuf *packet)
{
#if DEBUG
	dprintf("PAP: SendAck() state=%d\n", State());
#endif
	
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
#if DEBUG
	dprintf("PAP: SendNak() state=%d\n", State());
#endif
	
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
