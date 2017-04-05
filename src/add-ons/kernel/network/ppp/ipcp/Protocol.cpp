/*
 * Copyright 2003-2006, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#if DEBUG
#include <cstdio>
#endif

#include "Protocol.h"
#include "IPCP.h"
#include <KPPPConfigurePacket.h>
#include <KPPPInterface.h>
#include <settings_tools.h>

#include <PPPoEDevice.h>

#include <cstring>

#include <arpa/inet.h>
#include <net_buffer.h>
#include <net_stack.h>
#include <net/route.h>
#include <sys/sockio.h>

// For updating resolv.conf
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RESOLV_CONF_FILE "/boot/system/settings/network/resolv.conf"

extern net_buffer_module_info *gBufferModule;
static struct net_datalink_module_info *sDatalinkModule;
static struct net_stack_module_info *sStackModule;

#if DEBUG
#include <unistd.h>

static int sFD;
	// the file descriptor for debug output
static char sDigits[] = "0123456789ABCDEF";
void
dump_packet(net_buffer *packet, const char *direction)
{
	if (!packet)
		return;

	uint8 *data = mtod(packet, uint8*);
	uint8 buffer[128];
	uint8 bufferIndex = 0;

	sprintf((char*) buffer, "Dumping %s packet;len=%ld;pkthdr.len=%d\n", direction,
		packet->m_len, packet->m_flags & M_PKTHDR ? packet->m_pkthdr.len : -1);
	write(sFD, buffer, strlen((char*) buffer));

	for (uint32 index = 0; index < packet->m_len; index++) {
		buffer[bufferIndex++] = sDigits[data[index] >> 4];
		buffer[bufferIndex++] = sDigits[data[index] & 0x0F];
		if (bufferIndex == 32 || index == packet->m_len - 1) {
			buffer[bufferIndex++] = '\n';
			buffer[bufferIndex] = 0;
			write(sFD, buffer, strlen((char*) buffer));
			bufferIndex = 0;
		}
	}
}
#endif


static const bigtime_t kIPCPStateMachineTimeout = 3000000;
	// 3 seconds


IPCP::IPCP(KPPPInterface& interface, driver_parameter *settings)
	: KPPPProtocol("IPCP", PPP_NCP_PHASE, IPCP_PROTOCOL, PPP_PROTOCOL_LEVEL,
		AF_INET, 0, interface, settings, PPP_INCLUDES_NCP),
	fDefaultRoute(NULL),
	fRequestPrimaryDNS(false),
	fRequestSecondaryDNS(false),
	fState(PPP_INITIAL_STATE),
	fID(system_time() & 0xFF),
	fMaxRequest(10),
	fMaxTerminate(2),
	fMaxNak(5),
	fRequestID(0),
	fTerminateID(0),
	fNextTimeout(0)
{
	// reset configurations
	memset(&fLocalConfiguration, 0, sizeof(ipcp_configuration));
	memset(&fPeerConfiguration, 0, sizeof(ipcp_configuration));

	// reset requests
	memset(&fLocalRequests, 0, sizeof(ipcp_requests));
	memset(&fPeerRequests, 0, sizeof(ipcp_requests));

	// Parse settings:
	// "Local" and "Peer" describe each side's settings
	ParseSideRequests(get_parameter_with_name(IPCP_LOCAL_SIDE_KEY, Settings()),
		PPP_LOCAL_SIDE);
	ParseSideRequests(get_parameter_with_name(IPCP_PEER_SIDE_KEY, Settings()),
		PPP_PEER_SIDE);

	get_module(NET_STACK_MODULE_NAME, (module_info **)&sStackModule);
	get_module(NET_DATALINK_MODULE_NAME, (module_info **)&sDatalinkModule);
#if DEBUG
	sFD = open("/boot/home/ipcpdebug", O_WRONLY | O_CREAT | O_TRUNC);
#endif
}


IPCP::~IPCP()
{

	put_module(NET_DATALINK_MODULE_NAME);
	put_module(NET_STACK_MODULE_NAME);
#if DEBUG
	close(sFD);
#endif
}


void
IPCP::Uninit()
{
	RemoveRoutes();
}


status_t
IPCP::StackControl(uint32 op, void *data)
{
	TRACE("IPCP: StackControl(op=%ld)\n", op);

	// TODO:
	// check values

	switch (op) {
		case SIOCSIFADDR:
		break;

		case SIOCSIFFLAGS:
		break;

		case SIOCSIFDSTADDR:
		break;

		case SIOCSIFNETMASK:
		break;

		default:
			ERROR("IPCP: Unknown ioctl: %" B_PRIu32 "\n", op);
			return KPPPProtocol::StackControl(op, data);
	}

	return B_OK;
}


bool
IPCP::Up()
{
	TRACE("IPCP: Up() state=%d\n", State());

	// Servers do not send a configure-request when Up() is called. They wait until
	// the client requests this protocol.
	if (Interface().Mode() == PPP_SERVER_MODE)
		return true;

	switch (State()) {
		case PPP_INITIAL_STATE:
			NewState(PPP_REQ_SENT_STATE);
			InitializeRestartCount();
			SendConfigureRequest();
		break;

		default:
			;
	}

	return true;
}


bool
IPCP::Down()
{
	TRACE("IPCP: Down() state=%d\n", State());

	switch (Interface().Phase()) {
		case PPP_DOWN_PHASE:
			// interface finished terminating
			NewState(PPP_INITIAL_STATE);
			ReportDownEvent();
				// this will also reset and update addresses
		break;

/*		case PPP_TERMINATION_PHASE:
			// interface is terminating
		break;

		case PPP_ESTABLISHMENT_PHASE:
			// interface is reconfiguring
		break;
*/
		case PPP_ESTABLISHED_PHASE:
			// terminate this NCP individually (block until we finished terminating)
			if (State() != PPP_INITIAL_STATE && State() != PPP_CLOSING_STATE) {
				NewState(PPP_CLOSING_STATE);
				InitializeRestartCount();
				SendTerminateRequest();
			}

			while (State() == PPP_CLOSING_STATE)
				snooze(50000);
		break;

		default:
			;
	}

	return true;
}


