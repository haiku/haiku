/* stack.c */

/* This the heart of network stack
 */

#include <stdio.h>

#include <drivers/module.h>
#include <drivers/KernelExport.h>

#include "net_stack.h"
#include "memory_pool.h"

#include "attribute.h"
#include "layers_manager.h"
#include "buffer.h"
#include "timer.h"
#include "dump.h"

memory_pool_module_info *g_memory_pool = NULL;

static bool g_started = false;

status_t 		std_ops(int32 op, ...);

static status_t start(void);
static status_t stop(void);


static status_t start(void)
{
	if (g_started)
		return B_OK;

	dprintf("stack: starting...\n");

	start_buffers_service();
	start_timers_service();

	start_layers_manager();

	dprintf("stack: started.\n");
	g_started = true;

	return B_OK;
}

static status_t stop(void)
{
	dprintf("stack: stopping...\n");

	stop_layers_manager();

	stop_timers_service();
	stop_buffers_service();


	dprintf("stack: stopped.\n");
	g_started = false;
			
	return 0;
}


// #pragma mark -

struct net_stack_module_info nsmi = {
	{
		NET_STACK_MODULE_NAME,
		0, // B_KEEP_LOADED,
		std_ops
	},

	// Global stack control
	start,
	stop,

	// Attributs IDs
	register_attribute_id,
	
	// Layers handling
	register_layer,
	unregister_layer,
	find_layer,
	add_layer_attribute,
	remove_layer_attribute,
	find_layer_attribute,
	
	send_up,
	send_down,
	
	// net_buffer support
	new_buffer,
	delete_buffer,
	duplicate_buffer,
	clone_buffer,
	split_buffer,
	
	add_to_buffer,
	remove_from_buffer,

	attach_buffer_free_element,
	
	read_buffer,
	write_buffer,
	
	add_buffer_attribute,
	remove_buffer_attribute,
	find_buffer_attribute,

	dump_buffer,
	
	// net_buffer_queue support
	new_buffer_queue,
	delete_buffer_queue,
	empty_buffer_queue,
	enqueue_buffer,
	dequeue_buffer,
	
	// Timers support
	new_net_timer,
	delete_net_timer,
	start_net_timer,
	cancel_net_timer,
	net_timer_appointment,
	
	// Misc.
	dump_memory
};

status_t std_ops(int32 op, ...) 
{
	status_t	status;

	switch(op) {
		case B_MODULE_INIT:
			dprintf("stack: B_MODULE_INIT\n");
			load_driver_symbols("stack");
			status = get_module(MEMORY_POOL_MODULE_NAME, (module_info **) &g_memory_pool);
			if (status != B_OK) {
				dprintf("stack: Can't load " MEMORY_POOL_MODULE_NAME " module!\n");
				return status;
			};
				
			// Re-publish memory_pool module api thru our
			nsmi.new_pool 			= g_memory_pool->new_pool;
			nsmi.delete_pool 		= g_memory_pool->delete_pool;
			nsmi.new_pool_node		= g_memory_pool->new_pool_node;
			nsmi.delete_pool_node	= g_memory_pool->delete_pool_node;
			nsmi.for_each_pool_node	= g_memory_pool->for_each_pool_node;
			break;

		case B_MODULE_UNINIT:
			dprintf("stack: B_MODULE_UNINIT\n");
			put_module(MEMORY_POOL_MODULE_NAME);
			break;

		default:
			return B_ERROR;
	}
	return B_OK;
}

_EXPORT module_info *modules[] = {
	(module_info *) &nsmi, 	// net_stack_module_info
	NULL
};

