/* layers_manager.h
 * private definitions for network layers manager
 */

#ifndef OBOS_NET_STACK_LAYERS_MANAGER_H
#define OBOS_NET_STACK_LAYERS_MANAGER_H

#include "net_stack.h"

#ifdef __cplusplus
extern "C" {
#endif

extern status_t		start_layers_manager(void);
extern status_t		stop_layers_manager(void);

extern status_t 	register_layer(const char *name, const char *type, int priority,
					net_layer_module_info *module, void *cookie, net_layer **layer);
extern status_t 	unregister_layer(net_layer *layer);
extern net_layer *	find_layer(const char *name);

extern status_t 	add_layer_attribute(net_layer *layer, const void *id, int type, ...);
extern status_t 	remove_layer_attribute(net_layer *layer, const void *id);
extern status_t 	find_layer_attribute(net_layer *layer, const void *id,
						int *type, void **attribute, size_t *size);

extern status_t 	send_layers_up(net_layer *me, struct net_buffer *buffer);
extern status_t 	send_layers_down(net_layer *me, struct net_buffer *buffer);


#ifdef __cplusplus
}
#endif

#endif /* OBOS_NET_STACK_LAYERS_MANAGER_H */
