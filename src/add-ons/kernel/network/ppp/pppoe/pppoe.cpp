/*
 * Copyright 2003-2006, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <driver_settings.h>
#include <core_funcs.h>
#include <net_module.h>

#include <ethernet_module.h>

#include <KPPPInterface.h>
#include <KPPPModule.h>
#include <KPPPUtils.h>
#include <LockerHelper.h>

#include "PPPoEDevice.h"
#include "DiscoveryPacket.h"


typedef struct pppoe_query {
	ifnet *ethernetIfnet;
	uint32 hostUniq;
	thread_id receiver;
} pppoe_query;

#define PPPoE_MODULE_NAME		NETWORK_MODULES_ROOT "ppp/pppoe"

struct core_module_info *core = NULL;
static struct ethernet_module_info *sEthernet;
static int32 sHostUniq = 0;
status_t std_ops(int32 op, ...);

static BLocker sLock("PPPoEList");
static TemplateList<PPPoEDevice*> *sDevices;
static TemplateList<pppoe_query*> *sQueries;


static
void
SendQueryPacket(pppoe_query *query, DiscoveryPacket& discovery)
{
	char data[PPPoE_QUERY_REPORT_SIZE];
	uint32 position = sizeof(uint32);
	pppoe_tag *acName = discovery.TagWithType(AC_NAME);
	
	if(acName) {
		if(acName->length >= PPPoE_QUERY_REPORT_SIZE)
			return;
		
		memcpy(data + position, acName->data, acName->length);
		position += acName->length;
	}
	
	data[position++] = 0;
	
	pppoe_tag *tag;
	for(int32 index = 0; index < discovery.CountTags(); index++) {
		tag = discovery.TagAt(index);
		if(tag && tag->type == SERVICE_NAME) {
			if(position + tag->length >= PPPoE_QUERY_REPORT_SIZE)
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


ifnet*
FindPPPoEInterface(const char *name)
{
	if(!name)
		return NULL;
	
	ifnet *current = get_interfaces();
	for(; current; current = current->if_next) {
		if(current->if_type == IFT_ETHER && current->if_name
				&& !strcmp(current->if_name, name))
			return current;
	}
	
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
	
	LockerHelper locker(sLock);
	if(!sDevices->HasItem(device))
		sDevices->AddItem(device);
}


void
remove_device(PPPoEDevice *device)
{
	TRACE("PPPoE: remove_device()\n");
	
	LockerHelper locker(sLock);
	sDevices->RemoveItem(device);
}


static
void
pppoe_input(struct mbuf *packet)
{
	if(!packet)
		return;
	
	ifnet *sourceIfnet = packet->m_pkthdr.rcvif;
	complete_pppoe_header *header = mtod(packet, complete_pppoe_header*);
	PPPoEDevice *device;
	pppoe_query *query;
	
	LockerHelper locker(sLock);
	
	if(header->ethernetHeader.ether_type == ETHERTYPE_PPPOEDISC
			&& header->pppoeHeader.length <= PPPoE_QUERY_REPORT_SIZE) {
		for(int32 index = 0; index < sDevices->CountItems(); index++) {
			query = sQueries->ItemAt(index);
			
			if(query && query->ethernetIfnet == sourceIfnet) {
				if(header->pppoeHeader.code == PADO) {
					DiscoveryPacket discovery(packet, ETHER_HDR_LEN);
					if(discovery.InitCheck() != B_OK) {
						ERROR("PPPoE: received corrupted discovery packet!\n");
						m_freem(packet);
						return;
					}
					
					pppoe_tag *hostTag = discovery.TagWithType(HOST_UNIQ);
					if(hostTag && hostTag->length == 4
							&& *((uint32*)hostTag->data) == query->hostUniq) {
						SendQueryPacket(query, discovery);
						m_freem(packet);
						return;
					}
				}
			}
		}
	}
	
	for(int32 index = 0; index < sDevices->CountItems(); index++) {
		device = sDevices->ItemAt(index);
		
		if(device && device->EthernetIfnet() == sourceIfnet) {
			if(header->ethernetHeader.ether_type == ETHERTYPE_PPPOE
					&& header->pppoeHeader.sessionID == device->SessionID()) {
				TRACE("PPPoE: session packet\n");
				device->Receive(packet);
				return;
			} else if(header->ethernetHeader.ether_type == ETHERTYPE_PPPOEDISC
					&& header->pppoeHeader.code != PADI
					&& header->pppoeHeader.code != PADR
					&& !device->IsDown()) {
				TRACE("PPPoE: discovery packet\n");
				DiscoveryPacket discovery(packet, ETHER_HDR_LEN);
				if(discovery.InitCheck() != B_OK) {
					ERROR("PPPoE: received corrupted discovery packet!\n");
					m_freem(packet);
					return;
				}
				
				pppoe_tag *tag = discovery.TagWithType(HOST_UNIQ);
				if(header->pppoeHeader.code == PADT || (tag && tag->length == 4
						&& *((uint32*)tag->data) == device->HostUniq())) {
					device->Receive(packet);
					return;
				}
			}
		}
	}
	
	ERROR("PPPoE: No device found for packet from: %s\n", sourceIfnet->if_name);
	m_freem(packet);
}


static
bool
add_to(KPPPInterface& mainInterface, KPPPInterface *subInterface,
	driver_parameter *settings, ppp_module_key_type type)
{
	if(mainInterface.Mode() != PPP_CLIENT_MODE || type != PPP_DEVICE_KEY_TYPE)
		return B_ERROR;
	
	PPPoEDevice *device;
	bool success;
	if(subInterface) {
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
	switch(op) {
		case PPPoE_GET_INTERFACES: {
			int32 position = 0, count = 0;
			char *names = (char*) data;
			
			ifnet *current = get_interfaces();
			for(; current; current = current->if_next) {
				if(current->if_type == IFT_ETHER && current->if_name) {
					if(position + strlen(current->if_name) + 1 > length)
						return B_NO_MEMORY;
					
					strcpy(names + position, current->if_name);
					position += strlen(current->if_name);
					names[position++] = 0;
					++count;
				}
			}
			
			return count;
		}
		
		case PPPoE_QUERY_SERVICES: {
			// XXX: as all modules are loaded on-demand we must wait for the results
			
			if(!data || length != sizeof(pppoe_query_request))
				return B_ERROR;
			
			pppoe_query_request *request = (pppoe_query_request*) data;
			
			pppoe_query query;
			query.ethernetIfnet = FindPPPoEInterface(request->interfaceName);
			if(!query.ethernetIfnet)
				return B_BAD_VALUE;
			
			query.hostUniq = NewHostUniq();
			query.receiver = request->receiver;
			
			sLock.Lock();
			sQueries->AddItem(&query);
			sLock.Unlock();
			
			snooze(2000000);
				// wait two seconds for results
			
			sLock.Lock();
			sQueries->RemoveItem(&query);
			sLock.Unlock();
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
			if(get_module(NET_CORE_MODULE_NAME, (module_info**)&core) != B_OK)
				return B_ERROR;
			
			if(get_module(NET_ETHERNET_MODULE_NAME,
					(module_info**) &sEthernet) != B_OK) {
				put_module(NET_CORE_MODULE_NAME);
				return B_ERROR;
			}
			
			set_max_linkhdr(2 + PPPoE_HEADER_SIZE + ETHER_HDR_LEN);
				// 2 bytes for PPP header
			sDevices = new TemplateList<PPPoEDevice*>;
			sQueries = new TemplateList<pppoe_query*>;
			sEthernet->set_pppoe_receiver(pppoe_input);
			
			TRACE("PPPoE: Registered PPPoE receiver.\n");
		return B_OK;
		
		case B_MODULE_UNINIT:
			delete sDevices;
			sEthernet->unset_pppoe_receiver();
			TRACE("PPPoE: Unregistered PPPoE receiver.\n");
			put_module(NET_CORE_MODULE_NAME);
			put_module(NET_ETHERNET_MODULE_NAME);
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
