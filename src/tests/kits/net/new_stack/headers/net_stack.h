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

// String tokens use here and there by stack components
typedef const void *string_token;

// Networking attribute(s) definition
typedef struct net_attribute net_attribute;

#define NET_ATTRIBUTE_TYPE_MASK 	0xFF		// Up to 256 basic types
#define NET_ATTRIBUTE_FLAGS_MASK	0xFFFFFF00
enum {
	NET_ATTRIBUTE_BOOL = 1,		// ... = bool value
	NET_ATTRIBUTE_BYTE,			// ... = uint8 value
	NET_ATTRIBUTE_INT16,		// ... = uint16 value
	NET_ATTRIBUTE_INT32,		// ... = uint32 value
	NET_ATTRIBUTE_INT64,		// ... = uint64 value
	NET_ATTRIBUTE_DATA,			// ... = void *data, size_t data_len
	NET_ATTRIBUTE_STRING,		// ... = char *string
	NET_ATTRIBUTE_POINTER,		// ... = void *ptr
	NET_ATTRIBUTE_IOVEC			// ... = int nb, iovec *vec
};

// Networking buffer(s) definition
typedef struct net_buffer net_buffer;
typedef struct net_buffer_queue net_buffer_queue;
typedef void (*buffer_chunk_free_func)(void * arg1, ...); // buffer chunk free callback prototype

// Networking timers definitions
typedef struct net_timer net_timer;
typedef void (*net_timer_func)(net_timer *timer, void *cookie);	// timer callback prototype

// Generic lockers support
typedef int benaphore;
#define create_benaphore(a, b)
#define delete_benaphore(a)
#define lock_benaphore(a)
#define unlock_benaphore(a)

// Networking layer(s) definitions
typedef struct net_layer net_layer;
typedef struct net_layer_module_info net_layer_module_info;

#define NET_MODULES_ROOT	"network_v2/"

struct net_layer_module_info {
	module_info	info;
	
	uint32	api_version;
	#define NET_LAYER_API_VERSION	(0x1)
	
	status_t (*init)(net_layer *me);
	status_t (*uninit)(net_layer *me);
	status_t (*enable)(net_layer *me, bool enable);
	status_t (*control)(net_layer *me, int opcode, ...);
	status_t (*process_input)(net_layer *me, struct net_buffer *buffer);
	status_t (*process_output)(net_layer *me, struct net_buffer *buffer);
};

struct net_layer {
	struct net_layer 		*next;
	char 					*name;
	string_token			type;		// "frame", "ethernet" "ipv4", "ppp", etc. Joker (*) allowed
	string_token			sub_type;	// joker (*) allowed here too.
	int						priority;	// negative values reserved for packet sniffing, high value = lesser priority
	net_layer_module_info 	*module;
	void 					*cookie;
	uint32					use_count;	
	struct net_layer 		**layers_above;
	struct net_layer		**layers_below;
	struct net_attribute 	*attributes;  // layer attributes
};

// Network stack main module definition

struct net_stack_module_info {
	module_info	module;

	// Global stack control
	status_t (*start)(void);
	status_t (*stop)(void);

	// String IDs atomizer
	string_token 	(*string_to_token)(const char *name);
	const char * 	(*string_for_token)(string_token token);
	
	/*
	 * Socket layer
	 */
	 
	/*
	 * Net layers handling
	 */
	
	status_t 	(*register_layer)(const char *name, const char *packets_type, int priority,
					net_layer_module_info *module, void *cookie, net_layer **layer);
	status_t 	(*unregister_layer)(net_layer *layer);
	net_layer *	(*find_layer)(const char *name);

	status_t 	(*add_layer_attribute)(net_layer *layer, string_token id, int type, ...);
	status_t 	(*remove_layer_attribute)(net_layer *layer, string_token id, int index);
	status_t 	(*find_layer_attribute)(net_layer *layer, string_token id, int index,
					int *type, void **value, size_t *size);
	
	status_t  	(*send_layers_up)(net_layer *me, net_buffer *buffer);
	status_t  	(*send_layers_down)(net_layer *me, net_buffer *buffer);

	/*
	 * Buffer(s) support
	 */


	net_buffer *	(*new_buffer)(void);
	status_t 		(*delete_buffer)(net_buffer *buffer, bool interrupt_safe);

	net_buffer *	(*duplicate_buffer)(net_buffer *from);
	net_buffer *	(*clone_buffer)(net_buffer *from);
	net_buffer *	(*split_buffer)(net_buffer *from, uint32 offset);


	// specials offsets for add_to_buffer()	
	#define BUFFER_BEGIN 0
	#define BUFFER_END 	 0xFFFFFFFF
	status_t		(*add_to_buffer)(net_buffer *buffer, uint32 offset, const void *data, size_t bytes, buffer_chunk_free_func freethis);
	status_t		(*remove_from_buffer)(net_buffer *buffer, uint32 offset, size_t bytes);

	status_t 		(*attach_buffer_free_element)(net_buffer *buffer, void *arg1, void *arg2, buffer_chunk_free_func freethis);

	size_t 			(*read_buffer)(net_buffer *buffer, uint32 offset, void *data, size_t bytes);
	size_t 			(*write_buffer)(net_buffer *buffer, uint32 offset, const void *data, size_t bytes);

	// Flags to combine with  
	#define FROM_BUFFER		0x010000	// ... = int offset, int size 
	#define NETWORK_ORDER	0x020000	
	status_t 		(*add_buffer_attribute)(net_buffer *buffer, string_token id, int type, ...);
	status_t 		(*remove_buffer_attribute)(net_buffer *buffer, string_token id, int index);
	status_t 		(*find_buffer_attribute)(net_buffer *buffer, string_token id, int index, 
						int *type, void **value, size_t *size);
	
	void			(*dump_buffer)(net_buffer *buffer);

	// Buffers queues support
	net_buffer_queue * (*new_buffer_queue)(size_t max_bytes);
	status_t 		(*delete_buffer_queue)(net_buffer_queue *queue);

	status_t		(*empty_buffer_queue)(net_buffer_queue *queue);
	status_t 		(*enqueue_buffer)(net_buffer_queue *queue, net_buffer *buffer);
	size_t  		(*dequeue_buffer)(net_buffer_queue *queue, net_buffer **buffer, bigtime_t timeout, bool peek); 

	// Timers support
	
	net_timer *	(*new_timer)(void);
	status_t	(*delete_timer)(net_timer *timer);
	status_t	(*start_timer)(net_timer *timer, net_timer_func func, void *cookie, bigtime_t period);
	status_t	(*cancel_timer)(net_timer *timer);
	status_t	(*timer_appointment)(net_timer *timer, bigtime_t *period, bigtime_t *when);
	
	// Lockers
	
	// Misc.
	void 	(*dump_memory)(const char *prefix, const void *data, size_t len);
	
	// Memory Pools support
	memory_pool *	(*new_pool)(size_t node_size, uint32 node_count);
	status_t		(*delete_pool)(memory_pool *pool);
	void *			(*new_pool_node)(memory_pool *pool);
	status_t		(*delete_pool_node)(memory_pool *pool, void *node);
	status_t		(*for_each_pool_node)(memory_pool *pool, pool_iterate_func iterate, void *cookie);

};

#define NET_STACK_MODULE_NAME	NET_MODULES_ROOT "stack/v1"

#ifdef __cplusplus
}
#endif

#endif /* OBOS_NET_STACK_H */
