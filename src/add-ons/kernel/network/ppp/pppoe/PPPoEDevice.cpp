//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "PPPoEDevice.h"
#include "DiscoveryPacket.h"

#include <core_funcs.h>
#include <cstdlib>
#include <kernel_cpp.h>

// from libkernelppp
#include <settings_tools.h>
#include <LockerHelper.h>

#ifdef _KERNEL_MODE
	#define spawn_thread spawn_kernel_thread
	#define printf dprintf
#endif


PPPoEDevice::PPPoEDevice(PPPInterface& interface, driver_parameter *settings)
	: PPPDevice("PPPoE", interface, settings),
	fEthernetIfnet(NULL),
	fSessionID(0),
	fHostUniq(NewHostUniq()),
	fACName(NULL),
	fServiceName(NULL),
	fNextTimeout(0),
	fState(INITIAL)
{
#if DEBUG
	printf("PPPoEDevice: Constructor\n");
	if(!settings || !settings->parameters)
		printf("PPPoEDevice::ctor: No settings!\n");
	else if(settings->parameter_count > 0 && settings->parameters[0].value_count > 0)
		printf("PPPoEDevice::ctor: Value0: %s\n", settings->parameters[0].values[0]);
	else
		printf("PPPoEDevice::ctor: No values!\n");
#endif
	
	memset(fPeer, 0xFF, sizeof(fPeer));
	SetMTU(1494);
		// MTU size does not contain PPP header
	
	// find ethernet device
	const char *interfaceName = get_parameter_value(PPPoE_INTERFACE_KEY, settings);
	if(!interfaceName)
		return;
#if DEBUG
	printf("PPPoEDevice::ctor: interfaceName: %s\n", interfaceName);
#endif
	
	ifnet *current = get_interfaces();
	for(; current; current = current->if_next) {
		if(current->if_name && !strcmp(current->if_name, interfaceName)) {
#if DEBUG
			printf("PPPoEDevice::ctor: found ethernet interface\n");
#endif
			fEthernetIfnet = current;
			break;
		}
	}
	
#if DEBUG
	if(!fEthernetIfnet)
		printf("PPPoEDevice::ctor: could not find ethernet interface\n");
#endif
}


PPPoEDevice::~PPPoEDevice()
{
#if DEBUG
	printf("PPPoEDevice: Destructor\n");
#endif
	
	free(fACName);
	free(fServiceName);
}


status_t
PPPoEDevice::InitCheck() const
{
	return EthernetIfnet() && EthernetIfnet()->output
		&& PPPDevice::InitCheck() == B_OK ? B_OK : B_ERROR;
}


bool
PPPoEDevice::Up()
{
#if DEBUG
	printf("PPPoEDevice: Up()\n");
#endif
	
	if(InitCheck() != B_OK)
		return false;
	
	LockerHelper locker(fLock);
	
	if(IsUp())
		return true;
	
	// reset connection settings
	memset(fPeer, 0xFF, sizeof(fPeer));
	
	// create PADI
	DiscoveryPacket discovery(PADI);
	if(fServiceName)
		discovery.AddTag(SERVICE_NAME, strlen(fServiceName), fServiceName);
	else
		discovery.AddTag(SERVICE_NAME, 0, NULL);
	discovery.AddTag(HOST_UNIQ, sizeof(uint32), &fHostUniq);
	discovery.AddTag(END_OF_LIST, 0, NULL);
	
	// set up PPP header
	struct mbuf *packet = discovery.ToMbuf();
	if(!packet)
		return false;
	
	// create destination
	struct sockaddr destination;
	memset(&destination, 0, sizeof(destination));
	destination.sa_family = AF_UNSPEC;
		// raw packet with ethernet header
	memcpy(destination.sa_data, fPeer, sizeof(fPeer));
	
	if(EthernetIfnet()->output(EthernetIfnet(), packet, &destination, NULL) != B_OK)
		return false;
	
	// check if we are allowed to go up now (user intervention might disallow that)
	if(!UpStarted()) {
		fState = INITIAL;
			// reset state
		DownEvent();
		return true;
	}
	
	fState = PADI_SENT;
	
	fNextTimeout = system_time() + PPPoE_TIMEOUT;
	
	return true;
}