status_t
IPCP::Send(net_buffer *packet, uint16 protocolNumber)
{
	TRACE("IPCP: Send(0x%X)\n", protocolNumber);

	if ((protocolNumber == IP_PROTOCOL && State() == PPP_OPENED_STATE)
			|| protocolNumber == IPCP_PROTOCOL) {
#if DEBUG
		dump_packet(packet, "outgoing");
#endif
		Interface().UpdateIdleSince();
		return SendToNext(packet, protocolNumber);
	}

	ERROR("IPCP: Send() failed because of wrong state or protocol number!\n");

	gBufferModule->free(packet);
	return B_ERROR;
}


status_t
IPCP::Receive(net_buffer *packet, uint16 protocolNumber)
{
	TRACE("IPCP: Receive(0x%X)\n", protocolNumber);

	if (!packet)
		return B_ERROR;

	if (protocolNumber == IP_PROTOCOL)
		return ReceiveIPPacket(packet, protocolNumber);

	if (protocolNumber != IPCP_PROTOCOL)
		return PPP_UNHANDLED;

	NetBufferHeaderReader<ppp_lcp_packet> bufferheader(packet);
	if (bufferheader.Status() != B_OK)
		return B_ERROR;
	ppp_lcp_packet &data = bufferheader.Data();

	if (ntohs(data.length) < 4)
		return B_ERROR;

	// packet is freed by event methods
	switch (data.code) {
		case PPP_CONFIGURE_REQUEST:
			RCREvent(packet);
		break;

		case PPP_CONFIGURE_ACK:
			RCAEvent(packet);
		break;

		case PPP_CONFIGURE_NAK:
		case PPP_CONFIGURE_REJECT:
			RCNEvent(packet);
		break;

		case PPP_TERMINATE_REQUEST:
			RTREvent(packet);
		break;

		case PPP_TERMINATE_ACK:
			RTAEvent(packet);
		break;

		case PPP_CODE_REJECT:
			RXJBadEvent(packet);
				// we implemented the minimum requirements
		break;

		default:
			RUCEvent(packet);
			return PPP_REJECTED;
	}

	return B_OK;
}


status_t
IPCP::ReceiveIPPacket(net_buffer *packet, uint16 protocolNumber)
{
	if (protocolNumber != IP_PROTOCOL || State() != PPP_OPENED_STATE)
		return PPP_UNHANDLED;

	// TODO: add VJC support (the packet would be decoded here)

	if (packet)
	{
#if DEBUG
		dump_packet(packet, "incoming");
#endif
		Interface().UpdateIdleSince();

		TRACE("We got 1 IP packet from %s::%s\n", __FILE__, __func__);
		gBufferModule->free(packet);
		return B_OK;
	} else {
		ERROR("IPCP: Error: Could not find input function for IP!\n");
		gBufferModule->free(packet);
		return B_ERROR;
	}
}


void
IPCP::Pulse()
{
	if (fNextTimeout == 0 || fNextTimeout > system_time())
		return;
	fNextTimeout = 0;

	switch (State()) {
		case PPP_CLOSING_STATE:
			if (fTerminateCounter <= 0)
				TOBadEvent();
			else
				TOGoodEvent();
		break;

		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			if (fRequestCounter <= 0)
				TOBadEvent();
			else
				TOGoodEvent();
		break;

		default:
			;
	}
}


bool
IPCP::ParseSideRequests(const driver_parameter *requests, ppp_side side)
{
	if (!requests)
		return false;

	ipcp_requests *selectedRequests;

	if (side == PPP_LOCAL_SIDE) {
		selectedRequests = &fLocalRequests;
		fRequestPrimaryDNS = fRequestSecondaryDNS = false;
	} else
		selectedRequests = &fPeerRequests;

	memset(selectedRequests, 0, sizeof(ipcp_requests));
		// reset current requests

	// The following values are allowed:
	//  "Address"		the ip address that will be suggested
	//  "Netmask"		the netmask that should be used
	//  "PrimaryDNS"	primary DNS server
	//  "SecondaryDNS"	secondary DNS server
	// Setting any value to 0.0.0.0 or "auto" means it should be chosen automatically.

	in_addr_t address = INADDR_ANY;
	for (int32 index = 0; index < requests->parameter_count; index++) {
		if (requests->parameters[index].value_count == 0)
			continue;

		// all values are IP addresses, so parse the address here
		if (strcasecmp(requests->parameters[index].values[0], "auto")) {
			address = inet_addr(requests->parameters[index].values[0]);
			// address = INADDR_ANY;
			if (address == INADDR_NONE)
				continue;
		}

		if (!strcasecmp(requests->parameters[index].name, IPCP_IP_ADDRESS_KEY))
			selectedRequests->address = address;
		else if (!strcasecmp(requests->parameters[index].name, IPCP_NETMASK_KEY))
			selectedRequests->netmask = address;
		else if (!strcasecmp(requests->parameters[index].name, IPCP_PRIMARY_DNS_KEY)) {
			selectedRequests->primaryDNS = address;
			if (side == PPP_LOCAL_SIDE)
				fRequestPrimaryDNS = true;
		} else if (!strcasecmp(requests->parameters[index].name,
				IPCP_SECONDARY_DNS_KEY)) {
			selectedRequests->secondaryDNS = address;
			if (side == PPP_LOCAL_SIDE)
				fRequestSecondaryDNS = true;
		}
	}

	return true;
}


