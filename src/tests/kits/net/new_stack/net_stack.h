/* net_stack.h
 * definitions needed by all network stack modules
 */

#ifndef OBOS_NET_STACK_H
#define OBOS_NET_STACK_H

#include <drivers/module.h>

#include "memory_pool.h"

#ifdef __cplusplus
extern "C" {
#endif

// Networking data chunk(s) definition
typedef struct net_data net_data;
typedef struct net_data_queue net_data_queue;
typedef void (*data_node_free_func)(void * arg1, ...); // data node free callback prototype

// Networking timers definitions
typedef struct net_timer net_timer;
typedef void (*net_timer_func)(net_timer *timer, void *cookie);	// timer callback prototype

// Generic lockers support

typedef int benaphore;
#define create_benaphore(a, b)
#define delete_benaphore(a)
#define lock_benaphore(a)
#define unlock_benaphore(a)

#include "net_interface.h"


// Network stack main module definition

struct net_stack_module_info {
	module_info	module;

	status_t (*start)(void);
	status_t (*stop)(void);
	
	/*
	 * Socket layer
	 */
	 
	/*
	 * Data-Link layer
	 */
	
	status_t (*register_interface)(ifnet_t *ifnet);
	status_t (*unregister_interface)(ifnet_t *ifnet);

	/*
	 * Data chunk(s) support
	 */

	net_data *		(*new_data)(void);
	status_t 		(*delete_data)(net_data *nd, bool interrupt_safe);

	net_data *		(*duplicate_data)(net_data *from);
	net_data *		(*clone_data)(net_data *from);
	
	status_t		(*prepend_data)(net_data *nd, const void *data, uint32 bytes, data_node_free_func freethis);
	status_t		(*append_data)(net_data *nd, const void *data, uint32 bytes, data_node_free_func freethis);
	status_t		(*insert_data)(net_data *nd, uint32 offset, const void *data, uint32 bytes, data_node_free_func freethis);
	status_t		(*remove_data)(net_data *nd, uint32 offset, uint32 bytes);

	status_t 		(*add_data_free_node)(net_data *nd, void *arg1, void *arg2, data_node_free_func freethis);

	uint32 			(*copy_from_data)(net_data *nd, uint32 offset, void *copyinto, uint32 bytes);
	
	void			(*dump_data)(net_data *nd);

	// Data queues support
	net_data_queue * (*new_data_queue)(size_t max_bytes);
	status_t 		(*delete_data_queue)(net_data_queue *queue);

	status_t		(*empty_data_queue)(net_data_queue *queue);
	status_t 		(*enqueue_data)(net_data_queue *queue, net_data *data);
	size_t  		(*dequeue_data)(net_data_queue *queue, net_data **data, bigtime_t timeout, bool peek); 

	// Timers support
	
	net_timer *	(*new_timer)(void);
	status_t	(*delete_timer)(net_timer *timer);
	status_t	(*start_timer)(net_timer *timer, net_timer_func func, void *cookie, bigtime_t period);
	status_t	(*cancel_timer)(net_timer *timer);
	status_t	(*get_timer_appointment)(net_timer *timer, bigtime_t *period, bigtime_t *when);
	
	// Lockers
	
	// Memory Pools support
	memory_pool *	(*new_pool)(size_t node_size, uint32 node_count);
	status_t		(*delete_pool)(memory_pool *pool);
	void *			(*new_pool_node)(memory_pool *pool);
	status_t		(*delete_pool_node)(memory_pool *pool, void *node);
	status_t		(*for_each_pool_node)(memory_pool *pool, pool_iterate_func iterate, void *cookie);
	
};

#define NET_STACK_MODULE_NAME	"network/stack/v1"

#ifdef __cplusplus
}
#endif

#endif /* OBOS_NET_STACK_H */
