/* ipv4_module.h
 * definitions for ipv4 protocol
 */
 
#ifndef IPV4_MODULE_H
#define IPV4_MODULE_H

#include "net_module.h"

#include <KernelExport.h>

#define NET_IPV4_MODULE_NAME	NETWORK_MODULES_ROOT "protocols/ipv4"

struct ipv4_module_info {
	struct kernel_net_module_info info;

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