net_interface *
get_interface_by_name(net_domain *domain, const char *name)
{
	ifreq request;
	memset(&request, 0, sizeof(request));
	size_t size = sizeof(request);

	strlcpy(request.ifr_name, name, IF_NAMESIZE);

	if (sDatalinkModule->control(domain, SIOCGIFINDEX, &request, &size) != B_OK) {
		TRACE("sDatalinkModule->control failure\n");
		return NULL;
	}
	return sDatalinkModule->get_interface(domain, request.ifr_index);
}


status_t
set_interface_address(net_domain* domain, struct ifaliasreq* inreq)
{
	size_t size = sizeof(struct ifaliasreq);
	return sDatalinkModule->control(domain, B_SOCKET_SET_ALIAS, inreq, &size);
}


void
IPCP::UpdateAddresses()
{
	TRACE("%s::%s: entering UpdateAddresses\n", __FILE__, __func__);
	RemoveRoutes();

	if (State() != PPP_OPENED_STATE && !Interface().DoesConnectOnDemand())
		return;

	TRACE("%s::%s: entering ChangeAddress\n", __FILE__, __func__);
	if (sDatalinkModule == NULL) {
		TRACE("%s::%s: some module not found!\n", __FILE__, __func__);
		return;
	}


	struct sockaddr newAddr = {6, AF_INET, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
	struct sockaddr netmask = {6, AF_INET, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
	struct sockaddr broadaddr = {6, AF_INET, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

	if (fLocalRequests.address != INADDR_ANY)
		memcpy(newAddr.sa_data + 2, &fLocalRequests.address, sizeof(in_addr_t));
	else if (fLocalConfiguration.address == INADDR_ANY) {
		in_addr_t inaddrBroadcast = 0x010F0F0F; // was: INADDR_BROADCAST
		memcpy(newAddr.sa_data + 2, &inaddrBroadcast, sizeof(in_addr_t));
	} else
		memcpy(newAddr.sa_data + 2, &fLocalConfiguration.address, sizeof(in_addr_t));

	struct ifaliasreq inreq;
	memset(&inreq, 0, sizeof(struct ifaliasreq));
	memcpy(inreq.ifra_name, Interface().Name(), IF_NAMESIZE);
	memcpy(&inreq.ifra_addr, &newAddr, sizeof(struct sockaddr));
	memcpy(&inreq.ifra_mask, &netmask, sizeof(struct sockaddr));
	memcpy(&inreq.ifra_broadaddr, &broadaddr, sizeof(struct sockaddr));
	inreq.ifra_index = -1;
		// create a new interface address
		// Is it OK when we already have one?
		// test case:	ifconfig ppp up
		// 		ifconfig ppp down
		// 		ifconfig ppp up
		// 		check if some weird things happen

	net_domain* domain = sStackModule->get_domain(AF_INET);
	status_t status = set_interface_address(domain, &inreq);

	if (status != B_OK) {
		TRACE("%s:%s: set_interface_address Fail!!!!\n", __FILE__, __func__);
		return;
	}
	TRACE("%s:%s: set_interface_address fine\n", __FILE__, __func__);


	net_interface* pppInterface = get_interface_by_name(domain, Interface().Name());
	if (pppInterface == NULL) {
		TRACE("%s::%s: pppInterface not found!\n", __FILE__, __func__);
		return;
	}

	net_interface_address* pppInterfaceAddress = NULL;
	while (sDatalinkModule->get_next_interface_address(pppInterface,
				&pppInterfaceAddress)) {
		if (pppInterfaceAddress->domain->family != AF_INET)
			continue;
		break;
	}

	// add default/subnet route
	if (Side() == PPP_LOCAL_SIDE && pppInterfaceAddress != NULL) {
		struct sockaddr addrGateway = {8, AF_INET, {0x00, 0x00, 0x00, 0x00,
			0x00, 0x00} };

		// create destination address
		if (fPeerRequests.address != INADDR_ANY)
			memcpy(addrGateway.sa_data + 2, &fPeerRequests.address,
				sizeof(in_addr_t));
		else if (fPeerConfiguration.address == INADDR_ANY) {
			in_addr_t gateway = 0x020F0F0F;
			memcpy(addrGateway.sa_data + 2, &gateway, sizeof(in_addr_t));
				// was: INADDR_BROADCAST
		} else
			memcpy(addrGateway.sa_data + 2, &fPeerConfiguration.address,
				sizeof(in_addr_t));

		net_route defaultRoute;
		defaultRoute.destination = NULL;
		defaultRoute.mask = NULL;
		defaultRoute.gateway = &addrGateway;
		defaultRoute.flags = RTF_DEFAULT | RTF_GATEWAY;
		defaultRoute.interface_address = pppInterfaceAddress;
			// route->interface_address;

		status_t status = sDatalinkModule->add_route(domain, &defaultRoute);
		if (status == B_OK)
			dprintf("%s::%s: add route default OK!\n", __FILE__, __func__);
		else
			dprintf("%s::%s: add route default Fail!\n", __FILE__, __func__);

		sDatalinkModule->put_interface_address(pppInterfaceAddress);
	}

	if (Side() == PPP_LOCAL_SIDE) {
		int file;
		int primary_dns, secondary_dns;
		char buf[256];

		file = open(RESOLV_CONF_FILE, O_RDWR);

		primary_dns = ntohl(fLocalConfiguration.primaryDNS);
		secondary_dns = ntohl(fLocalConfiguration.secondaryDNS);

		sprintf(buf, "%s\t%d.%d.%d.%d\n%s\t%d.%d.%d.%d\n",
				"nameserver",
				(primary_dns & 0xff000000) >> 24,
				(primary_dns & 0x00ff0000) >> 16,
				(primary_dns & 0x0000ff00) >> 8,
				(primary_dns & 0x000000ff),
				"nameserver",
				(secondary_dns & 0xff000000) >> 24,
				(secondary_dns & 0x00ff0000) >> 16,
				(secondary_dns & 0x0000ff00) >> 8,
				(secondary_dns & 0x000000ff));

		write(file, buf, strlen(buf));
		close(file);
	}
}


void
IPCP::RemoveRoutes()
{
	// note:
	// 	haiku supports multi default route. But for Desktop, ppp is generally
	// 	the only default route. So is it necessary to remove other default
	// 	route?
	TRACE("%s::%s: entering RemoveRoutes!\n", __FILE__, __func__);

	char *ethernetName = NULL;
	PPPoEDevice* pppoeDevice = (PPPoEDevice *)Interface().Device();
	if (pppoeDevice == NULL)
		return;
	ethernetName = pppoeDevice->EthernetIfnet()->name;
	if (ethernetName == NULL)
		return;

	net_domain* domain = sStackModule->get_domain(AF_INET);
	net_interface* pppInterface = get_interface_by_name(domain, ethernetName);

	if (pppInterface == NULL) {
		TRACE("%s::%s: pppInterface not found!\n", __FILE__, __func__);
		return;
	}

	net_interface_address* pppInterfaceAddress = NULL;

	while (sDatalinkModule->get_next_interface_address(pppInterface,
				&pppInterfaceAddress)) {
		if (pppInterfaceAddress->domain->family != AF_INET)
			continue;

		net_route oldDefaultRoute;
		oldDefaultRoute.destination = NULL;
		oldDefaultRoute.mask = NULL;
		oldDefaultRoute.gateway = NULL;
		oldDefaultRoute.flags= RTF_DEFAULT;
		oldDefaultRoute.interface_address = pppInterfaceAddress;

		status_t status = sDatalinkModule->remove_route(domain, &oldDefaultRoute);
			// current: can not get the system default route so we fake
			// 	    one default route for delete
			// Todo: save the oldDefaultRoute to fDefaultRoute
			// 	 restore the fDefaultRoute when ppp is down

		sDatalinkModule->put_interface_address(pppInterfaceAddress);

		if (status == B_OK)
			dprintf("IPCP::RemoveRoutes: remove old default route OK!\n");
		else
			dprintf("IPCP::RemoveRoutes: remove old default route Fail!\n");

		break;
	}

	if (fDefaultRoute) {
		struct sockaddr_in netmask;
		memset(&netmask, 0, sizeof(struct sockaddr_in));

		netmask.sin_family = AF_INET;
		netmask.sin_addr.s_addr = fLocalRequests.netmask;
		netmask.sin_len = sizeof(struct sockaddr_in);

		// if (rtrequest(RTM_DELETE, (struct sockaddr*) &netmask,
				// (struct sockaddr*) &fGateway, (struct sockaddr*) &netmask,
				// RTF_UP | RTF_GATEWAY, &fDefaultRoute) != B_OK)
			ERROR("IPCP: RemoveRoutes(): could not remove default/subnet route!\n");

		fDefaultRoute = NULL;
	}
}


uint8
IPCP::NextID()
{
	return (uint8) atomic_add(&fID, 1);
}


void
IPCP::NewState(ppp_state next)
{
	TRACE("IPCP: NewState(%d) state=%d\n", next, State());

	// report state changes
	if (State() == PPP_INITIAL_STATE && next != State())
		UpStarted();
	else if (State() == PPP_OPENED_STATE && next != State())
		DownStarted();

	// maybe we do not need the timer anymore
	if (next < PPP_CLOSING_STATE || next == PPP_OPENED_STATE)
		fNextTimeout = 0;

	fState = next;
}


void
IPCP::TOGoodEvent()
{
#if DEBUG
	printf("IPCP: TOGoodEvent() state=%d\n", State());
#endif

	switch (State()) {
		case PPP_CLOSING_STATE:
			SendTerminateRequest();
		break;

		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);

		case PPP_REQ_SENT_STATE:
		case PPP_ACK_SENT_STATE:
			SendConfigureRequest();
		break;

		default:
			IllegalEvent(PPP_TO_GOOD_EVENT);
	}
}


void
IPCP::TOBadEvent()
{
	TRACE("IPCP: TOBadEvent() state=%d\n", State());

	switch (State()) {
		case PPP_CLOSING_STATE:
			NewState(PPP_INITIAL_STATE);
			ReportDownEvent();
		break;

		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_INITIAL_STATE);
			ReportUpFailedEvent();
		break;

		default:
			IllegalEvent(PPP_TO_BAD_EVENT);
	}
}


void
IPCP::RCREvent(net_buffer *packet)
{
	TRACE("IPCP: RCREvent() state=%d\n", State());

	KPPPConfigurePacket request(packet);
	KPPPConfigurePacket nak(PPP_CONFIGURE_NAK);
	KPPPConfigurePacket reject(PPP_CONFIGURE_REJECT);

	NetBufferHeaderReader<ppp_lcp_packet> bufferheader(packet);
	if (bufferheader.Status() != B_OK)
		return;
	ppp_lcp_packet &lcpHeader = bufferheader.Data();

	// we should not use the same id as the peer
	if (fID == lcpHeader.id)
		fID -= 128;

	nak.SetID(request.ID());
	reject.SetID(request.ID());

	// parse each item
	ppp_configure_item *item;
	in_addr_t *requestedAddress, *wishedAddress = NULL;
	for (int32 index = 0; index < request.CountItems(); index++) {
		item = request.ItemAt(index);
		if (!item)
			continue;

		// addresses have special handling to reduce code size
		switch (item->type) {
			case IPCP_ADDRESSES:
				// abandoned by the standard
			case IPCP_ADDRESS:
				wishedAddress = &fPeerRequests.address;
			break;

			case IPCP_PRIMARY_DNS:
				wishedAddress = &fPeerRequests.primaryDNS;
			break;

			case IPCP_SECONDARY_DNS:
				wishedAddress = &fPeerRequests.secondaryDNS;
			break;
		}

		// now parse item
		switch (item->type) {
			case IPCP_ADDRESSES:
				// abandoned by the standard
			case IPCP_ADDRESS:
			case IPCP_PRIMARY_DNS:
			case IPCP_SECONDARY_DNS:
				if (item->length != 6) {
					// the packet is invalid
					gBufferModule->free(packet);
					NewState(PPP_INITIAL_STATE);
					ReportUpFailedEvent();
					return;
				}

				requestedAddress = (in_addr_t*) item->data;
				if (*wishedAddress == INADDR_ANY) {
					if (*requestedAddress == INADDR_ANY) {
						// we do not have an address for you
						gBufferModule->free(packet);
						NewState(PPP_INITIAL_STATE);
						ReportUpFailedEvent();
						return;
					}
				} else if (*requestedAddress != *wishedAddress) {
					// we do not want this address
					ip_item ipItem;
					ipItem.type = item->type;
					ipItem.length = 6;
					ipItem.address = *wishedAddress;
					nak.AddItem((ppp_configure_item*) &ipItem);
				}
			break;

//			case IPCP_COMPRESSION_PROTOCOL:
				// TODO: implement me!
//			break;

			default:
				reject.AddItem(item);
		}
	}

	// append additional values to the nak
	if (!request.ItemWithType(IPCP_ADDRESS) && fPeerRequests.address == INADDR_ANY) {
		// The peer did not provide us his address. Tell him to do so.
		ip_item ipItem;
		ipItem.type = IPCP_ADDRESS;
		ipItem.length = 6;
		ipItem.address = INADDR_ANY;
		nak.AddItem((ppp_configure_item*) &ipItem);
	}

	if (nak.CountItems() > 0) {
		RCRBadEvent(nak.ToNetBuffer(Interface().MRU()), NULL);
		gBufferModule->free(packet);
	} else if (reject.CountItems() > 0) {
		RCRBadEvent(NULL, reject.ToNetBuffer(Interface().MRU()));
		gBufferModule->free(packet);
	} else
		RCRGoodEvent(packet);
}


void
IPCP::RCRGoodEvent(net_buffer *packet)
{
	TRACE("IPCP: RCRGoodEvent() state=%d\n", State());

	switch (State()) {
		case PPP_INITIAL_STATE:
			NewState(PPP_ACK_SENT_STATE);
			InitializeRestartCount();
			SendConfigureRequest();
			SendConfigureAck(packet);
		break;

		case PPP_REQ_SENT_STATE:
			NewState(PPP_ACK_SENT_STATE);

		case PPP_ACK_SENT_STATE:
			SendConfigureAck(packet);
		break;

		case PPP_ACK_RCVD_STATE:
			NewState(PPP_OPENED_STATE);
			SendConfigureAck(packet);
			ReportUpEvent();
		break;

		case PPP_OPENED_STATE:
			NewState(PPP_ACK_SENT_STATE);
			SendConfigureRequest();
			SendConfigureAck(packet);
		break;

		default:
			gBufferModule->free(packet);
	}
}


void
IPCP::RCRBadEvent(net_buffer *nak, net_buffer *reject)
{
	TRACE("IPCP: RCRBadEvent() state=%d\n", State());

	uint16 lcpHdrRejectLength = 0;
	uint16 lcpHdrNakLength = 0;

	if (nak) {
		NetBufferHeaderReader<ppp_lcp_packet> nakBufferHeaderReader(nak);
		if (nakBufferHeaderReader.Status() != B_OK)
			return;
		ppp_lcp_packet &lcpNakPacket = nakBufferHeaderReader.Data();
		lcpHdrNakLength = lcpNakPacket.length;
	}


	if (reject) {
		NetBufferHeaderReader<ppp_lcp_packet> rejectBufferHeaderReader(reject);
		if (rejectBufferHeaderReader.Status() != B_OK)
			return;
		ppp_lcp_packet &lcpRejectPacket = rejectBufferHeaderReader.Data();
		lcpHdrRejectLength = lcpRejectPacket.length;
	}

	switch (State()) {
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			SendConfigureRequest();

		case PPP_ACK_SENT_STATE:
			if (State() == PPP_ACK_SENT_STATE)
				NewState(PPP_REQ_SENT_STATE);
					// OPENED_STATE might have set this already

		case PPP_INITIAL_STATE:
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
			if (nak && ntohs(lcpHdrNakLength) > 3)
				SendConfigureNak(nak);
			else if (reject && ntohs(lcpHdrRejectLength) > 3)
				SendConfigureNak(reject);
		return;
			// prevents the nak/reject from being m_freem()'d

		default:
			;
	}

	if (nak)
		gBufferModule->free(nak);
	if (reject)
		gBufferModule->free(reject);
}


void
IPCP::RCAEvent(net_buffer *packet)
{
	ERROR("IPCP: RCAEvent() state=%d\n", State());

	NetBufferHeaderReader<ppp_lcp_packet> bufferheader(packet);
	if (bufferheader.Status() != B_OK)
		return;
	ppp_lcp_packet &lcpHeader = bufferheader.Data();
	if (fRequestID != lcpHeader.id) {
		// this packet is not a reply to our request

		// TODO: log this event
		gBufferModule->free(packet);
		return;
	}

	// parse this ack
	KPPPConfigurePacket ack(packet);
	ppp_configure_item *item;
	in_addr_t *requestedAddress, *wishedAddress = NULL, *configuredAddress = NULL;
	for (int32 index = 0; index < ack.CountItems(); index++) {
		item = ack.ItemAt(index);
		if (!item)
			continue;

		// addresses have special handling to reduce code size
		switch (item->type) {
			case IPCP_ADDRESSES:
				// abandoned by the standard
			case IPCP_ADDRESS:
				wishedAddress = &fLocalRequests.address;
				configuredAddress = &fLocalConfiguration.address;
			break;

			case IPCP_PRIMARY_DNS:
				wishedAddress = &fLocalRequests.primaryDNS;
				configuredAddress = &fLocalConfiguration.primaryDNS;
			break;

			case IPCP_SECONDARY_DNS:
				wishedAddress = &fLocalRequests.secondaryDNS;
				configuredAddress = &fLocalConfiguration.secondaryDNS;
			break;
		}

		// now parse item
		switch (item->type) {
			case IPCP_ADDRESSES:
				// abandoned by the standard
			case IPCP_ADDRESS:
			case IPCP_PRIMARY_DNS:
			case IPCP_SECONDARY_DNS:
				requestedAddress = (in_addr_t*) item->data;
				if ((*wishedAddress == INADDR_ANY && *requestedAddress != INADDR_ANY)
						|| *wishedAddress == *requestedAddress)
					*configuredAddress = *requestedAddress;
			break;

//			case IPCP_COMPRESSION_PROTOCOL:
				// TODO: implement me
//			break;

			default:
				;
		}
	}

	// if address was not specified we should select the given one
	if (!ack.ItemWithType(IPCP_ADDRESS))
		fLocalConfiguration.address = fLocalRequests.address;


	switch (State()) {
		case PPP_INITIAL_STATE:
			IllegalEvent(PPP_RCA_EVENT);
		break;

		case PPP_REQ_SENT_STATE:
			NewState(PPP_ACK_RCVD_STATE);
			InitializeRestartCount();
		break;

		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
			SendConfigureRequest();
		break;

		case PPP_ACK_SENT_STATE:
			NewState(PPP_OPENED_STATE);
			InitializeRestartCount();
			ReportUpEvent();
		break;

		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			SendConfigureRequest();
		break;

		default:
			;
	}

	gBufferModule->free(packet);
}


void
IPCP::RCNEvent(net_buffer *packet)
{
	TRACE("IPCP: RCNEvent() state=%d\n", State());

	NetBufferHeaderReader<ppp_lcp_packet> bufferheader(packet);
	if (bufferheader.Status() != B_OK)
		return;
	ppp_lcp_packet &lcpHeader = bufferheader.Data();

	if (fRequestID != lcpHeader.id) {
		// this packet is not a reply to our request

		// TODO: log this event
		gBufferModule->free(packet);
		return;
	}

	// parse this nak/reject
	KPPPConfigurePacket nak_reject(packet);
	ppp_configure_item *item;
	in_addr_t *requestedAddress;
	if (nak_reject.Code() == PPP_CONFIGURE_NAK)
		for (int32 index = 0; index < nak_reject.CountItems(); index++) {
			item = nak_reject.ItemAt(index);
			if (!item)
				continue;

			switch (item->type) {
				case IPCP_ADDRESSES:
					// abandoned by the standard
				case IPCP_ADDRESS:
					if (item->length != 6)
						continue;

					requestedAddress = (in_addr_t*) item->data;
					if (fLocalRequests.address == INADDR_ANY
							&& *requestedAddress != INADDR_ANY)
						fLocalConfiguration.address = *requestedAddress;
							// this will be used in our next request
				break;

//				case IPCP_COMPRESSION_PROTOCOL:
					// TODO: implement me!
//				break;

				case IPCP_PRIMARY_DNS:
					if (item->length != 6)
						continue;

					requestedAddress = (in_addr_t*) item->data;
					if (fRequestPrimaryDNS
							&& fLocalRequests.primaryDNS == INADDR_ANY
							&& *requestedAddress != INADDR_ANY)
						fLocalConfiguration.primaryDNS = *requestedAddress;
							// this will be used in our next request
				break;

				case IPCP_SECONDARY_DNS:
					if (item->length != 6)
						continue;

					requestedAddress = (in_addr_t*) item->data;
					if (fRequestSecondaryDNS
							&& fLocalRequests.secondaryDNS == INADDR_ANY
							&& *requestedAddress != INADDR_ANY)
						fLocalConfiguration.secondaryDNS = *requestedAddress;
							// this will be used in our next request
				break;

				default:
					;
			}
		}
	else if (nak_reject.Code() == PPP_CONFIGURE_REJECT)
		for (int32 index = 0; index < nak_reject.CountItems(); index++) {
			item = nak_reject.ItemAt(index);
			if (!item)
				continue;

			switch (item->type) {
//				case IPCP_COMPRESSION_PROTOCOL:
					// TODO: implement me!
//				break;

				default:
					// DNS and addresses must be supported if we set them to auto
					gBufferModule->free(packet);
					NewState(PPP_INITIAL_STATE);
					ReportUpFailedEvent();
					return;
			}
		}

	switch (State()) {
		case PPP_INITIAL_STATE:
			IllegalEvent(PPP_RCN_EVENT);
		break;

		case PPP_REQ_SENT_STATE:
		case PPP_ACK_SENT_STATE:
			InitializeRestartCount();

		case PPP_ACK_RCVD_STATE:
		case PPP_OPENED_STATE:
			if (State() == PPP_ACK_RCVD_STATE || State() == PPP_OPENED_STATE)
				NewState(PPP_REQ_SENT_STATE);
			SendConfigureRequest();
		break;

		default:
			;
	}

	gBufferModule->free(packet);
}


void
IPCP::RTREvent(net_buffer *packet)
{
	TRACE("IPCP: RTREvent() state=%d\n", State());

	NetBufferHeaderReader<ppp_lcp_packet> bufferheader(packet);
	if (bufferheader.Status() != B_OK)
		return;
	ppp_lcp_packet &lcpHeader = bufferheader.Data();

	// we should not use the same ID as the peer
	if (fID == lcpHeader.id)
		fID -= 128;

	switch (State()) {
		case PPP_INITIAL_STATE:
			IllegalEvent(PPP_RTR_EVENT);
		break;

		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_REQ_SENT_STATE);

		case PPP_CLOSING_STATE:
		case PPP_REQ_SENT_STATE:
			SendTerminateAck(packet);
		return;
			// do not free packet

		case PPP_OPENED_STATE:
			NewState(PPP_CLOSING_STATE);
			ResetRestartCount();
			SendTerminateAck(packet);
		return;
			// do not free packet

		default:
			;
	}

	gBufferModule->free(packet);
}


