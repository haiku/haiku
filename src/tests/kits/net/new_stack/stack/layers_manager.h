/* layers_manager.h
 * private definitions for network layers manager
 */

#ifndef OBOS_NET_STACK_LAYERS_MANAGER_H
#define OBOS_NET_STACK_LAYERS_MANAGER_H

#include "net_layer.h"

#ifdef __cplusplus
extern "C" {
#endif

extern status_t	start_layers_manager();
extern status_t	stop_layers_manager();

extern status_t register_layer(net_layer *layer);
extern status_t unregister_layer(net_layer *layer);

extern status_t push_buffer_up(net_layer *me, struct net_buffer *buffer);
extern status_t push_buffer_down(net_layer *me, struct net_buffer *buffer);


#ifdef __cplusplus
}
#endif

#endif /* OBOS_NET_STACK_LAYERS_MANAGER_H */
