/* ipv4_module.h
 * definitions for ipv4 protocol
 */
 
#ifndef IPV4_MODULE_H
#define IPV4_MODULE_H

#include "net_module.h"

#ifdef _KERNEL_MODE
#include <KernelExport.h>
#define IPV4_MODULE_PATH	"network/protocol/ipv4"
#else /* _KERNEL_MODE */
#define IPV4_MODULE_PATH	"modules/protocol/ipv4"
#endif /* _KERNEL_MODE */

struct ipv4_module_info {
	struct kernel_net_module_info info;
#ifndef _KERNEL_MODE
	void (*set_core)(struct core_module_info *);
#endif

	int (*output)(struct mbuf *, 
	                 struct mbuf *, 
	                 struct route *,
	                 int, void *);
	uint16 (*ip_id)(void);
	int (*ctloutput)(int,
	                 struct socket *,
                     int, int,
                     struct mbuf **);
	struct mbuf *(*ip_srcroute)(void);
	void (*ip_stripoptions)(struct mbuf *, 
	                        struct mbuf *);
	struct mbuf *(*srcroute)(void);
};

#endif /* IPV4_MODULE_H */
