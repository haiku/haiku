/* loopback.c - loopback interface module 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <KernelExport.h>

#include "net_stack.h"

status_t 	std_ops(int32 op, ...);

struct net_layer_module_info nlmi;

static struct net_stack_module_info *g_stack = NULL;


// -------------------
status_t init(net_layer *me)
{
	dprintf("%s: initing layer\n", me->name);
	return B_OK;
}

status_t uninit(net_layer *me)
{
	dprintf("loopback: uniniting layer\n");
	return B_OK;
}


status_t enable(net_layer *me, bool enable)
{
	return B_OK;
}


status_t process_output(net_layer *me, net_buffer *buffer)
{
	if (!buffer)
		return B_ERROR;
	
	// Here the magical loopback effect ;-)
	return g_stack->send_up(me, buffer);
}

// #pragma mark -

status_t std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT: {
			status_t status;
			
			dprintf("loopback: B_MODULE_INIT\n");
			status = get_module(NET_STACK_MODULE_NAME, (module_info **) &g_stack);
			if (status != B_OK)
				return status;
			
			return g_stack->register_layer("loopback", "loopback/frame", 0, &nlmi, NULL, NULL);
		}
			
		case B_MODULE_UNINIT:
			dprintf("loopback: B_MODULE_UNINIT\n");
			put_module(NET_STACK_MODULE_NAME);
			break;
			
		default:
			return B_ERROR;
	}
	return B_OK;
}

struct net_layer_module_info nlmi = {
	{
		NET_MODULES_ROOT "interfaces/loopback",
		0,
		std_ops
	},
	
	NET_LAYER_API_VERSION,
	
	init,
	uninit,
	enable,
	NULL,	// no control()
	NULL,	// interface layer: processing input buffer doesn't make sense!
	process_output
};

_EXPORT module_info *modules[] = {
	(module_info*) &nlmi,	// net_layer_module_info
	NULL
};
