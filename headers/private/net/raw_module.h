/* raw_module.h
 */
 
#ifndef RAW_MODULE_H
#define RAW_MODULE_H

#include "net_module.h"

#ifdef _KERNEL_
#include <KernelExport.h>
#define RAW_MODULE_PATH	      "network/protocol/raw"
#else
#define RAW_MODULE_PATH       "modules/protocols/raw"
#endif

struct raw_module_info {
	struct kernel_net_module_info info;
	void (*input)(struct mbuf *, int);
};

#endif /* RAW_MODULE_H */
