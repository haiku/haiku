/* icmp_module.h
 */
 
#ifndef ICMP_MODULE_H
#define ICMP_MODULE_H

#include "net_module.h"

#ifdef _KERNEL_MODE
#include <KernelExport.h>
#define ICMP_MODULE_PATH	"network/protocol/icmp"
#else /* _KERNEL_MODE */
#define ICMP_MODULE_PATH	"modules/protocol/icmp"
#endif /* _KERNEL_MODE */

struct icmp_module_info {
	struct kernel_net_module_info info;

#ifndef _KERNEL_MODE
	void (*set_core)(struct core_module_info *);
#endif

	void (*error)(struct mbuf *, int, int, n_long, struct ifnet *);
};

#endif /* ICMP_MODULE_H */