void
IPCP::RTAEvent(net_buffer *packet)
{
	TRACE("IPCP: RTAEvent() state=%d\n", State());

	NetBufferHeaderReader<ppp_lcp_packet> bufferheader(packet);
	if (bufferheader.Status() != B_OK)
		return;
	ppp_lcp_packet &lcpHeader = bufferheader.Data();
	if (fTerminateID != lcpHeader.id) {
		// this packet is not a reply to our request

		// TODO: log this event
		gBufferModule->free(packet);
		return;
	}

	switch (State()) {
		case PPP_INITIAL_STATE:
			IllegalEvent(PPP_RTA_EVENT);
		break;

		case PPP_CLOSING_STATE:
			NewState(PPP_INITIAL_STATE);
			ReportDownEvent();
		break;

		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
		break;

		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			SendConfigureRequest();
		break;

		default:
			;
	}

	gBufferModule->free(packet);
}


void
IPCP::RUCEvent(net_buffer *packet)
{
	TRACE("IPCP: RUCEvent() state=%d\n", State());

	SendCodeReject(packet);
}


void
IPCP::RXJBadEvent(net_buffer *packet)
{
	TRACE("IPCP: RXJBadEvent() state=%d\n", State());

	switch (State()) {
		case PPP_INITIAL_STATE:
			IllegalEvent(PPP_RXJ_BAD_EVENT);
		break;

		case PPP_CLOSING_STATE:
			NewState(PPP_INITIAL_STATE);
			ReportDownEvent();
		break;

		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_INITIAL_STATE);
			ReportUpFailedEvent();
		break;

		case PPP_OPENED_STATE:
			NewState(PPP_CLOSING_STATE);
			InitializeRestartCount();
			SendTerminateRequest();
		break;

		default:
			;
	}

	gBufferModule->free(packet);
}


