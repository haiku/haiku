/* ethernet_framer.c - ethernet framing layer module 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <KernelExport.h>
#include <ByteOrder.h>

#include "net_stack.h"

#define	ETHER_ADDR_LEN	6	/* Ethernet address length		*/
#define ETHER_TYPE_LEN	2	/* Ethernet type field length		*/
#define ETHER_CRC_LEN	4	/* Ethernet CRC lenght			*/
#define ETHER_HDR_LEN	((ETHER_ADDR_LEN * 2) + ETHER_TYPE_LEN)
#define ETHER_MIN_LEN	64	/* Minimum frame length, CRC included	*/
#define ETHER_MAX_LEN	1518	/* Maximum frame length, CRC included	*/

#define	ETHERMTU	(ETHER_MAX_LEN - ETHER_HDR_LEN - ETHER_CRC_LEN)
#define	ETHERMIN	(ETHER_MIN_LEN - ETHER_HDR_LEN - ETHER_CRC_LEN)

typedef struct ethernet_addr { 
        uint8 byte[ETHER_ADDR_LEN]; 
} ethernet_addr; 


typedef struct ethernet2_header {
	ethernet_addr dst;
	ethernet_addr src;
	uint16 type;
} ethernet2_header;



status_t 	std_ops(int32 op, ...);

struct net_layer_module_info nlmi;

static struct net_stack_module_info *g_stack = NULL;

string_token ethernet_header_attr;
string_token ethernet_from_attr;
string_token ethernet_to_attr;
string_token ethernet_protocol_attr;


static void dump_ethernet(net_buffer *buffer)
{
	ethernet2_header hdr;
	const char *pname;

	g_stack->read_buffer(buffer, 0, &hdr, sizeof(hdr));
	
	switch(ntohs(hdr.type)) {
	case 0x0800: pname = "IPv4"; break;
	case 0x0806: pname = "ARP"; break;
	default: pname = "unknown"; break;
	};
	
	dprintf("========== Ethernet Frame ==========\n");	
	dprintf("Station:  %02x:%02x:%02x:%02x:%02x:%02x ----> %02x:%02x:%02x:%02x:%02x:%02x\n",
			hdr.src.byte[0], hdr.src.byte[1], hdr.src.byte[2], hdr.src.byte[3],	hdr.src.byte[4], hdr.src.byte[5], 
			hdr.dst.byte[0], hdr.dst.byte[1], hdr.dst.byte[2], hdr.dst.byte[3],	hdr.dst.byte[4], hdr.dst.byte[5]); 
	dprintf("Protocol: 0x%04x (%s)\n", ntohs(hdr.type), pname);
}


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


// ----------------------------------------------------
static status_t process_input(net_layer *me, net_buffer *buffer)
{
	if (!buffer)
		return B_ERROR;
		
	dprintf("%s: process_input:\n", me->name);
	
	dump_ethernet(buffer);
	
	g_stack->add_buffer_attribute(buffer, ethernet_header_attr, FROM_BUFFER, 0, 14);
	g_stack->add_buffer_attribute(buffer, ethernet_to_attr, FROM_BUFFER, 0, 6);
	g_stack->add_buffer_attribute(buffer, ethernet_from_attr, FROM_BUFFER, 6, 6);
	g_stack->add_buffer_attribute(buffer, ethernet_protocol_attr, FROM_BUFFER | NETWORK_ORDER, 12, 2);

	// strip ethernet header
	g_stack->remove_from_buffer(buffer, 0, 14);

	return g_stack->send_layers_up(me, buffer);
}


static status_t process_output(net_layer *me, net_buffer *buffer)
{
	if (!buffer)
		return B_ERROR;
		
	// TODO!
	dprintf("%s: process_output:\n", me->name);
	return B_ERROR;
}


// #pragma mark -

status_t std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT: {
			status_t status;
			
			dprintf("ethernet_frame: B_MODULE_INIT\n");
			status = get_module(NET_STACK_MODULE_NAME, (module_info **) &g_stack);
			if (status != B_OK)
				return status;
				
			ethernet_header_attr 	= g_stack->string_to_token("ethernet:header"); 
			ethernet_from_attr 		= g_stack->string_to_token("ethernet:from"); 
			ethernet_to_attr 		= g_stack->string_to_token("ethernet:to"); 
			ethernet_protocol_attr 	= g_stack->string_to_token("ethernet:protocol"); 
				
			return g_stack->register_layer("Ethernet framing", "frame/ethernet", 0, &nlmi, NULL, NULL);
		};
			
		case B_MODULE_UNINIT:
			dprintf("ethernet_frame: B_MODULE_UNINIT\n");
			put_module(NET_STACK_MODULE_NAME);
			break;
			
		default:
			return B_ERROR;
	}
	return B_OK;
}

struct net_layer_module_info nlmi = {
	{
		NET_MODULES_ROOT "framings/ethernet_frame",
		0,
		std_ops
	},
	
	NET_LAYER_API_VERSION,
	
	init,
	uninit,
	enable,
	NULL,	// no control()
	process_input,
	process_output
};

_EXPORT module_info *modules[] = {
	(module_info*) &nlmi,	// net_layer_module_info
	NULL
};