bool
PPPoEDevice::Down()
{
#if DEBUG
	printf("PPPoEDevice: Down()\n");
#endif
	
	if(InitCheck() != B_OK)
		return false;
	
	LockerHelper locker(fLock);
	
	fNextTimeout = 0;
		// disable timeouts
	
	DownStarted();
		// this tells StateMachine that DownEvent() does not mean we lost connection
	
	if(!IsUp()) {
		DownEvent();
		return true;
	}
	
	// create PADT
	DiscoveryPacket discovery(PADT, SessionID());
	discovery.AddTag(END_OF_LIST, 0, NULL);
	
	struct mbuf *packet = discovery.ToMbuf();
	if(!packet) {
		DownEvent();
		return false;
	}
	
	// create destination
	struct sockaddr destination;
	memset(&destination, 0, sizeof(destination));
	destination.sa_family = AF_UNSPEC;
		// raw packet with ethernet header
	memcpy(destination.sa_data, fPeer, sizeof(fPeer));
	
	// reset connection settings
	memset(fPeer, 0xFF, sizeof(fPeer));
	
	EthernetIfnet()->output(EthernetIfnet(), packet, &destination, NULL);
	DownEvent();
	
	return false;
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
PPPoEDevice::Send(struct mbuf *packet, uint16 protocolNumber = 0)
{
#if DEBUG
	printf("PPPoEDevice: Send()\n");
#endif
	
	if(InitCheck() != B_OK || protocolNumber != 0) {
		m_freem(packet);
		return B_ERROR;
	} else if(!packet)
		return B_ERROR;
	
	LockerHelper locker(fLock);
	
	if(!IsUp())
		return PPP_NO_CONNECTION;
	
	uint16 length = packet->m_flags & M_PKTHDR ? packet->m_pkthdr.len : packet->m_len;
	
	// encapsulate packet into pppoe header
	M_PREPEND(packet, PPPoE_HEADER_SIZE);
	pppoe_header *header = mtod(packet, pppoe_header*);
	header->ethernetHeader.ether_type = ETHERTYPE_PPPOE;
	header->version = PPPoE_VERSION;
	header->type = PPPoE_TYPE;
	header->code = 0x00;
	header->sessionID = SessionID();
	header->length = htons(length);
	
	// create destination
	struct sockaddr destination;
	memset(&destination, 0, sizeof(destination));
	destination.sa_family = AF_UNSPEC;
		// raw packet with ethernet header
	memcpy(destination.sa_data, fPeer, sizeof(fPeer));
	
	locker.UnlockNow();
	
	if(EthernetIfnet()->output(EthernetIfnet(), packet, &destination, NULL) != B_OK) {
		printf("PPPoEDevice::Send(): EthernetIfnet()->output() failed!\n");
		DownEvent();
			// DownEvent() without DownStarted() indicates connection lost
		return PPP_NO_CONNECTION;
	}
	
	return B_OK;
}


status_t
PPPoEDevice::Receive(struct mbuf *packet, uint16 protocolNumber = 0)
{
#if DEBUG
	printf("PPPoEDevice: Receive()\n");
#endif
	
	if(InitCheck() != B_OK || IsDown()) {
		m_freem(packet);
		return B_ERROR;
	} else if(!packet)
		return B_ERROR;
	
	pppoe_header *header = mtod(packet, pppoe_header*);
	if(!header) {
		m_freem(packet);
		return B_ERROR;
	}
	
	status_t result = B_OK;
	
	if(header->ethernetHeader.ether_type == ETHERTYPE_PPPOE) {
		if(!IsUp() || header->version != PPPoE_VERSION || header->type != PPPoE_TYPE
				|| header->code != 0x0 || header->sessionID != SessionID()) {
			m_freem(packet);
			return B_ERROR;
		}
		
		m_adj(packet, PPPoE_HEADER_SIZE);
		return Interface().ReceiveFromDevice(packet);
	} else if(header->ethernetHeader.ether_type == ETHERTYPE_PPPOEDISC) {
		// we do not need to check HOST_UNIQ tag as this is done in pppoe.cpp
		if(header->version != PPPoE_VERSION || header->type != PPPoE_TYPE) {
			m_freem(packet);
			return B_ERROR;
		}
		
		LockerHelper locker(fLock);
		
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
				
				bool hasServiceName = false;
				pppoe_tag *tag;
				DiscoveryPacket reply(PADR);
				for(int32 index = 0; index < discovery.CountTags(); index++) {
					tag = discovery.TagAt(index);
					switch(tag->type) {
						case SERVICE_NAME:
							if(!hasServiceName && (!fServiceName
									|| !memcmp(tag->data, fServiceName, tag->length))) {
								hasServiceName = true;
								reply.AddTag(tag->type, tag->length, tag->data);
							}
						break;
						
						case AC_COOKIE:
						case RELAY_SESSION_ID:
							reply.AddTag(tag->type, tag->length, tag->data);
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
				
				reply.AddTag(END_OF_LIST, 0, NULL);
				struct mbuf *replyPacket = reply.ToMbuf();
				if(!replyPacket) {
					m_freem(packet);
					return B_ERROR;
				}
				
				memcpy(fPeer, header->ethernetHeader.ether_shost, sizeof(fPeer));
				
				// create destination
				struct sockaddr destination;
				memset(&destination, 0, sizeof(destination));
				destination.sa_family = AF_UNSPEC;
					// raw packet with ethernet header
				memcpy(destination.sa_data, fPeer, sizeof(fPeer));
				
				if(EthernetIfnet()->output(EthernetIfnet(), replyPacket, &destination,
						NULL) != B_OK) {
					m_freem(packet);
					return B_ERROR;
				}
				
				fState = PADR_SENT;
				fNextTimeout = system_time() + PPPoE_TIMEOUT;
			} break;
			
			case PADS:
				if(fState != PADR_SENT
						|| memcmp(header->ethernetHeader.ether_shost, fPeer,
							sizeof(fPeer))) {
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
						|| memcmp(header->ethernetHeader.ether_shost, fPeer,
							sizeof(fPeer))
						|| header->sessionID != SessionID()) {
					m_freem(packet);
					return B_ERROR;
				}
				
				fState = INITIAL;
				fSessionID = 0;
				fNextTimeout = 0;
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
	
	LockerHelper locker(fLock);
	
	// check if timed out
	if(system_time() >= fNextTimeout)
		Up();
}
