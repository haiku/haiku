/* ethernet.c - ethernet devices interface module 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/dirent.h>
#include <sys/stat.h>

#include <Drivers.h>

#include "net_stack.h"

status_t 	std_ops(int32 op, ...);

struct net_layer_module_info nlmi;

static struct net_stack_module_info *g_stack = NULL;

// -------------------
status_t init(net_layer *me)
{
	printf("%s: initing layer\n", me->name);
	return B_OK;

}

status_t uninit(net_layer *me)
{
	printf("%s: uniniting layer\n", me->name);
	return B_OK;
}


status_t enable(net_layer *me, bool enable)
{
	return B_OK;
}


status_t input_buffer(net_layer *me, net_buffer *buffer)
{
//	uint16 *protocol;

	if (!buffer)
		return B_ERROR;
/*		
	if (g_stack->find_buffer_attribut(buffer, "ethernet:protocol", &protocol) != B_OK)
		return B_ERROR;

	if (protocol != 0x0806)
		// Not an ARP packet
		return B_ERROR;
*/
	// TODO!

	return B_OK;
}

status_t output_buffer(net_layer *me, net_buffer *buffer)
{
	if (!buffer)
		return B_ERROR;
		
	// TODO!
	return B_OK;
}

// #pragma mark -

// #pragma mark -

status_t std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT: {
			status_t status;
			net_layer *layer1, *layer2;
			
			printf("arp: B_MODULE_INIT\n");
			status = get_module(NET_STACK_MODULE_NAME, (module_info **) &g_stack);
			if (status != B_OK)
				return status;
				
			status = g_stack->register_layer("ethernet/arp", &nlmi, NULL, &layer1);
			if (status != B_OK)
				goto error1;
				
			status = g_stack->register_layer("arp/ethernet", &nlmi, NULL, &layer2);
			if (status != B_OK)
				goto error2;

			return B_OK;
error2:
			g_stack->unregister_layer(layer1);
error1:
			put_module(NET_STACK_MODULE_NAME);
			return status;
			}
			
		case B_MODULE_UNINIT:
			printf("arp: B_MODULE_UNINIT\n");
			put_module(NET_STACK_MODULE_NAME);
			break;
			
		default:
			return B_ERROR;
	}
	return B_OK;
}

struct net_layer_module_info nlmi = {
	{
		NET_LAYER_MODULES_ROOT "protocols/arp/v0",
		0,
		std_ops
	},
	
	init,
	uninit,
	enable,
	output_buffer,
	input_buffer
};

_EXPORT module_info *modules[] = {
	(module_info*) &nlmi,	// net_layer_module_info
	NULL
};
