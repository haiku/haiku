/* ipv4.c - ipv4 protocol module 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/dirent.h>
#include <sys/stat.h>

#include <KernelExport.h>
#include <ByteOrder.h>

#include "net_stack.h"

typedef uint32 ipv4_addr; 

typedef struct ipv4_header {
	uint8 	version_header_length;
	uint8 	tos;
	uint16 	total_length;
	uint16 	identification;
	uint16 	flags_frag_offset;
	uint8 	ttl;
	uint8 	protocol;
	uint16 	header_checksum;
	ipv4_addr src;
	ipv4_addr dst;
} _PACKED ipv4_header;

#define IPV4_FLAG_MORE_FRAGS   0x2000
#define IPV4_FLAG_MAY_NOT_FRAG 0x4000
#define IPV4_FRAG_OFFSET_MASK  0x1fff



status_t 	std_ops(int32 op, ...);

struct net_layer_module_info nlmi;

static struct net_stack_module_info *g_stack = NULL;
string_token ethernet_protocol_attr;
static uint32 g_next_ipv4_id = 0;

static void dump_ipv4_addr(ipv4_addr *addr)
{
	uint8 *bytes = (uint8 *) addr;

	// NOTE: addr should point to a Network Byte Order'ed (aka Big Endian) IPv4 address
	
	dprintf("%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
}
	
static void dump_ipv4_header(net_buffer *buffer)
{
	ipv4_header ip;
	const char *pname;

	g_stack->read_buffer(buffer, 0, &ip, sizeof(ip)); 

	switch(ip.protocol) {
	case 1: 	pname = "ICMP"; break;
	case 6: 	pname = "TCP"; break;
	case 17: 	pname = "UDP"; break;
	default: 	pname = "unknown"; break;
	};
		
	dprintf("========== Internet Protocol (IPv4) =============\n");

	dprintf("Station: ");
	dump_ipv4_addr(&ip.src);
	dprintf(" ----> ");
	dump_ipv4_addr(&ip.dst);
	dprintf("Protocol: %d (%s)\n", ip.protocol, pname);
	dprintf("Version:  %d\n", ip.version_header_length >> 4);
	dprintf("Header Length (32 bit words): %d (%d bytes)\n",
		ip.version_header_length & 0x0f,
		4 * (ip.version_header_length & 0x0f));
	// TODO: dprintf("Precedence: \n");
	dprintf("Total length: %d\n", ntohs(ip.total_length));
	dprintf("Identification: %d\n", ntohs(ip.identification));
	// TODO: dprintf("Fragmentation allowed, Last fragment\n");
	dprintf("Fragment Offset: %d\n", ntohs(ip.flags_frag_offset) & 0x1fff);
	dprintf("Time to Live: %d second(s)\n", ip.ttl);
	dprintf("Checksum: 0x%X (%s)\n", ntohs(ip.header_checksum), "Valid");	// TODO: check the checksum!
}

// #pragma mark -


// -------------------
static status_t init(net_layer *me)
{
	printf("%s: initing layer\n", me->name);
	g_next_ipv4_id = system_time();
	return B_OK;

}

static status_t uninit(net_layer *me)
{
	printf("%s: uniniting layer\n", me->name);
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

	if (g_stack->find_buffer_attribute(buffer, ethernet_protocol_attr, 0, NULL, &protocol, NULL) != B_OK)
		return B_ERROR;

	if (*protocol != htons(0x0800))	// IPv4 on Ethernet protocol value
		return B_ERROR;

	// TODO!
	dump_ipv4_header(buffer);
	g_stack->delete_buffer(buffer, false);

	return B_OK;
}

static status_t process_output(net_layer *me, net_buffer *buffer)
{
	if (!buffer)
		return B_ERROR;
		
	// TODO!
	return B_ERROR;
}

// #pragma mark -

status_t std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT: {
			status_t status;
			net_layer *layer;
			
			printf("ipv4: B_MODULE_INIT\n");
			status = get_module(NET_STACK_MODULE_NAME, (module_info **) &g_stack);
			if (status != B_OK)
				return status;
				
			ethernet_protocol_attr = g_stack->string_to_token("ethernet:protocol");
				
			status = g_stack->register_layer("IPv4", "ethernet/ipv4", 2, &nlmi, NULL, &layer);
			if (status != B_OK)
				goto error;
				
			return B_OK;

error:
			g_stack->unregister_layer(layer);
			put_module(NET_STACK_MODULE_NAME);
			return status;
			}
			
		case B_MODULE_UNINIT:
			printf("ipv4: B_MODULE_UNINIT\n");
			put_module(NET_STACK_MODULE_NAME);
			break;
			
		default:
			return B_ERROR;
	}
	return B_OK;
}

struct net_layer_module_info nlmi = {
	{
		NET_MODULES_ROOT "protocols/ipv4/v0",
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
