/*
 * Copyright 2003-2006, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "PPPoEDevice.h"
#include "DiscoveryPacket.h"

#include <core_funcs.h>
#include <cstdlib>

// from libkernelppp
#include <settings_tools.h>


#if DEBUG
static char sDigits[] = "0123456789ABCDEF";
void
dump_packet(struct mbuf *packet)
{
	if(!packet)
		return;
	
	uint8 *data = mtod(packet, uint8*);
	uint8 buffer[33];
	uint8 bufferIndex = 0;
	
	TRACE("Dumping packet;len=%ld;pkthdr.len=%d\n", packet->m_len,
		packet->m_flags & M_PKTHDR ? packet->m_pkthdr.len : -1);
	
	for(uint32 index = 0; index < packet->m_len; index++) {
		buffer[bufferIndex++] = sDigits[data[index] >> 4];
		buffer[bufferIndex++] = sDigits[data[index] & 0x0F];
		if(bufferIndex == 32 || index == packet->m_len - 1) {
			buffer[bufferIndex] = 0;
			TRACE("%s\n", buffer);
			bufferIndex = 0;
		}
	}
}
#endif


PPPoEDevice::PPPoEDevice(KPPPInterface& interface, driver_parameter *settings)
	: KPPPDevice("PPPoE", PPPoE_HEADER_SIZE + ETHER_HDR_LEN, interface, settings),
	fEthernetIfnet(NULL),
	fSessionID(0),
	fHostUniq(NewHostUniq()),
	fACName(NULL),
	fServiceName(NULL),
	fAttempts(0),
	fNextTimeout(0),
	fState(INITIAL)
{
#if DEBUG
	TRACE("PPPoEDevice: Constructor\n");
	if(!settings || !settings->parameters)
		TRACE("PPPoEDevice::ctor: No settings!\n");
#endif
	
	interface.SetPFCOptions(PPP_ALLOW_PFC);
		// we do not want to fail just because the other side requests PFC
	
	memset(fPeer, 0xFF, sizeof(fPeer));
	SetMTU(1494);
		// MTU size does not contain PPP header
	
	// find ethernet device
	const char *interfaceName = get_parameter_value(PPPoE_INTERFACE_KEY, settings);
	if(!interfaceName)
		return;
	
	fACName = get_parameter_value(PPPoE_AC_NAME_KEY, settings);
	fServiceName = get_parameter_value(PPPoE_SERVICE_NAME_KEY, settings);
	
	fEthernetIfnet = FindPPPoEInterface(interfaceName);
	
#if DEBUG
	if(!fEthernetIfnet)
		TRACE("PPPoEDevice::ctor: could not find ethernet interface\n");
#endif
}


status_t
PPPoEDevice::InitCheck() const
{
	return EthernetIfnet() && EthernetIfnet()->output
		&& KPPPDevice::InitCheck() == B_OK ? B_OK : B_ERROR;
}


bool
PPPoEDevice::Up()
{
	TRACE("PPPoEDevice: Up()\n");
	
	if(InitCheck() != B_OK)
		return false;
	
	if(IsUp())
		return true;
	
	add_device(this);
	
	fState = INITIAL;
		// reset state
	
	if(fAttempts > PPPoE_MAX_ATTEMPTS) {
		fAttempts = 0;
		return false;
	}
	
	++fAttempts;
	// reset connection settings
	memset(fPeer, 0xFF, sizeof(fPeer));
	
	// create PADI
	DiscoveryPacket discovery(PADI);
	if(ServiceName())
		discovery.AddTag(SERVICE_NAME, ServiceName(), strlen(ServiceName()));
	else
		discovery.AddTag(SERVICE_NAME, NULL, 0);
	discovery.AddTag(HOST_UNIQ, &fHostUniq, sizeof(fHostUniq));
	discovery.AddTag(END_OF_LIST, NULL, 0);
	
	// set up PPP header
	struct mbuf *packet = discovery.ToMbuf(MTU());
	if(!packet)
		return false;
	
	// create destination
	struct ether_header *ethernetHeader;
	struct sockaddr destination;
	memset(&destination, 0, sizeof(destination));
	destination.sa_family = AF_UNSPEC;
		// raw packet with ethernet header
	ethernetHeader = (struct ether_header*) destination.sa_data;
	ethernetHeader->ether_type = ETHERTYPE_PPPOEDISC;
	memcpy(ethernetHeader->ether_dhost, fPeer, sizeof(fPeer));
	
	// check if we are allowed to go up now (user intervention might disallow that)
	if(fAttempts > 0 && !UpStarted()) {
		fAttempts = 0;
		remove_device(this);
		DownEvent();
		return true;
			// there was no error
	}
	
	fState = PADI_SENT;
		// needed before sending, otherwise we might not get all packets
	
	if(EthernetIfnet()->output(EthernetIfnet(), packet, &destination, NULL) != B_OK) {
		fState = INITIAL;
		fAttempts = 0;
		ERROR("PPPoEDevice::Up(): EthernetIfnet()->output() failed!\n");
		return false;
	}
	
	fNextTimeout = system_time() + PPPoE_TIMEOUT;
	
	return true;
}


bool
PPPoEDevice::Down()
{
	TRACE("PPPoEDevice: Down()\n");
	
	remove_device(this);
	
	if(InitCheck() != B_OK)
		return false;
	
	fState = INITIAL;
	fAttempts = 0;
	fNextTimeout = 0;
		// disable timeouts
	
	if(!IsUp()) {
		DownEvent();
		return true;
	}
	
	DownStarted();
		// this tells StateMachine that DownEvent() does not mean we lost connection
	
	// create PADT
	DiscoveryPacket discovery(PADT, SessionID());
	discovery.AddTag(END_OF_LIST, NULL, 0);
	
	struct mbuf *packet = discovery.ToMbuf(MTU());
	if(!packet) {
		ERROR("PPPoEDevice::Down(): ToMbuf() failed; MTU=%ld\n", MTU());
		DownEvent();
		return false;
	}
	
	// create destination
	struct ether_header *ethernetHeader;
	struct sockaddr destination;
	memset(&destination, 0, sizeof(destination));
	destination.sa_family = AF_UNSPEC;
		// raw packet with ethernet header
	ethernetHeader = (struct ether_header*) destination.sa_data;
	ethernetHeader->ether_type = ETHERTYPE_PPPOEDISC;
	memcpy(ethernetHeader->ether_dhost, fPeer, sizeof(fPeer));
	
	// reset connection settings
	memset(fPeer, 0xFF, sizeof(fPeer));
	
	EthernetIfnet()->output(EthernetIfnet(), packet, &destination, NULL);
	DownEvent();
	
	return true;
}


uint32
PPPoEDevice::InputTransferRate() const
{
	return 10000000;
}


uint32
PPPoEDevice::OutputTransferRate() const
{
	return 10000000;
}


uint32
PPPoEDevice::CountOutputBytes() const
{
	// TODO:
	// ?look through ethernet queue for outgoing pppoe packets coming from our device?
	return 0;
}


status_t
PPPoEDevice::Send(struct mbuf *packet, uint16 protocolNumber)
{
	// Send() is only for PPP packets. PPPoE packets are sent directly to ethernet.
	
	TRACE("PPPoEDevice: Send()\n");
	
	if(!packet)
		return B_ERROR;
	else if(InitCheck() != B_OK || protocolNumber != 0) {
		ERROR("PPPoEDevice::Send(): InitCheck() != B_OK!\n");
		m_freem(packet);
		return B_ERROR;
	}
	
	if(!IsUp()) {
		ERROR("PPPoEDevice::Send(): no connection!\n");
		m_freem(packet);
		return PPP_NO_CONNECTION;
	}
	
	uint16 length = packet->m_flags & M_PKTHDR ? packet->m_pkthdr.len : packet->m_len;
	
	// encapsulate packet into pppoe header
	M_PREPEND(packet, PPPoE_HEADER_SIZE);
	pppoe_header *header = mtod(packet, pppoe_header*);
	header->version = PPPoE_VERSION;
	header->type = PPPoE_TYPE;
	header->code = 0x00;
	header->sessionID = SessionID();
	header->length = htons(length);
	
	// create destination
	struct ether_header *ethernetHeader;
	struct sockaddr destination;
	memset(&destination, 0, sizeof(destination));
	destination.sa_family = AF_UNSPEC;
		// raw packet with ethernet header
	ethernetHeader = (struct ether_header*) destination.sa_data;
	ethernetHeader->ether_type = ETHERTYPE_PPPOE;
	memcpy(ethernetHeader->ether_dhost, fPeer, sizeof(fPeer));
	
	if(!packet || !mtod(packet, pppoe_header*))
		ERROR("PPPoEDevice::Send(): packet is NULL!\n");
	
	if(EthernetIfnet()->output(EthernetIfnet(), packet, &destination, NULL) != B_OK) {
		ERROR("PPPoEDevice::Send(): EthernetIfnet()->output() failed!\n");
		remove_device(this);
		DownEvent();
			// DownEvent() without DownStarted() indicates connection lost
		return PPP_NO_CONNECTION;
	}
	
	return B_OK;
}


status_t
PPPoEDevice::Receive(struct mbuf *packet, uint16 protocolNumber)
{
	if(!packet)
		return B_ERROR;
	else if(InitCheck() != B_OK || IsDown()) {
		m_freem(packet);
		return B_ERROR;
	}
	
	complete_pppoe_header *completeHeader = mtod(packet, complete_pppoe_header*);
	if(!completeHeader) {
		m_freem(packet);
		return B_ERROR;
	}
	
	uint8 ethernetSource[6];
	memcpy(ethernetSource, completeHeader->ethernetHeader.ether_shost, sizeof(fPeer));
	status_t result = B_OK;
	
	if(completeHeader->ethernetHeader.ether_type == ETHERTYPE_PPPOE) {
		m_adj(packet, ETHER_HDR_LEN);
		pppoe_header *header = mtod(packet, pppoe_header*);
		
		if(!IsUp() || header->version != PPPoE_VERSION || header->type != PPPoE_TYPE
				|| header->code != 0x0 || header->sessionID != SessionID()) {
			m_freem(packet);
			return B_ERROR;
		}
		
		m_adj(packet, PPPoE_HEADER_SIZE);
		return Interface().ReceiveFromDevice(packet);
	} else if(completeHeader->ethernetHeader.ether_type == ETHERTYPE_PPPOEDISC) {
		m_adj(packet, ETHER_HDR_LEN);
		pppoe_header *header = mtod(packet, pppoe_header*);
		
		// we do not need to check HOST_UNIQ tag as this is done in pppoe.cpp
		if(header->version != PPPoE_VERSION || header->type != PPPoE_TYPE) {
			m_freem(packet);
			return B_ERROR;
		}
		
		if(IsDown()) {
			m_freem(packet);
			return B_ERROR;
		}
		
		DiscoveryPacket discovery(packet);
		switch(discovery.Code()) {
			case PADO: {
				if(fState != PADI_SENT) {
					m_freem(packet);
					return B_OK;
				}
				
				bool hasServiceName = false, hasACName = false;
				pppoe_tag *tag;
				DiscoveryPacket reply(PADR);
				for(int32 index = 0; index < discovery.CountTags(); index++) {
					tag = discovery.TagAt(index);
					if(!tag)
						continue;
					
					switch(tag->type) {
						case SERVICE_NAME:
							if(!hasServiceName && (!ServiceName()
									|| (strlen(ServiceName()) == tag->length)
										&& !memcmp(tag->data, ServiceName(),
											tag->length))) {
								hasServiceName = true;
								reply.AddTag(tag->type, tag->data, tag->length);
							}
						break;
						
						case AC_NAME:
							if(!hasACName && (!ACName()
									|| (strlen(ACName()) == tag->length)
										&& !memcmp(tag->data, ACName(),
											tag->length))) {
								hasACName = true;
								reply.AddTag(tag->type, tag->data, tag->length);
							}
						break;
						
						case AC_COOKIE:
						case RELAY_SESSION_ID:
							reply.AddTag(tag->type, tag->data, tag->length);
						break;
						
						case SERVICE_NAME_ERROR:
						case AC_SYSTEM_ERROR:
						case GENERIC_ERROR:
							m_freem(packet);
							return B_ERROR;
						break;
						
						default:
							;
					}
				}
				
				if(!hasServiceName) {
					m_freem(packet);
					return B_ERROR;
				}
				
				reply.AddTag(HOST_UNIQ, &fHostUniq, sizeof(fHostUniq));
				reply.AddTag(END_OF_LIST, NULL, 0);
				struct mbuf *replyPacket = reply.ToMbuf(MTU());
				if(!replyPacket) {
					m_freem(packet);
					return B_ERROR;
				}
				
				memcpy(fPeer, ethernetSource, sizeof(fPeer));
				
				// create destination
				struct ether_header *ethernetHeader;
				struct sockaddr destination;
				memset(&destination, 0, sizeof(destination));
				destination.sa_family = AF_UNSPEC;
					// raw packet with ethernet header
				ethernetHeader = (struct ether_header*) destination.sa_data;
				ethernetHeader->ether_type = ETHERTYPE_PPPOEDISC;
				memcpy(ethernetHeader->ether_dhost, fPeer, sizeof(fPeer));
				
				fState = PADR_SENT;
				
				if(EthernetIfnet()->output(EthernetIfnet(), replyPacket, &destination,
						NULL) != B_OK) {
					m_freem(packet);
					return B_ERROR;
				}
				
				fNextTimeout = system_time() + PPPoE_TIMEOUT;
			} break;
			
			case PADS:
				if(fState != PADR_SENT
						|| memcmp(ethernetSource, fPeer, sizeof(fPeer))) {
					m_freem(packet);
					return B_ERROR;
				}
				
				fSessionID = header->sessionID;
				fState = OPENED;
				fNextTimeout = 0;
				UpEvent();
			break;
			
			case PADT:
				if(!IsUp()
						|| memcmp(ethernetSource, fPeer, sizeof(fPeer))
						|| header->sessionID != SessionID()) {
					m_freem(packet);
					return B_ERROR;
				}
				
				fState = INITIAL;
				fAttempts = 0;
				fSessionID = 0;
				fNextTimeout = 0;
				remove_device(this);
				DownEvent();
			break;
			
			default:
				m_freem(packet);
				return B_ERROR;
		}
	} else
		result = B_ERROR;
	
	m_freem(packet);
	return result;
}


void
PPPoEDevice::Pulse()
{
	// We use Pulse() for timeout of connection establishment.
	
	if(fNextTimeout == 0 || IsUp() || IsDown())
		return;
	
	// check if timed out
	if(system_time() >= fNextTimeout) {
		if(!Up())
			UpFailedEvent();
	}
}
