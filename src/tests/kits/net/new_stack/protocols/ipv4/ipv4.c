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

status_t 	std_ops(int32 op, ...);

struct net_layer_module_info nlmi;

static struct net_stack_module_info *g_stack = NULL;

net_attribute_id ethernet_protocol_attr;

// -------------------
static status_t init(net_layer *me)
{
	printf("%s: initing layer\n", me->name);
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

	if (g_stack->find_buffer_attribute(buffer, ethernet_protocol_attr, NULL, (void **) &protocol, NULL) != B_OK)
		return B_ERROR;

	if (*protocol != htons(0x0800))	// IPv4 on Ethernet protocol value
		return B_ERROR;

	// TODO!
	dprintf("%s: packet %p is an IPv4 packet!\n", me->name, buffer);
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
