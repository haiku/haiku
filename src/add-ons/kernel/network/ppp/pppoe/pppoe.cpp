//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include <KernelExport.h>
#include <driver_settings.h>
#include <core_funcs.h>
#include <net_module.h>

#include <ethernet_module.h>

#include <KPPPInterface.h>
#include <KPPPModule.h>
#include <LockerHelper.h>

#include "PPPoEDevice.h"
#include "DiscoveryPacket.h"


#define PPPoE_MODULE_NAME		NETWORK_MODULES_ROOT "ppp/pppoe"

struct core_module_info *core = NULL;
static struct ethernet_module_info *sEthernet;
static int32 sHostUniq = 0;
status_t std_ops(int32 op, ...);

static BLocker sLock;
static TemplateList<PPPoEDevice*> *sDevices;


uint32
NewHostUniq()
{
	return (uint32) atomic_add(&sHostUniq, 1);
}


void
add_device(PPPoEDevice *device)
{
#if DEBUG
	dprintf("PPPoE: add_device()\n");
#endif
	
	LockerHelper locker(sLock);
	sDevices->AddItem(device);
}


void
remove_device(PPPoEDevice *device)
{
#if DEBUG
	dprintf("PPPoE: remove_device()\n");
#endif
	
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
	
	LockerHelper locker(sLock);
	
	for(int32 index = 0; index < sDevices->CountItems(); index++) {
		device = sDevices->ItemAt(index);
		
		if(device && device->EthernetIfnet() == sourceIfnet) {
			if(header->ethernetHeader.ether_type == ETHERTYPE_PPPOE
					&& header->pppoeHeader.sessionID == device->SessionID()) {
#if DEBUG
				dprintf("PPPoE: session packet\n");
#endif
				device->Receive(packet);
				return;
			} else if(header->ethernetHeader.ether_type == ETHERTYPE_PPPOEDISC
					&& header->pppoeHeader.code != PADI
					&& header->pppoeHeader.code != PADR
					&& !device->IsDown()) {
#if DEBUG
				dprintf("PPPoE: discovery packet\n");
#endif
				DiscoveryPacket discovery(packet, ETHER_HDR_LEN);
				if(discovery.InitCheck() != B_OK) {
					dprintf("PPPoE: received corrupted discovery packet!\n");
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
	
	dprintf("PPPoE: No device found for packet from: %s\n", sourceIfnet->if_name);
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
	
#if DEBUG
	dprintf("PPPoE: add_to(): %s\n",
		success && device && device->InitCheck() == B_OK ? "OK" : "ERROR");
#endif
	
	return success && device && device->InitCheck() == B_OK;
}


static
status_t
control(uint32 op, void *data, size_t length)
{
	return B_ERROR;
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
			
			set_max_linkhdr(PPPoE_HEADER_SIZE + ETHER_HDR_LEN);
			
			sDevices = new TemplateList<PPPoEDevice*>;
			
			sEthernet->set_pppoe_receiver(pppoe_input);
			
#if DEBUG
			dprintf("PPPoE: Registered PPPoE receiver.\n");
#endif
		return B_OK;
		
		case B_MODULE_UNINIT:
			delete sDevices;
			sEthernet->unset_pppoe_receiver();
#if DEBUG
			dprintf("PPPoE: Unregistered PPPoE receiver.\n");
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
