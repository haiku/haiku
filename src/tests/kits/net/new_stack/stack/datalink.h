/* datalink.h
 * private definitions for network datalink layer
 */

#ifndef OBOS_NET_STACK_DATALINK_H
#define OBOS_NET_STACK_DATALINK_H

#include "net_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

extern status_t		start_datalink_layer();
extern status_t		stop_datalink_layer();

extern status_t register_interface(ifnet_t *ifnet);
extern status_t unregister_interface(ifnet_t *ifnet);

#ifdef __cplusplus
}
#endif

#endif /* OBOS_NET_STACK_DATALINK_H */