// actions
void
IPCP::IllegalEvent(ppp_event event)
{
	// TODO: update error statistics
	ERROR("IPCP: IllegalEvent(event=%d) state=%d\n", event, State());
}


void
IPCP::ReportUpFailedEvent()
{
	// reset configurations
	memset(&fLocalConfiguration, 0, sizeof(ipcp_configuration));
	memset(&fPeerConfiguration, 0, sizeof(ipcp_configuration));

	UpdateAddresses();

	UpFailedEvent();
}


void
IPCP::ReportUpEvent()
{
	UpdateAddresses();

	UpEvent();
}


void
IPCP::ReportDownEvent()
{
	// reset configurations
	memset(&fLocalConfiguration, 0, sizeof(ipcp_configuration));
	memset(&fPeerConfiguration, 0, sizeof(ipcp_configuration));

	// don't update address if connect on demand is enabled
	dprintf("ppp down, and leaving old address and rotues\n");
	// UpdateAddresses();

	DownEvent();
}


void
IPCP::InitializeRestartCount()
{
	fRequestCounter = fMaxRequest;
	fTerminateCounter = fMaxTerminate;
	fNakCounter = fMaxNak;
}


void
IPCP::ResetRestartCount()
{
	fRequestCounter = 0;
	fTerminateCounter = 0;
	fNakCounter = 0;
}


