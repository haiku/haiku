/* layers_manager.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>

#include <KernelExport.h>
#include <OS.h>
#include <module.h>

#include "memory_pool.h"
#include "layers_manager.h"
#include "buffer.h"

struct layers_list {
	struct net_layer	*first;
	sem_id				lock;
};

static struct layers_list g_layers;

static memory_pool *	g_layers_pool = NULL;
#define LAYERS_PER_POOL		(64)

static net_layer * 	new_layer(const char *name, net_layer_module_info *module, void *cookie);
static status_t		delete_layer(net_layer *layer);

extern struct memory_pool_module_info *g_memory_pool;


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

			printf("layers_manager: loading %s layer(s) module\n", module_name);
			if (get_module(module_name, (module_info **) &nlmi) != B_OK)
				continue;
				
			// this module may have been acquire more time by calling register_layer()
			// or have B_KEEP_LOADED flag, so we don't care...
			put_module(module_name);
		};
		close_module_list(module_list);
	};

	// auto-start layer(s)
	acquire_sem(g_layers.lock);
	layer = g_layers.first;
	while (layer) {
		layer->module->init(layer);
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

	// disable all layers... 
	layer = g_layers.first;
	while (layer) {
		next = layer->next;
		module_name = layer->module->info.name;
		printf("layers_manager: uniniting '%s' layer\n", layer->name);
		// down the layer if currently up
		layer->module->enable(layer, false);
		layer->module->uninit(layer);

		delete_layer(layer);

		printf("layers_manager: unloading %s layer(s) module\n", module_name);
		put_module(module_name);

		layer = next;
	};

	puts("net layers manager stopped.");

	return B_OK;
}



// #pragma mark [Public functions]

// --------------------------------------------------
status_t register_layer(const char *name, net_layer_module_info *module, void *cookie, net_layer **_layer)
{
	status_t status;
	module_info *dummy;
	net_layer *layer;

	if (name == NULL ||
		strlen(name) <= 0 ||
		module == NULL)
		return B_ERROR;
	
	layer = new_layer(name, module, cookie);
	if (!layer)
		return B_NO_MEMORY;
	
	// acquire this module, to keep it loaded while this layer is registered
	status = get_module(module->info.name, &dummy);
	if (status != B_OK)
		goto error1;

	status = acquire_sem(g_layers.lock);
	if (status != B_OK)
		goto error2;
		
	layer->next = g_layers.first;
	g_layers.first = layer;
	
	release_sem(g_layers.lock);
		
	printf("layers_manager: '%s' layer registered\n", layer->name);
	
	if (_layer)
		*_layer = layer;
	return B_OK;

error2:
	put_module(module->info.name);
error1:
	return status;
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
			
		if (!p) {
			// Oh oh... :-(
			printf("layers_manager: unregister_layer(): %p layer not found in list!\n", layer);
			release_sem(g_layers.lock);
			return B_ERROR;
		};
			
		p->next = layer->next;
	};
	release_sem(g_layers.lock);

	layer->module->uninit(layer);

	printf("layers_manager: '%s' layer unregistered\n", layer->name);

	put_module(layer->module->info.name);

	return delete_layer(layer);
}


// --------------------------------------------------
net_layer * find_layer(const char *name)
{
	net_layer *layer;
	status_t status;
	
	if (!name)
		return NULL;

	status = acquire_sem(g_layers.lock);
	if (status != B_OK)
		return NULL;

	layer = g_layers.first;
	while (layer) {
		if (strcmp(layer->name, name) == 0)
			break;
		layer = layer->next;
	}
	release_sem(g_layers.lock);
	return layer;	
}


// --------------------------------------------------
status_t add_layer_attribut(net_layer *layer, const char *name, int type, ...)
{
	va_list args;
	
	va_start(args, type);

	switch(type) {
	case NET_ATTRIBUT_POINTER:
		// hack
		layer->cookie = va_arg(args, void *);
		break;
	}

	va_end(args);
	
	return B_OK;
}


// --------------------------------------------------
status_t remove_layer_attribut(net_layer *layer, const char *name)
{
	return B_ERROR;
}


status_t find_layer_attribut(net_layer *layer, const char *name, int *type, void **attribut, size_t *size)
{
	*attribut = &layer->cookie;
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


// --------------------------------------------------
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


// --------------------------------------------------
static net_layer * new_layer(const char *name, net_layer_module_info *module, void *cookie)
{
	net_layer *layer;

	if (! g_layers_pool)
		g_layers_pool = g_memory_pool->new_pool(sizeof(*layer), LAYERS_PER_POOL);
			
	if (! g_layers_pool)
		return NULL;
	
	layer = (net_layer *) g_memory_pool->new_pool_node(g_layers_pool);
	if (! layer)
		return NULL;

	layer->name = strdup(name);
	layer->module = module;
	layer->cookie = cookie;

	layer->layers_above = NULL;
	layer->layers_below = NULL;

	return layer;
}


// --------------------------------------------------
static status_t delete_layer(net_layer *layer)
{
	if (!layer)
		return B_ERROR;

	if (! g_layers_pool)
		// Uh? From where come this net_data!?!
		return B_ERROR;
		
	if (layer->name)
		free(layer->name);
	if (layer->layers_above)
		free(layer->layers_above);
	if (layer->layers_below)
		free(layer->layers_below);
		
	return g_memory_pool->delete_pool_node(g_layers_pool, layer);
}


