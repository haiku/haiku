#ifndef OBOS_MEMORY_POOL_H
#define OBOS_MEMORY_POOL_H

#include <drivers/module.h>

#ifdef __cplusplus
extern "C" {
#endif

// Pools of *nodes* kernel module
// Pools store any node of data, but they should be all the same node_size 
// size (set at pool creation time).
// Very usefull for multiple same-sized structs storage...

struct memory_pool;
typedef struct memory_pool memory_pool;

// for_each_pool_node() callback prototype:
typedef status_t (*pool_iterate_func)(memory_pool * pool, void * node, void * cookie);

typedef struct {
	module_info	module;

	memory_pool *	(*new_pool)(size_t node_size, uint32 node_count);
	status_t		(*delete_pool)(memory_pool * pool);
	void *			(*new_pool_node)(memory_pool * pool);
	status_t		(*delete_pool_node)(memory_pool * pool, void * node);
	status_t		(*for_each_pool_node)(memory_pool * pool, pool_iterate_func iterate, void * cookie);
} memory_pool_module_info;

#define MEMORY_POOL_MODULE_NAME	"generic/memory_pool/v1"

#ifdef __cplusplus
}
#endif

#endif /* OBOS_MEMORY_POOL_H */