bool
IPCP::SendConfigureRequest()
{
	TRACE("IPCP: SendConfigureRequest() state=%d\n", State());

	--fRequestCounter;
	fNextTimeout = system_time() + kIPCPStateMachineTimeout;

	KPPPConfigurePacket request(PPP_CONFIGURE_REQUEST);
	request.SetID(NextID());
	fRequestID = request.ID();
	ip_item ipItem;
	ipItem.length = 6;

	// add address
	ipItem.type = IPCP_ADDRESS;
	if (fLocalRequests.address == INADDR_ANY)
		ipItem.address = (fLocalConfiguration.address);
	else
		ipItem.address =(fLocalRequests.address);
	request.AddItem((ppp_configure_item*) &ipItem);

	TRACE("IPCP: SCR: confaddr=%X; reqaddr=%X; addr=%X\n",
		fLocalConfiguration.address, fLocalRequests.address,
		((ip_item*)request.ItemAt(0))->address);

	// add primary DNS (if needed)
	if (fRequestPrimaryDNS && fLocalRequests.primaryDNS == INADDR_ANY) {
		ipItem.type = IPCP_PRIMARY_DNS;
		ipItem.address = fLocalConfiguration.primaryDNS;
			// at first this is 0.0.0.0, but a nak might have set it to a correct value
		request.AddItem((ppp_configure_item*) &ipItem);
	}

	// add secondary DNS (if needed)
	if (fRequestSecondaryDNS && fLocalRequests.primaryDNS == INADDR_ANY) {
		ipItem.type = IPCP_SECONDARY_DNS;
		ipItem.address = fLocalConfiguration.secondaryDNS;
			// at first this is 0.0.0.0, but a nak might have set it to a correct value
		request.AddItem((ppp_configure_item*) &ipItem);
	}

	// TODO: add VJC support

	return Send(request.ToNetBuffer(Interface().MRU())) == B_OK;
}


