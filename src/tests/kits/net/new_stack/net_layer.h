/* net_layer.h
 * definitions of networking layer(s) API
 */

#ifndef OBOS_NET_LAYER_H
#define OBOS_NET_LAYER_H

#include <drivers/module.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_buffer;

typedef struct net_layer {
	struct net_layer 				*next;
	char 							*name;
	int								level;
	uint32							kind;	// Filter, consumer, producer, etc...
	int								chaining_constraint;
	struct net_layer_module_info 	*module;
	struct net_layer 				**layers_above;
	struct net_layer				**layers_below;
} net_layer;

struct net_layer_module_info {
	module_info	info;

	status_t (*init)(void * params);
	status_t (*uninit)(net_layer *me);
	status_t (*enable)(net_layer *me, bool enable);
	status_t (*input_buffer)(net_layer *me, struct net_buffer *buffer);
	status_t (*output_buffer)(net_layer *me, struct net_buffer *buffer);
};

#define NET_LAYER_MODULE_ROOT	"network/layers/"

#ifdef __cplusplus
}
#endif

#endif /* OBOS_NET_LAYER_H */
