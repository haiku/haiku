/* raw_module.h
 */
 
#ifndef RAW_MODULE_H
#define RAW_MODULE_H

#include "net_module.h"

#include <KernelExport.h>


#define NET_RAW_MODULE_NAME	  NETWORK_MODULES_ROOT "protocols/raw"

struct raw_module_info {
	struct kernel_net_module_info info;

	void (*input)(struct mbuf *, int);
};

#endif /* RAW_MODULE_H */
