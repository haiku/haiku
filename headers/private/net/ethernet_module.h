/*
	ethernet_module.h
*/

#ifndef ETHERNET_MODULE_H
#define ETHERNET_MODULE_H

#include "net_module.h"

#include <KernelExport.h>
#define NET_ETHERNET_MODULE_NAME	"network/interfaces/ethernet"

typedef void (*ethernet_receiver_func)(struct mbuf *buf);

struct ethernet_module_info {
	struct kernel_net_module_info info;
	
	void (*set_pppoe_receiver)(ethernet_receiver_func func);
	void (*unset_pppoe_receiver)(void);
};

#endif /* ETHERNET_MODULE_H */
