//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <core_funcs.h>
#include <KernelExport.h>
#include <driver_settings.h>

#include <ethernet_module.h>

#include <KPPPInterface.h>
#include <KPPPModule.h>
#include <LockerHelper.h>

#include "PPPoEDevice.h"
#include "DiscoveryPacket.h"


#ifdef _KERNEL_MODE
	#define spawn_thread spawn_kernel_thread
	#define printf dprintf
#else
	#include <cstdio>
#endif


#define PPPoE_MODULE_NAME		"network/ppp/pppoe"

struct core_module_info *core = NULL;
static struct ethernet_module_info *ethernet;
static int32 host_uniq = 0;
status_t std_ops(int32 op, ...);

static BLocker lock;
static List<PPPoEDevice*> *devices;


uint32
NewHostUniq()
{
	return (uint32) atomic_add(&host_uniq, 1);
}


void
add_device(PPPoEDevice *device)
{
#if DEBUG
	printf("PPPoE: add_device()\n");
#endif
	
	LockerHelper locker(lock);
	devices->AddItem(device);
}


void
remove_device(PPPoEDevice *device)
{
#if DEBUG
	printf("PPPoE: remove_device()\n");
#endif
	
	LockerHelper locker(lock);
	devices->RemoveItem(device);
}


static
void
pppoe_input(struct mbuf *packet)
{
	if(!packet)
		return;
	
#if DEBUG
//	dump_packet(packet);
#endif
	
	ifnet *sourceIfnet = packet->m_pkthdr.rcvif;
	complete_pppoe_header *header = mtod(packet, complete_pppoe_header*);
	PPPoEDevice *device;
	
	LockerHelper locker(lock);
	
	for(int32 index = 0; index < devices->CountItems(); index++) {
		device = devices->ItemAt(index);
		
		if(device && device->EthernetIfnet() == sourceIfnet) {
			if(header->ethernetHeader.ether_type == ETHERTYPE_PPPOE
					&& header->pppoeHeader.sessionID == device->SessionID()) {
#if DEBUG
				printf("PPPoE: session packet\n");
#endif
				device->Receive(packet);
				return;
			} else if(header->ethernetHeader.ether_type == ETHERTYPE_PPPOEDISC
					&& header->pppoeHeader.code != PADI
					&& header->pppoeHeader.code != PADR
					&& !device->IsDown()) {
#if DEBUG
				printf("PPPoE: discovery packet\n");
#endif
				DiscoveryPacket discovery(packet, ETHER_HDR_LEN);
				if(discovery.InitCheck() != B_OK) {
					printf("PPPoE: received corrupted discovery packet!\n");
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
	
	printf("PPPoE: No device found for packet from: %s\n", sourceIfnet->if_name);
	m_freem(packet);
}


static
bool
add_to(PPPInterface& mainInterface, PPPInterface *subInterface,
	driver_parameter *settings, ppp_module_key_type type)
{
	if(type != PPP_DEVICE_KEY_TYPE)
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
	
#if DEBUG
	printf("PPPoE: add_to(): %s\n",
		success && device && device->InitCheck() == B_OK ? "OK" : "ERROR");
#endif
	
	return success && device && device->InitCheck() == B_OK;
}


static ppp_module_info pppoe_module = {
	{
		PPPoE_MODULE_NAME,
		0,
		std_ops
	},
	NULL,
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
			
			if(get_module(NET_ETHERNET_MODULE_NAME, (module_info**)&ethernet) != B_OK) {
				put_module(NET_CORE_MODULE_NAME);
				return B_ERROR;
			}
			
			devices = new List<PPPoEDevice*>;
			
			ethernet->set_pppoe_receiver(pppoe_input);
			
#if DEBUG
			printf("PPPoE: Registered PPPoE receiver.\n");
#endif
		return B_OK;
		
		case B_MODULE_UNINIT:
			delete devices;
			ethernet->unset_pppoe_receiver();
#if DEBUG
			printf("PPPoE: Unregistered PPPoE receiver.\n");
#endif
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
