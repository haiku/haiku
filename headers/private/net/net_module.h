/* net_module.h
 * fundtions and staructures for all modules using the
 * net_server
 */

#ifndef _NET_NET_MODULE_H
#define _NET_NET_MODULE_H

#include <OS.h>

#include <module.h>

struct kernel_net_module_info {
	module_info info;
	int (*start)(void *);
	int (*stop)(void);
};

struct net_module {
	struct net_module *next;
	char *name;
	struct kernel_net_module_info *ptr;
	int status;
};

int  inetctlerrmap(int);
 	
#endif /* _NET_NET_MODULE_H */

