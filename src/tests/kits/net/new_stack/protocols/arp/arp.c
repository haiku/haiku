/* ethernet.c - ethernet devices interface module 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>
#include <ByteOrder.h>

#include "net_stack.h"

status_t 	std_ops(int32 op, ...);

struct net_layer_module_info nlmi;

static struct net_stack_module_info *g_stack = NULL;

net_attribute_id ethernet_protocol_attr;

typedef struct ethernet_addr { 
        uint8 byte[6]; 
} ethernet_addr; 

typedef struct ipv4_addr { 
        uint8 byte[4]; 
} ipv4_addr; 

struct	arp_header {
	uint16 hardware_type;	/* format of hardware address */
	uint16 protocol_type;	/* format of protocol address */
	uint8  hardware_size;	/* length of hardware address */
	uint8  protocol_size;	/* length of protocol address */
	uint16 op;	/* one of: */

	// for ethernet/ipv4 arp packets
		
	ethernet_addr  	sender_hardware_address;	/* sender hardware address */
	ipv4_addr  		sender_protocol_address;	/* sender protocol address */
	ethernet_addr 	target_hardware_address;	/* target hardware address */
	ipv4_addr  		target_protocol_address;	/* target protocol address */
};

enum {
	ARP_HARDWARE_TYPE_ETHERNET = 1,	 /* ethernet hardware format */
	ARP_HARDWARE_TYPE_IEEE802  = 6,	 /* IEEE 802 hardware format */
	ARP_HARDWARE_TYPE_FRAMEREALY = 15
};

enum {
	ARP_OP_REQUEST = 1,  /* request to resolve address */
	ARP_OP_REPLY,
	ARP_OP_RARP_REQUEST,
	ARP_OP_RARP_REPLY
};

static void dump_arp(net_buffer *buffer)
{
	struct arp_header arp;
	const char *pname;
	ethernet_addr *ea;
	ipv4_addr *ia;

	g_stack->read_buffer(buffer, 0, &arp, sizeof(arp)); 

	switch(ntohs(arp.protocol_type)) {
	case 0x0800: pname = "IPv4"; break;
	case 0x0806: pname = "ARP"; break;
	default: pname = "unknown"; break;
	};
		
	dprintf("========== Address Resolution Protocol ==========\n");
	dprintf("Hardware :  %s\n",
		ntohs(arp.hardware_type) == ARP_HARDWARE_TYPE_ETHERNET ? "ethernet" : "unknown");
	dprintf("Protocol :  0x%04x (%s)\n", ntohs(arp.protocol_type), pname);
	dprintf("Operation : ");
	switch(ntohs(arp.op)) {
		case ARP_OP_REPLY:
			dprintf("ARP Reply\n");
			break;
		case ARP_OP_REQUEST:
			dprintf("ARP Request\n");
			break;
		default:
			dprintf("Who knows? 0x%04x\n", ntohs(arp.op));
	};
	dprintf("Hardware address length: %d\n", arp.hardware_size);
	dprintf("Protocol address length: %d\n", arp.protocol_size);

	ea = &arp.sender_hardware_address;
	dprintf("Sender Hardware Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			ea->byte[0], ea->byte[1], ea->byte[2], ea->byte[3],	ea->byte[4], ea->byte[5]);
	ia = &arp.sender_protocol_address;
	dprintf("Sender Protocol Address: %d.%d.%d.%d\n",
			ia->byte[0], ia->byte[1], ia->byte[2], ia->byte[3]); 

	ea = &arp.target_hardware_address;
	dprintf("Target Hardware Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			ea->byte[0], ea->byte[1], ea->byte[2], ea->byte[3],	ea->byte[4], ea->byte[5]);
	ia = &arp.target_protocol_address;
	dprintf("Target Protocol Address: %d.%d.%d.%d\n",
			ia->byte[0], ia->byte[1], ia->byte[2], ia->byte[3]); 
}


// #pragma mark -


// -------------------
static status_t init(net_layer *me)
{
	dprintf("%s: initing layer\n", me->name);
	return B_OK;

}

static status_t uninit(net_layer *me)
{
	dprintf("%s: uniniting layer\n", me->name);
	return B_OK;
}


static status_t enable(net_layer *me, bool enable)
{
	return B_OK;
}


static status_t process_input(net_layer *me, net_buffer *buffer)
{
	uint16 *protocol;

	if (!buffer)
		return B_ERROR;
		
	if (g_stack->find_buffer_attribute(buffer, ethernet_protocol_attr, NULL, (void **) &protocol, NULL) != B_OK)
		return B_ERROR;

	if (*protocol != htons(0x0806))	// ARP on Ethernet protocol value
		// Not an ARP packet
		return B_ERROR;

	// TODO!
	// dprintf("%s: packet %p is an ARP packet!\n", me->name, buffer);
	dump_arp(buffer);
	g_stack->delete_buffer(buffer, false);

	return B_OK;
}

static status_t process_output(net_layer *me, net_buffer *buffer)
{
	if (!buffer)
		return B_ERROR;
		
	// TODO!
	return B_OK;
}

// #pragma mark -

status_t std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT: {
			status_t status;
			net_layer *layer;
			
			dprintf("arp: B_MODULE_INIT\n");
			status = get_module(NET_STACK_MODULE_NAME, (module_info **) &g_stack);
			if (status != B_OK)
				return status;
				
			ethernet_protocol_attr = g_stack->string_to_token("ethernet:protocol");
				
			status = g_stack->register_layer("ARP", "ethernet/arp", 30, &nlmi, NULL, &layer);
			if (status != B_OK)
				goto error;
				
			return B_OK;

error:
			g_stack->unregister_layer(layer);
			put_module(NET_STACK_MODULE_NAME);
			return status;
			}
			
		case B_MODULE_UNINIT:
			dprintf("arp: B_MODULE_UNINIT\n");
			put_module(NET_STACK_MODULE_NAME);
			break;
			
		default:
			return B_ERROR;
	}
	return B_OK;
}

struct net_layer_module_info nlmi = {
	{
		NET_MODULES_ROOT "protocols/arp/v0",
		0,
		std_ops
	},
	
	NET_LAYER_API_VERSION,
	
	init,
	uninit,
	enable,
	NULL,
	process_input,
	process_output
};

_EXPORT module_info *modules[] = {
	(module_info*) &nlmi,	// net_layer_module_info
	NULL
};
