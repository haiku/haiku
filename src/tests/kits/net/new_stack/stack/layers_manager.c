/* layers_manager.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <KernelExport.h>
#include <OS.h>

#include "layers_manager.h"
#include "buffer.h"

/* Defines we need */
#define NET_LAYER_MODULES_ROOT	"network/layers/"

struct layers_list {
	net_layer	*first;
	sem_id		lock;
};

static struct layers_list g_layers;

//	#pragma mark [Start/Stop functions]


// --------------------------------------------------
status_t start_layers_manager()
{
	void 		*module_list;
	net_layer 	*layer;

	g_layers.first = NULL;
	
	g_layers.lock = create_sem(1, "net_layers list lock");
	if (g_layers.lock < B_OK)
		return B_ERROR;

#ifdef _KERNEL_MODE
	set_sem_owner(g_layers.lock, B_SYSTEM_TEAM);
#endif

	// Load all network/interfaces/* modules and let them
	// register any layer they may support by calling init()
	module_list = open_module_list(NET_LAYER_MODULES_ROOT);
	if (module_list) {
		size_t sz;
		char module_name[256];
		struct net_layer_module_info *nlmi;
		
		sz = sizeof(module_name);
		while(read_next_module_name(module_list, module_name, &sz) == B_OK) {
			sz = sizeof(module_name);
			if (!strlen(module_name))
				continue;

			if (get_module(module_name, (module_info **) &nlmi) != B_OK)
				continue;
				
			printf("layers_manager: initing %s layer(s) module...\n", module_name);
			// allow this module to register one or more interfaces
			nlmi->init(NULL);
			// this module may have been acquire more time by calling register_layer()
			put_module(module_name);
		};
		close_module_list(module_list);
	};

	// auto-start layer(s)
	acquire_sem(g_layers.lock);
	layer = g_layers.first;
	while (layer) {
		layer->module->enable(layer, true);
		layer = layer->next;
	};
	release_sem(g_layers.lock);

	puts("net layers manager started.");
	return B_OK;
}

// --------------------------------------------------
status_t stop_layers_manager()
{
	net_layer *layer, *next;
	const char *module_name;

	delete_sem(g_layers.lock);
	g_layers.lock = -1;

	// TODO: handle multiple layers registered by same net module 

	// unregister all layers... 
	layer = g_layers.first;
	while (layer) {
		next = layer->next;
		module_name = layer->module->info.name;
		printf("layers_manager: uniniting '%s' layer\n", layer->name);
		// down the layer if currently up
		layer->module->enable(layer, false);
		layer->module->uninit(layer);
		printf("layers_manager: unloading %s layer(s) module\n", module_name);
		put_module(module_name);
		layer = next;
	};

	puts("net layers manager stopped.");

	return B_OK;
}



// #pragma mark [Public functions]

// --------------------------------------------------
status_t register_layer(net_layer *layer)
{
	status_t status;
	module_info *dummy;

	if (!layer)
		return B_ERROR;
	
	if (layer->module == NULL)
		return B_ERROR;		
		
	// acquire this module, to keep it loaded while this layer is registered
	status = get_module(layer->module->info.name, &dummy);
	if (status != B_OK)
		return status;

	layer->layers_above = NULL;
	layer->layers_below = NULL;
		
	status = acquire_sem(g_layers.lock);
	if (status != B_OK)
		return status;

	layer->next = g_layers.first;
	g_layers.first = layer;
	
	release_sem(g_layers.lock);

		
	printf("layers_manager: '%s' layer registered\n", layer->name);
	return B_OK;
}

// --------------------------------------------------
status_t unregister_layer(net_layer *layer)
{
	status_t status;

	if (!layer)
		return B_ERROR;
		
	status = acquire_sem(g_layers.lock);
	if (status != B_OK)
		return status;

	if (g_layers.first == layer)
		g_layers.first = layer->next;
	else {
		net_layer * p = g_layers.first;
		while (p && p->next != layer)
			p = p->next;
			
		if (!p)
			printf("layers_manager: unregister_layer(): %p layer not found in list!\n", layer);
		else
			p->next = layer->next;
	};
	
	release_sem(g_layers.lock);

	printf("layers_manager: '%s' layer unregistered\n", layer->name);
	return B_OK;
}


// --------------------------------------------------
status_t push_buffer_up(net_layer *layer, net_buffer *buffer)
{
	net_layer *above;
	status_t status;
	int i;

	if (!layer)
		return B_ERROR;

	// #### TODO: lock the layer(s) above & below hierarchy
	if (!layer->layers_above)
		// no layers above this one
		return B_ERROR;
	
	status = B_ERROR;
	i = 0;
	while (layer->layers_above[i]) {
		above = layer->layers_above[i++];
		if (!above->module->input_buffer)
			// this layer don't input buffer
			continue;
			
		status = above->module->input_buffer(above, buffer);
		if (status == B_OK)
			return status;
	};

	return status;
}


status_t push_buffer_down(net_layer *layer, net_buffer *buffer)
{
	net_layer *below;
	status_t status;
	int i;

	if (!layer)
		return B_ERROR;

	// #### TODO: lock the layer(s) above & below hierarchy
	if (!layer->layers_below)
		// no layers below this one
		return B_ERROR;
	
	status = B_ERROR;
	i = 0;
	while (layer->layers_below[i]) {
		below = layer->layers_below[i++];
		if (!below->module->output_buffer)
			// this layer don't output buffer
			continue;
			
		status = below->module->output_buffer(below, buffer);
		if (status == B_OK)
			return status;
	};

	return status;
}

// #pragma mark [Private functions]


#if 0
// ----------------------------------------------------
status_t interface_reader(void *args)
{
	ifnet_t 	*iface = args;
	net_buffer 	*buffer;
	status_t 	status;
	
	if (!iface || iface->module == NULL)
		return B_ERROR;
		
	while(iface->if_flags & IFF_UP) {
		buffer = NULL;
		status = iface->module->receive_buffer(iface, &buffer);
		if (status < B_OK || buffer == NULL)
			continue;

		dump_buffer(buffer);
		delete_buffer(buffer, false);
	};
	
	return B_OK;
}
#endif