bool
IPCP::SendConfigureAck(net_buffer *packet)
{
	TRACE("IPCP: SendConfigureAck() state=%d\n", State());

	if (!packet)
		return false;

	NetBufferHeaderReader<ppp_lcp_packet> bufferheader(packet);
	if (bufferheader.Status() != B_OK)
		return false;
	ppp_lcp_packet &lcpheader = bufferheader.Data();
	lcpheader.code = PPP_CONFIGURE_ACK;

	bufferheader.Sync();

	KPPPConfigurePacket ack(packet);

	// verify items
	ppp_configure_item *item;
	in_addr_t *requestedAddress, *wishedAddress = NULL, *configuredAddress = NULL;
	for (int32 index = 0; index < ack.CountItems(); index++) {
		item = ack.ItemAt(index);
		if (!item)
			continue;

		// addresses have special handling to reduce code size
		switch (item->type) {
			case IPCP_ADDRESSES:
				// abandoned by the standard
			case IPCP_ADDRESS:
				wishedAddress = &fPeerRequests.address;
				configuredAddress = &fPeerConfiguration.address;
			break;

			case IPCP_PRIMARY_DNS:
				wishedAddress = &fPeerRequests.primaryDNS;
				configuredAddress = &fPeerConfiguration.primaryDNS;
			break;

			case IPCP_SECONDARY_DNS:
				wishedAddress = &fPeerRequests.secondaryDNS;
				configuredAddress = &fPeerConfiguration.secondaryDNS;
			break;
		}

		// now parse item
		switch (item->type) {
			case IPCP_ADDRESSES:
				// abandoned by the standard
			case IPCP_ADDRESS:
			case IPCP_PRIMARY_DNS:
			case IPCP_SECONDARY_DNS:
				requestedAddress = (in_addr_t*) item->data;
				if ((*wishedAddress == INADDR_ANY && *requestedAddress != INADDR_ANY)
						|| *wishedAddress == *requestedAddress)
					*configuredAddress = *requestedAddress;
			break;

//			case IPCP_COMPRESSION_PROTOCOL:
				// TODO: implement me!
//			break;

			default:
				;
		}
	}

	// if address was not specified we should select the given one
	if (!ack.ItemWithType(IPCP_ADDRESS))
		fPeerConfiguration.address = fPeerRequests.address;

	return Send(packet) == B_OK;
}


