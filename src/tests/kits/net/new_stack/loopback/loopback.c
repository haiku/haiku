/* loopback.c - loopback device 
 */

#include <stdio.h>
#include <stdlib.h>

#include "net_stack.h"
#include "net_interface.h"

status_t 		std_ops(int32 op, ...);

struct net_interface_module_info nimi;

struct net_stack_module_info *g_stack = NULL;

status_t init(void * params)
{
	ifnet_t * iface;
	
	iface = (ifnet_t *) malloc(sizeof(*iface));
	if (!iface)
		return B_ERROR;
	
	iface->if_name = "loopback";
	iface->if_flags = 0;
	iface->module = &nimi;
	
	g_stack->register_interface(iface);

	return B_OK;
}

status_t uninit(ifnet_t *iface)
{
	printf("loopback: uniniting %s interface\n", iface->if_name);
	return B_OK;
}

status_t up(ifnet_t *iface)
{
	return B_ERROR;
}


status_t down(ifnet_t *iface)
{
	return B_ERROR;
}


status_t send(ifnet_t *iface, net_data *data)
{
	if (!data)
		return B_ERROR;
		
	return B_OK;
}


status_t receive(ifnet_t *iface, net_data **data)
{
	return B_ERROR;
}
	
status_t control(ifnet_t *iface, int opcode, void *arg) { return -1; }
	
status_t get_hardware_address(ifnet_t *iface, net_interface_hwaddr *hwaddr) { return -1; }
status_t get_multicast_addresses(ifnet_t *iface, net_interface_hwaddr **hwaddr, int nb_addresses) { return -1; }
status_t set_multicast_addresses(ifnet_t *iface, net_interface_hwaddr *hwaddr, int nb_addresses) { return -1; }
	
status_t set_mtu(ifnet_t *iface, uint32 mtu)  { return -1; }
status_t set_promiscuous(ifnet_t *iface, bool enable) { return -1; }
status_t set_media(ifnet_t *iface, uint32 media)  { return -1; }


// #pragma mark -

status_t std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT:
			printf("loopback: B_MODULE_INIT\n");
			return get_module(NET_STACK_MODULE_NAME, (module_info **) &g_stack);
			
		case B_MODULE_UNINIT:
			printf("loopback: B_MODULE_UNINIT\n");
			put_module(NET_STACK_MODULE_NAME);
			break;
			
		default:
			return B_ERROR;
	}
	return B_OK;
}

struct net_interface_module_info nimi = {
	{
		NET_INTERFACE_MODULE_ROOT "loopback",
		0,
		std_ops
	},
	
	init,
	uninit,
	up,
	down,
	send,
	receive,
	control,
	get_hardware_address,
	get_multicast_addresses,
	set_multicast_addresses,
	set_mtu,
	set_promiscuous,
	set_media
};

_EXPORT module_info *modules[] = {
	(module_info*) &nimi,	// net_interface_module_info
	NULL
};
