/* net_module.h
 * fundtions and staructures for all modules using the
 * net_server
 */

#ifndef OBOS_NET_MODULE_H
#define OBOS_NET_MODULE_H

#include <kernel/OS.h>
#include <image.h>

#include "sys/mbuf.h"
#include "net_misc.h"
#include "sys/socketvar.h"
#include "net/if.h"
#include "net/route.h"

#ifdef _KERNEL_
#include <module.h>
#else
typedef struct module_info {
	const char *name;
	uint32 flags;
	status_t (*std_ops)(int32, ...);
} module_info;

#define	B_MODULE_INIT	    1
#define	B_MODULE_UNINIT	    2
#define	B_KEEP_LOADED		0x00000001
#endif

struct kernel_net_module_info {
	module_info info;
	int (*start)(void *);
	int (*stop)(void);
};

struct net_module {
	struct net_module *next;
	char *name;
	struct kernel_net_module_info *ptr;
#ifndef _KERNEL_
	image_id iid;
#endif
	int status;
};
	
enum {
	NET_LAYER1	= 1, /* link layer */
	NET_LAYER2,	     /* network layer */
	NET_LAYER3,	     /* transport layer */
	NET_LAYER4	     /* socket layer */
};

#endif /* OBOS_NET_MODULE_H */

