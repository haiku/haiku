/*
 * Copyright 2003-2006, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <cstdlib>

#include <ByteOrder.h>
#include <net/if_dl.h>
#include <net_stack.h>
#include <arpa/inet.h>

#include <ethernet.h>
#include <ether_driver.h>

#include "PPPoEDevice.h"
#include "DiscoveryPacket.h"

// from libkernelppp
#include <settings_tools.h>

extern net_stack_module_info *gStackModule;
extern net_buffer_module_info *gBufferModule;
extern status_t
pppoe_input(void *cookie, net_device *_device, net_buffer *packet);

#if DEBUG
static char sDigits[] = "0123456789ABCDEF";
void
dump_packet(net_buffer *packet)
{
	if (!packet)
		return;

	BufferHeaderReader<uint8> bufferheader(packet);
	if (bufferheader.Status() != B_OK)
		return;
	uint8 &buffer = bufferheader.Data();
	uint8 *data = &buffer;
	uint8 buffer[33];
	uint8 bufferIndex = 0;

	TRACE("Dumping packet;len=%ld;pkthdr.len=%d\n", packet->m_len,
		packet->m_flags & M_PKTHDR ? packet->m_pkthdr.len : -1);

	for (uint32 index = 0; index < packet->m_len; index++) {
		buffer[bufferIndex++] = sDigits[data[index] >> 4];
		buffer[bufferIndex++] = sDigits[data[index] & 0x0F];
		if (bufferIndex == 32 || index == packet->m_len - 1) {
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
	if (!settings || !settings->parameters)
		TRACE("PPPoEDevice::ctor: No settings!\n");
#endif

	interface.SetPFCOptions(PPP_ALLOW_PFC);
		// we do not want to fail just because the other side requests PFC

	memset(fPeer, 0xFF, sizeof(fPeer));

	SetMTU(1494);
		// MTU size does not contain PPP header

	if (!settings)
		dprintf("%s::%s: settings is NULL\n", __FILE__, __func__);

	// find ethernet device
	finterfaceName = get_parameter_value(PPPoE_INTERFACE_KEY, settings);
	TRACE("%s::%s: finterfaceName is %s\n", __FILE__, __func__,
			finterfaceName ? finterfaceName : "NULL");

	fACName = get_parameter_value(PPPoE_AC_NAME_KEY, settings);
	TRACE("fACName is %s\n", fACName ? fACName : "NULL");

	fServiceName = get_parameter_value(PPPoE_SERVICE_NAME_KEY, settings);
	TRACE("fServiceName is %s\n", fServiceName ? fServiceName : "NULL");

	// fEthernetIfnet = FindPPPoEInterface(interfaceName);

#if DEBUG
	if (!fEthernetIfnet)
		TRACE("PPPoEDevice::ctor: could not find ethernet interface\n");
	else
		TRACE("%s::%s: Find Ethernet device", __FILE__, __func__);
#endif
}


status_t
PPPoEDevice::InitCheck() const
{
	if (KPPPDevice::InitCheck() != B_OK)
		dprintf("%s::%s: KPPPDevice::InitCheck() has error\n", __FILE__, __func__);

	return KPPPDevice::InitCheck() == B_OK ? B_OK : B_ERROR;
}


bool
PPPoEDevice::Up()
{
	TRACE("PPPoEDevice: Up()\n");

	if (InitCheck() != B_OK)
		return false;

	if (IsUp())
		return true;

	fEthernetIfnet = FindPPPoEInterface(finterfaceName);

	if (fEthernetIfnet == NULL) {
		dprintf("%s::%s: fEthernetIfnet %s not found\n", __FILE__, __func__, finterfaceName);
		return false;
	}

	memcpy(fEtherAddr, fEthernetIfnet->address.data, ETHER_ADDRESS_LENGTH);
	dprintf("ppp's corresponding addr is %02x:%02x:%02x:%02x:%02x:%02x\n",
		fEtherAddr[0], fEtherAddr[1], fEtherAddr[2], fEtherAddr[3],
		fEtherAddr[4], fEtherAddr[5]);

	if (fEthernetIfnet->module == NULL) {
		dprintf("%s::%s: fEthernetIfnet->module not found\n", __FILE__,
			__func__);
		return false;
	}

	add_device(this);

	fState = INITIAL;
		// reset state

	if (fAttempts > PPPoE_MAX_ATTEMPTS) {
		fAttempts = 0;
		return false;
	}

	++fAttempts;
	// reset connection settings
	memset(fPeer, 0xFF, sizeof(fPeer));

	// create PADI
	DiscoveryPacket discovery(PADI);
	if (ServiceName())
		discovery.AddTag(SERVICE_NAME, ServiceName(), strlen(ServiceName()));
	else
		discovery.AddTag(SERVICE_NAME, NULL, 0);
	discovery.AddTag(HOST_UNIQ, &fHostUniq, sizeof(fHostUniq));
	discovery.AddTag(END_OF_LIST, NULL, 0);

	// set up PPP header
	net_buffer *packet = discovery.ToNetBuffer(MTU());
	if (!packet)
		return false;

	// create ether header
	NetBufferPrepend<ether_header> ethernetHeader(packet);
	if (ethernetHeader.Status() != B_OK)
		return false;
	ether_header &header = ethernetHeader.Data();

	memset(header.destination, 0xff, ETHER_ADDRESS_LENGTH);
	memcpy(header.source, fEtherAddr, ETHER_ADDRESS_LENGTH);
	header.type = htons(ETHER_TYPE_PPPOE_DISCOVERY);
	ethernetHeader.Sync();
	// raw packet with ethernet header

	// check if we are allowed to go up now (user intervention might disallow that)
	if (fAttempts > 0 && !UpStarted()) {
		fAttempts = 0;
		remove_device(this);
		DownEvent();
		return true;
			// there was no error
	}

	fState = PADI_SENT;
		// needed before sending, otherwise we might not get all packets

	status_t status = gStackModule->register_device_handler(fEthernetIfnet,
		B_NET_FRAME_TYPE_PPPOE_DISCOVERY, &pppoe_input, NULL);
	if (status != B_OK)
		return false;

	status = gStackModule->register_device_handler(fEthernetIfnet,
		B_NET_FRAME_TYPE_PPPOE, &pppoe_input, NULL);
	if (status != B_OK)
		return false;

	if (EthernetIfnet()->module->send_data(EthernetIfnet(), packet) != B_OK) {
		fState = INITIAL;
		fAttempts = 0;
		ERROR("PPPoEDevice::Up(): EthernetIfnet()->output() failed!\n");
		return false;
	}

	dprintf("PPPoEDevice::Up(): EthernetIfnet()->output() success!\n");
	fNextTimeout = system_time() + PPPoE_TIMEOUT;

	return true;
}


bool
PPPoEDevice::Down()
{
	TRACE("PPPoEDevice: Down()\n");

	gStackModule->unregister_device_handler(fEthernetIfnet,
		B_NET_FRAME_TYPE_PPPOE_DISCOVERY);
	gStackModule->unregister_device_handler(fEthernetIfnet,
		B_NET_FRAME_TYPE_PPPOE);

	remove_device(this);

	if (InitCheck() != B_OK)
		return false;

	fState = INITIAL;
	fAttempts = 0;
	fNextTimeout = 0;
		// disable timeouts

	if (!IsUp()) {
		DownEvent();
		return true;
	}

	// this tells StateMachine that DownEvent() does not mean we lost connection
	DownStarted();

	// create PADT
	DiscoveryPacket discovery(PADT, SessionID());
	discovery.AddTag(END_OF_LIST, NULL, 0);

	net_buffer *packet = discovery.ToNetBuffer(MTU());
	if (!packet) {
		ERROR("PPPoEDevice::Down(): ToNetBuffer() failed; MTU=%" B_PRIu32 "\n",
			MTU());
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
	ethernetHeader->type = ETHER_TYPE_PPPOE_DISCOVERY;
	memcpy(ethernetHeader->destination, fPeer, sizeof(fPeer));

	// reset connection settings
	memset(fPeer, 0xFF, sizeof(fPeer));

	EthernetIfnet()->module->send_data(EthernetIfnet(), packet);
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
PPPoEDevice::Send(net_buffer *packet, uint16 protocolNumber)
{
	// Send() is only for PPP packets. PPPoE packets are sent directly to ethernet.

	TRACE("PPPoEDevice: Send()\n");

	if (!packet)
		return B_ERROR;
	else if (InitCheck() != B_OK || protocolNumber != 0) {
		ERROR("PPPoEDevice::Send(): InitCheck() != B_OK!\n");
		gBufferModule->free(packet);
		return B_ERROR;
	}

	if (!IsUp()) {
		ERROR("PPPoEDevice::Send(): no connection!\n");
		gBufferModule->free(packet);
		return PPP_NO_CONNECTION;
	}

	uint16 length = packet->size;

	// encapsulate packet into pppoe header
	NetBufferPrepend<pppoe_header> bufferheader(packet);
	if (bufferheader.Status() != B_OK)
		return B_ERROR;
	pppoe_header &header = bufferheader.Data();
	header.version = PPPoE_VERSION;
	header.type = PPPoE_TYPE;
	header.code = 0x00;
	header.sessionID = SessionID();
	header.length = htons(length);
	bufferheader.Sync();

	// create ether header
	NetBufferPrepend<ether_header> ethernetHeader(packet);
	if (ethernetHeader.Status() != B_OK)
		return false;
	ether_header &ethheader = ethernetHeader.Data();

	memcpy(ethheader.destination, fPeer, ETHER_ADDRESS_LENGTH);
	memcpy(ethheader.source, fEtherAddr, ETHER_ADDRESS_LENGTH);
	ethheader.type = htons(ETHER_TYPE_PPPOE);
	ethernetHeader.Sync();
	// raw packet with ethernet header

	if (!packet)
		ERROR("PPPoEDevice::Send(): packet is NULL!\n");

	if (EthernetIfnet()->module->send_data(EthernetIfnet(), packet) != B_OK) {
		ERROR("PPPoEDevice::Send(): EthernetIfnet()->output() failed!\n");
		remove_device(this);
		DownEvent();
			// DownEvent() without DownStarted() indicates connection lost
		return PPP_NO_CONNECTION;
	}

	return B_OK;
}


status_t
PPPoEDevice::Receive(net_buffer *packet, uint16 protocolNumber)
{
	TRACE("%s entering %s: protocolNumber:%x\n", __FILE__, __func__, protocolNumber);
	if (!packet)
		return B_ERROR;

	if (InitCheck() != B_OK || IsDown()) {
		dprintf("PPPoED InitCheck fail or IsDown\n");
		// gBufferModule->free(packet);
		dprintf("PPPoEDevice::Receive fail\n");
		return B_ERROR;
	}

	uint8 ethernetSource[6];
	struct sockaddr_dl& source = *(struct sockaddr_dl*)packet->source;
	memcpy(ethernetSource, source.sdl_data, sizeof(fPeer));

	int32 PPP_Packet_Type = B_NET_FRAME_TYPE(source.sdl_type,
				ntohs(source.sdl_e_type));
//	bufferheader.Remove();
//	bufferheader.Sync();

	if (PPP_Packet_Type == B_NET_FRAME_TYPE_PPPOE) {
		TRACE("ETHER_TYPE_PPPOE\n");
		NetBufferHeaderReader<pppoe_header> bufferheader(packet);
		if (bufferheader.Status() != B_OK) {
			TRACE("creat NetBufferHeaderReader fail\n");
			return B_ERROR;
		}
		pppoe_header &header = bufferheader.Data();
		uint16 ppppoe_payload = ntohs(header.length);

		if (!IsUp() || header.version != PPPoE_VERSION || header.type != PPPoE_TYPE
				|| header.code != 0x0 || header.sessionID != SessionID()) {
			// gBufferModule->free(packet);
			TRACE("basic pppoe header check fail\n");
			return B_ERROR;
		}

		bufferheader.Remove();
		bufferheader.Sync();

		// trim the packet according to actual pppoe_payload
		gBufferModule->trim(packet, ppppoe_payload);

		return Interface().ReceiveFromDevice(packet);
	}

	if (PPP_Packet_Type == B_NET_FRAME_TYPE_PPPOE_DISCOVERY) {
		dprintf("ETHER_TYPE_PPPOE_DISCOVERY\n");
		NetBufferHeaderReader<pppoe_header> bufferheader(packet);
		if (bufferheader.Status() != B_OK) {
			dprintf("create NetBufferHeaderReader fail\n");
			return B_ERROR;
		}
		pppoe_header &header = bufferheader.Data();

		// we do not need to check HOST_UNIQ tag as this is done in pppoe.cpp
		if (header.version != PPPoE_VERSION || header.type != PPPoE_TYPE) {
			// gBufferModule->free(packet);
			dprintf("PPPoEDevice::Receive fail version type wrong\n");
			return B_ERROR;
		}

		if (IsDown()) {
			// gBufferModule->free(packet);
			dprintf("PPPoEDevice::Receive fail PPPoEDev IsDown\n");
			return B_ERROR;
		}

		DiscoveryPacket discovery(packet);
		switch(discovery.Code()) {
			case PADO: {
				dprintf("processing PADO\n");
				if (fState != PADI_SENT) {
					// gBufferModule->free(packet);
					dprintf("PPPoEDevice::Receive faile not PADI_Sent \n");
					return B_OK;
				}

				bool hasServiceName = false, hasACName = false;
				pppoe_tag *tag;
				DiscoveryPacket reply(PADR);
				for (int32 index = 0; index < discovery.CountTags(); index++) {
					tag = discovery.TagAt(index);
					if (!tag)
						continue;

					switch (tag->type) {
						case SERVICE_NAME:
							if (!hasServiceName && (!ServiceName()
									|| ((strlen(ServiceName()) == tag->length)
										&& !memcmp(tag->data, ServiceName(),
											tag->length)))) {
								hasServiceName = true;
								reply.AddTag(tag->type, tag->data, tag->length);
							}
						break;

						case AC_NAME:
							if (!hasACName && (!ACName()
									|| ((strlen(ACName()) == tag->length)
										&& !memcmp(tag->data, ACName(),
											tag->length)))) {
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
							// gBufferModule->free(packet);
							dprintf("PPPoEDevice::generic error faile\n");
							return B_ERROR;
						break;

						default:
							;
					}
				}

				if (!hasServiceName) {
					// gBufferModule->free(packet);
					dprintf("PPPoEDevice::Receive faile no svc name\n");
					return B_ERROR;
				}

				dprintf("reply.AddTag\n");
				reply.AddTag(HOST_UNIQ, &fHostUniq, sizeof(fHostUniq));
				reply.AddTag(END_OF_LIST, NULL, 0);
				net_buffer *replyPacket = reply.ToNetBuffer(MTU());
				if (!replyPacket) {
					// gBufferModule->free(packet);
					dprintf("PPPoEDevice::Receive fail no reply pack\n");
					return B_ERROR;
				}

				memcpy(fPeer, ethernetSource, ETHER_ADDRESS_LENGTH);

				// create ether header
				NetBufferPrepend<ether_header> ethernetHeader(replyPacket);
				if (ethernetHeader.Status() != B_OK)
					return false;
				ether_header &header = ethernetHeader.Data();

				memcpy(header.destination, fPeer, ETHER_ADDRESS_LENGTH);
				memcpy(header.source, fEtherAddr, ETHER_ADDRESS_LENGTH);
				header.type=htons(ETHER_TYPE_PPPOE_DISCOVERY);
				ethernetHeader.Sync();
				// raw packet with ethernet header

				fState = PADR_SENT;

				if (EthernetIfnet()->module->send_data(EthernetIfnet(), replyPacket) != B_OK) {
					gBufferModule->free(replyPacket);
					dprintf("PPPoEDevice::Receive fail send PADR\n");
					return B_ERROR;
				}

				fNextTimeout = system_time() + PPPoE_TIMEOUT;
			}
			break;
			case PADS:
				dprintf("procesing PADS\n");
				if (fState != PADR_SENT
					|| memcmp(ethernetSource, fPeer, sizeof(fPeer))) {
					// gBufferModule->free(packet);
					dprintf("PPPoEDevice::Receive PADS but no PADR sent\n");
					return B_ERROR;
				}

				fSessionID = header.sessionID;
				fState = OPENED;
				fNextTimeout = 0;
				UpEvent();
			break;

			case PADT:
				dprintf("procesing PADT\n");
				if (!IsUp()
						|| memcmp(ethernetSource, fPeer, sizeof(fPeer))
						|| header.sessionID != SessionID()) {
					// gBufferModule->free(packet);
					dprintf("PPPoEDevice::Receive fail not up yet\n");
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
				dprintf("PPPoEDevice::Receive fail unknown pppoed code\n");
				// gBufferModule->free(packet);
				return B_ERROR;
		}
	}

	dprintf("PPPoEDevice::Receive PADX fine!\n");
	// gBufferModule->free(packet);
	return B_OK;
}


void
PPPoEDevice::Pulse()
{
	// We use Pulse() for timeout of connection establishment.
	if (fNextTimeout == 0 || IsUp() || IsDown())
		return;

	// check if timed out
	if (system_time() >= fNextTimeout) {
		if (!Up())
			UpFailedEvent();
	}
}
