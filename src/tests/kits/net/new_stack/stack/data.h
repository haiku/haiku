/* data.h
 * private definitions for network data chunks support
 */

#ifndef OBOS_NET_STACK_DATA_H
#define OBOS_NET_STACK_DATA_H

#include <SupportDefs.h>

#include "net_stack.h"

#ifdef __cplusplus
extern "C" {
#endif

extern status_t 	start_data_service();
extern status_t 	stop_data_service();

// Network data chunk(s)
extern net_data * 		new_data(void);
extern status_t 		delete_data(net_data *nd, bool interrupt_safe);

extern net_data *		duplicate_data(net_data *from);
extern net_data *		clone_data(net_data *from);

extern 	status_t		prepend_data(net_data *nd, const void *data, uint32 bytes, data_node_free_func freethis);
extern 	status_t		append_data(net_data *nd, const void *data, uint32 bytes, data_node_free_func freethis);
extern	status_t		insert_data(net_data *nd, uint32 offset, const void *data, uint32 bytes, data_node_free_func freethis);
extern	status_t		remove_data(net_data *nd, uint32 offset, uint32 bytes);

extern	status_t 		add_data_free_node(net_data *nd, void *arg1, void *arg2, data_node_free_func freethis);

extern uint32 			copy_from_data(net_data *nd, uint32 offset, void * copyinto, uint32 bytes);

extern void				dump_data(net_data *data);

// Network data(s) queue(s)
extern net_data_queue * 	new_data_queue(size_t max_bytes);
extern status_t 			delete_data_queue(net_data_queue *queue);

extern status_t		empty_data_queue(net_data_queue *queue);
extern status_t 	enqueue_data(net_data_queue *queue, net_data *data);
extern size_t  		dequeue_data(net_data_queue *queue, net_data **data, bigtime_t timeout, bool peek); 

#ifdef __cplusplus
}
#endif

#endif	// OBOS_NET_STACK_DATA_H
