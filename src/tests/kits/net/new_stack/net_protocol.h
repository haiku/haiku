/* net_protocol.h
 * definitions of protocol networking module API
 */

#ifndef OBOS_NET_PROTOCOL_H
#define OBOS_NET_PROTOCOL_H

#include <drivers/module.h>

struct net_protocol_module_info {
	module_info	module;

};

#define NET_PROTOCOL_MODULE_ROOT	"network/protocols/"

#endif /* OBOS_NET_PROTOCOL_H */
