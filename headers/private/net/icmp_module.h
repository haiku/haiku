/* icmp_module.h
 */
 
#ifndef ICMP_MODULE_H
#define ICMP_MODULE_H

#include "net_module.h"

#include <KernelExport.h>

#define NET_ICMP_MODULE_NAME	NETWORK_MODULES_ROOT "protocols/icmp"

struct icmp_module_info {
	struct kernel_net_module_info info;

	void (*error)(struct mbuf *, int, int, n_long, struct ifnet *);
};

#endif /* ICMP_MODULE_H */
