/* ipv4_module.h
 * definitions for ipv4 protocol
 */
 
#ifndef IPV4_MODULE_H
#define IPV4_MODULE_H

#ifndef _KERNEL_MODE
#error "This is a kernel ONLY file"
#endif

#include "net/net_module.h"

#define IPV4_MODULE_PATH	"network/protocol/ipv4"

struct ipv4_module_info {
	struct kernel_net_module_info info;
	int (*output)(struct mbuf *, 
	                 struct mbuf *, 
	                 struct route *,
	                 int, void *);
	int (*ctloutput)(int,
	                 struct socket *,
                     int, int,
                     struct mbuf **);
	struct mbuf *(*ip_srcroute)(void);
	void (*ip_stripoptions)(struct mbuf *, 
	                        struct mbuf *);
	struct mbuf *(*srcroute)(void);
};

extern uint16 ip_id;

#endif /* IPV4_MODULE_H */
