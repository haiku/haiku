/* net_module.h
 * fundtions and staructures for all modules using the
 * net_server
 */

#ifndef OBOS_NET_MODULE_H
#define OBOS_NET_MODULE_H

#include <kernel/OS.h>
#include <image.h>

#include "mbuf.h"
#include "net_misc.h"
#include "sys/socketvar.h"
#include "net/if.h"
#include "net/route.h"

#include <module.h>

struct kernel_net_module_info {
	module_info info;
	int (*start)(void *);
	int (*stop)(void);
	status_t (*control)(uint32 op, void *data, size_t length);
};

struct net_module {
	struct net_module *next;
	char *name;
	struct kernel_net_module_info *ptr;
	int status;
};

#endif /* OBOS_NET_MODULE_H */

