/* loopback.c - loopback device 
 */

#include <stdio.h>
#include <stdlib.h>

#include "net_stack.h"
#include "net_interface.h"

#define LOOPBACK_MTU 16384 // can be as large as we want


typedef struct loopback_interface {
	ifnet_t ifnet;
	net_data_queue *queue;
} loopback_interface;


status_t 		std_ops(int32 op, ...);

struct net_interface_module_info nimi;

struct net_stack_module_info *g_stack = NULL;

status_t init(void * params)
{
	ifnet_t * iface;
	loopback_interface *li;
	
	li = (loopback_interface *) malloc(sizeof(*li));
	if (!li)
		return B_NO_MEMORY;		

	// create loopback fifo queue, to keep packets *sent*
	li->queue = g_stack->new_data_queue(64*LOOPBACK_MTU);


	iface = &li->ifnet;
	iface->if_name 	= "loopback";
	iface->if_flags = (IFF_LOOPBACK | IFF_MULTICAST);
	iface->if_type 	= 0x10; //IFT_LOOP
	iface->if_mtu 	= LOOPBACK_MTU;
			
	// never forget to store a pointer to us, otherwise datalink layer can't call us
	iface->module = &nimi;
			
	return g_stack->register_interface(iface);
}

status_t uninit(ifnet_t *iface)
{
	loopback_interface *li = (loopback_interface *) iface;

	printf("loopback: uniniting %s interface\n", iface->if_name);
	return g_stack->delete_data_queue(li->queue);
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
	loopback_interface *li = (loopback_interface *) iface;

	if (!data)
		return B_ERROR;

	return g_stack->enqueue_data(li->queue, data);
}


status_t receive(ifnet_t *iface, net_data **data)
{
	loopback_interface *li = (loopback_interface *) iface;
	
	return g_stack->dequeue_data(li->queue, data, B_INFINITE_TIMEOUT, false);
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