bool
IPCP::SendConfigureNak(net_buffer *packet)
{
	TRACE("IPCP: SendConfigureNak() state=%d\n", State());

	if (!packet)
		return false;

	NetBufferHeaderReader<ppp_lcp_packet> bufferheader(packet);
	if (bufferheader.Status() != B_OK)
		return false;

	ppp_lcp_packet &nak = bufferheader.Data();

	if (nak.code == PPP_CONFIGURE_NAK) {
		if (fNakCounter == 0) {
			// We sent enough naks. Let's try a reject.
			nak.code = PPP_CONFIGURE_REJECT;
		} else
			--fNakCounter;
	}

	bufferheader.Sync();

	return Send(packet) == B_OK;
}


bool
IPCP::SendTerminateRequest()
{
	TRACE("IPCP: SendTerminateRequest() state=%d\n", State());

	--fTerminateCounter;
	fNextTimeout = system_time() + kIPCPStateMachineTimeout;

	net_buffer *packet = gBufferModule->create(256);
	if (!packet)
		return false;

	ppp_lcp_packet *request;
	status_t status = gBufferModule->append_size(packet, 1492, (void **)(&request));
	if (status != B_OK)
		return false;

	request->code = PPP_TERMINATE_REQUEST;
	request->id = fTerminateID = NextID();
	request->length = htons(4);

	status = gBufferModule->trim(packet, 4);
	if (status != B_OK)
		return false;

	return Send(packet) == B_OK;
}


bool
IPCP::SendTerminateAck(net_buffer *request)
{
	TRACE("IPCP: SendTerminateAck() state=%d\n", State());

	net_buffer *reply = request;

	if (!reply) {
		reply = gBufferModule->create(256);
		ppp_lcp_packet *ack;
		status_t status = gBufferModule->append_size(reply, 1492, (void **)(&ack));
		if (status != B_OK) {
			gBufferModule->free(reply);
			return false;
		}
		ack->id = NextID();
		ack->code = PPP_TERMINATE_ACK;
		ack->length = htons(4);
		gBufferModule->trim(reply, 4);
	} else {
		NetBufferHeaderReader<ppp_lcp_packet> bufferHeader(reply);
		if (bufferHeader.Status() < B_OK)
			return false;
		ppp_lcp_packet &ack = bufferHeader.Data();
		ack.code = PPP_TERMINATE_ACK;
		ack.length = htons(4);
	}

	return Send(reply) == B_OK;
}


bool
IPCP::SendCodeReject(net_buffer *packet)
{
	TRACE("IPCP: SendCodeReject() state=%d\n", State());

	if (!packet)
		return false;

	NetBufferPrepend<ppp_lcp_packet> bufferHeader(packet);
	if (bufferHeader.Status() != B_OK)
		return false;

	ppp_lcp_packet &reject = bufferHeader.Data();

	reject.code = PPP_CODE_REJECT;
	reject.id = NextID();
	reject.length = htons(packet->size);

	bufferHeader.Sync();

	return Send(packet) == B_OK;
}
