/*
 * Copyright 2003-2006, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <driver_settings.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <ByteOrder.h>

#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if.h>
#include <sys/sockio.h>

#include <net_buffer.h>
#include <net_datalink.h> // for get_interface and control
#include <net_stack.h>
#include <NetBufferUtilities.h>

#include <KPPPInterface.h>
#include <KPPPModule.h>
#include <KPPPUtils.h>

#include "PPPoEDevice.h"
#include "DiscoveryPacket.h"


typedef struct pppoe_query {
	net_device *ethernetIfnet;
	uint32 hostUniq;
	thread_id receiver;
} pppoe_query;

#define PPPoE_MODULE_NAME	"network/ppp/pppoe"

net_stack_module_info *gStackModule = NULL;
net_buffer_module_info *gBufferModule = NULL;
net_datalink_module_info *sDatalinkModule = NULL;

static int32 sHostUniq = 0;
status_t std_ops(int32 op, ...);

// static mutex sLock("PPPoEList");

static TemplateList<PPPoEDevice*> *sDevices;
static TemplateList<pppoe_query*> *sQueries;


static
void
SendQueryPacket(pppoe_query *query, DiscoveryPacket& discovery)
{
	char data[PPPoE_QUERY_REPORT_SIZE];
	uint32 position = sizeof(uint32);
	pppoe_tag *acName = discovery.TagWithType(AC_NAME);

	if (acName) {
		if (acName->length >= PPPoE_QUERY_REPORT_SIZE)
			return;

		memcpy(data + position, acName->data, acName->length);
		position += acName->length;
	}

	data[position++] = 0;

	pppoe_tag *tag;
	for (int32 index = 0; index < discovery.CountTags(); index++) {
		tag = discovery.TagAt(index);
		if (tag && tag->type == SERVICE_NAME) {
			if (position + tag->length >= PPPoE_QUERY_REPORT_SIZE)
				return;

			memcpy(data + position, tag->data, tag->length);
			position += tag->length;
			data[position++] = 0;
		}
	}

	memcpy(data, &position, sizeof(uint32));
		// add the total length

	send_data_with_timeout(query->receiver, PPPoE_QUERY_REPORT, data,
		PPPoE_QUERY_REPORT_SIZE, 700000);
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


net_device*
FindPPPoEInterface(const char *name)
{
	if (!name)
		return NULL;

	net_interface* ethernetInterfaceOfPPPOE = NULL;
	net_domain* domain = NULL;

	domain = gStackModule->get_domain(AF_INET);
	ethernetInterfaceOfPPPOE = get_interface_by_name(domain, name);

	if (ethernetInterfaceOfPPPOE == NULL) {
		TRACE("get_interface_by_name can not find eth\n");
		return NULL;
	}

	if (ethernetInterfaceOfPPPOE->device)
		return ethernetInterfaceOfPPPOE->device;

	return NULL;
}


uint32
NewHostUniq()
{
	return (uint32) atomic_add(&sHostUniq, 1);
}


void
add_device(PPPoEDevice *device)
{
	TRACE("PPPoE: add_device()\n");

	// MutexLocker locker(sLock);
	if (!sDevices->HasItem(device))
		sDevices->AddItem(device);
}


void
remove_device(PPPoEDevice *device)
{
	TRACE("PPPoE: remove_device()\n");

	// MutexLocker locker(sLock);
	sDevices->RemoveItem(device);
}


status_t
pppoe_input(void *cookie, net_device *_device, net_buffer *packet)
{
	if (!packet)
		return B_ERROR;

	NetBufferHeaderReader<pppoe_header> bufferheader(packet);
	if (bufferheader.Status() != B_OK)
		return B_ERROR;

	pppoe_header &header = bufferheader.Data();

	// remove the following lines when pppoe server is enabled
	if (header.code == PADI) {
		TRACE("PADI packet received, ignoreing!\n");
		gBufferModule->free(packet);
		return B_OK;
	}

	if (header.code == PADO || header.code == PADR || header.code == PADS || header.code == PADT)
	{
		uint8 peerEtherAddr[ETHER_ADDRESS_LENGTH];
		struct sockaddr_dl& source = *(struct sockaddr_dl*)packet->source;
		memcpy(peerEtherAddr, source.sdl_data, ETHER_ADDRESS_LENGTH);
		const char *str = header.code == PADI ? "PADI" :
			header.code == PADO ? "PADO" :
			header.code == PADR ? "PADR" :
			header.code == PADS ? "PADS" :
			"PADT" ;

		dprintf("%s from:%02x:%02x:%02x:%02x:%02x:%02x code:%02x\n", str,
			peerEtherAddr[0], peerEtherAddr[1], peerEtherAddr[2],
			peerEtherAddr[3], peerEtherAddr[4], peerEtherAddr[5],
			header.code);
	}

	PPPoEDevice *device;
	pppoe_query *query;

	sockaddr_dl& linkAddress = *(sockaddr_dl*)packet->source;
	int32 specificType = B_NET_FRAME_TYPE(linkAddress.sdl_type,
				ntohs(linkAddress.sdl_e_type));

	// MutexLocker locker(sLock);

	if (specificType == B_NET_FRAME_TYPE_PPPOE_DISCOVERY
			&& ntohs(header.length) <= PPPoE_QUERY_REPORT_SIZE) {
		for (int32 index = 0; index < sDevices->CountItems(); index++) {
			query = sQueries->ItemAt(index);

			if (query) {// && query->ethernetIfnet == sourceIfnet) {
				if (header.code == PADO) {
					DiscoveryPacket discovery(packet, ETHER_HDR_LEN);
					if (discovery.InitCheck() != B_OK) {
						ERROR("PPPoE: received corrupted discovery packet!\n");
						// gBufferModule->free(packet);
						return B_ERROR;
					}

					pppoe_tag *hostTag = discovery.TagWithType(HOST_UNIQ);
					if (hostTag && hostTag->length == 4
							&& *((uint32*)hostTag->data) == query->hostUniq) {
						SendQueryPacket(query, discovery);
						// gBufferModule->free(packet);
						return B_ERROR;
					}
				}
			}
		}
	}

	TRACE("in pppoed processing sDevices->CountItems(): %" B_PRId32 "\n",
		sDevices->CountItems());

	for (int32 index = 0; index < sDevices->CountItems(); index++) {
		device = sDevices->ItemAt(index);

		TRACE("device->SessionID() %d, header.sessionID: %d\n", device->SessionID(),
						header.sessionID);

		if (device) { // && device->EthernetIfnet() == sourceIfnet) {
			if (specificType == B_NET_FRAME_TYPE_PPPOE
					&& header.sessionID == device->SessionID()) {
				TRACE("PPPoE: session packet\n");
				device->Receive(packet);
				return B_OK;
			}

			if (specificType == B_NET_FRAME_TYPE_PPPOE_DISCOVERY
					&& header.code != PADI
					&& header.code != PADR
					&& !device->IsDown()) {
				TRACE("PPPoE: discovery packet\n");

				DiscoveryPacket discovery(packet, ETHER_HDR_LEN);
				if (discovery.InitCheck() != B_OK) {
					ERROR("PPPoE: received corrupted discovery packet!\n");
					gBufferModule->free(packet);
					return B_OK;
				}

				pppoe_tag *tag = discovery.TagWithType(HOST_UNIQ);
				if (header.code == PADT || (tag && tag->length == 4
					&& *((uint32*)tag->data) == device->HostUniq())) {
					device->Receive(packet);
					return B_OK;
				}
			}
		}
	}

	ERROR("PPPoE: No device found for packet from: %s\n", "ethernet");
	// gBufferModule->free(packet);
	return B_ERROR;
}


static
bool
add_to(KPPPInterface& mainInterface, KPPPInterface *subInterface,
	driver_parameter *settings, ppp_module_key_type type)
{
	if (mainInterface.Mode() != PPP_CLIENT_MODE || type != PPP_DEVICE_KEY_TYPE)
		return B_ERROR;

	PPPoEDevice *device;
	bool success;
	if (subInterface) {
		device = new PPPoEDevice(*subInterface, settings);
		success = subInterface->SetDevice(device);
	} else {
		device = new PPPoEDevice(mainInterface, settings);
		success = mainInterface.SetDevice(device);
	}

	TRACE("PPPoE: add_to(): %s\n",
		success && device && device->InitCheck() == B_OK ? "OK" : "ERROR");

	return success && device && device->InitCheck() == B_OK;
}


static
status_t
control(uint32 op, void *data, size_t length)
{
	switch (op) {
		case PPPoE_GET_INTERFACES: {
			int32 position = 0, count = 0;
			char *names = (char*) data;

			net_device *current = NULL; // get_interfaces();
			for (; current; current = NULL) {
				if (current->type == IFT_ETHER && current->name) {
					if (position + strlen(current->name) + 1 > length)
						return B_NO_MEMORY;

					strcpy(names + position, current->name);
					position += strlen(current->name);
					names[position++] = 0;
					++count;
				}
			}

			return count;
		}

		case PPPoE_QUERY_SERVICES: {
			// XXX: as all modules are loaded on-demand we must wait for the results

			if (!data || length != sizeof(pppoe_query_request))
				return B_ERROR;

			pppoe_query_request *request = (pppoe_query_request*) data;

			pppoe_query query;
			query.ethernetIfnet = FindPPPoEInterface(request->interfaceName);
			if (!query.ethernetIfnet)
				return B_BAD_VALUE;

			query.hostUniq = NewHostUniq();
			query.receiver = request->receiver;

			{
				// MutexLocker tlocker(sLock);
				TRACE("add query\n");
				sQueries->AddItem(&query);
			}

			snooze(2000000);
				// wait two seconds for results
			{
				// MutexLocker tlocker(sLock);
				TRACE("remove query\n");
				sQueries->RemoveItem(&query);
			}
		} break;

		default:
			return B_ERROR;
	}

	return B_OK;
}


static ppp_module_info pppoe_module = {
	{
		PPPoE_MODULE_NAME,
		0,
		std_ops
	},
	control,
	add_to
};


_EXPORT
status_t
std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			if (get_module(NET_STACK_MODULE_NAME,
				(module_info**)&gStackModule) != B_OK)
				return B_ERROR;

			if (get_module(NET_DATALINK_MODULE_NAME,
				(module_info **)&sDatalinkModule) != B_OK) {
				put_module(NET_STACK_MODULE_NAME);
				return B_ERROR;
			}

			if (get_module(NET_BUFFER_MODULE_NAME,
				(module_info **)&gBufferModule) != B_OK) {
				put_module(NET_DATALINK_MODULE_NAME);
				put_module(NET_STACK_MODULE_NAME);
				return B_ERROR;
			}

			// set_max_linkhdr(2 + PPPoE_HEADER_SIZE + ETHER_HDR_LEN);
				// 2 bytes for PPP header
			sDevices = new TemplateList<PPPoEDevice*>;
			sQueries = new TemplateList<pppoe_query*>;

			TRACE("PPPoE: Registered PPPoE receiver.\n");
			return B_OK;

		case B_MODULE_UNINIT:
			delete sQueries;
			delete sDevices;
			TRACE("PPPoE: Unregistered PPPoE receiver.\n");
			put_module(NET_BUFFER_MODULE_NAME);
			put_module(NET_DATALINK_MODULE_NAME);
			put_module(NET_STACK_MODULE_NAME);
			break;

		default:
			return B_ERROR;
	}

	return B_OK;
}


_EXPORT
module_info *modules[] = {
	(module_info*) &pppoe_module,
	NULL
};
