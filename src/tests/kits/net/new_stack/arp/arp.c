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
#include "net_layer.h"

status_t 	std_ops(int32 op, ...);

struct net_layer_module_info nlmi;

static struct net_stack_module_info *g_stack = NULL;

// -------------------
status_t init(void * params)
{
	status_t status;
	net_layer *layer1, *layer2;

	layer2 = NULL;
	status = B_NO_MEMORY;

	layer1 = malloc(sizeof(*layer1));
	if (!layer1)
		goto error;

	layer1->name   = "ethernet/arp";
	layer1->module = &nlmi;
	
	layer2 = malloc(sizeof(*layer2));
	if (!layer2)
		goto error;

	layer2->name   = "arp/ethernet";
	layer2->module = &nlmi;
			
	status = g_stack->register_layer(layer1);
	return g_stack->register_layer(layer2);

error:;
	if (layer1)
		free(layer1);
	if (layer2)
		free(layer2);
	return status;
}

status_t uninit(net_layer *me)
{
	printf("%s: uniniting layer\n", me->name);
	
	free(me);
	return B_OK;
}


status_t enable(net_layer *me, bool enable)
{
	return B_OK;
}


status_t input_buffer(net_layer *me, net_buffer *buffer)
{
	if (!buffer)
		return B_ERROR;
		
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
		case B_MODULE_INIT:
			printf("arp: B_MODULE_INIT\n");
			return get_module(NET_STACK_MODULE_NAME, (module_info **) &g_stack);
			
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
		NET_LAYER_MODULE_ROOT "protocols/arp/v0",
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
