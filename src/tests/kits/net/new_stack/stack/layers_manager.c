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
#include "attribute.h"
#include "stack.h"	// for string_{to|for}_token()

struct layers_list {
	struct net_layer	*first;
	sem_id				lock;
};

static struct layers_list g_layers;

static memory_pool *	g_layers_pool = NULL;
#define LAYERS_PER_POOL		(64)

static net_layer * 	new_layer(const char *name, const char *type, int priority,
	net_layer_module_info *module, void *cookie);
static status_t		delete_layer(net_layer *layer);

extern memory_pool_module_info *g_memory_pool;

string_token  		g_joker_token = 0;
string_token		g_layers_trace_attr = 0;

// --------------------------------------------------
status_t start_layers_manager(void)
{
	void 		*module_list;
	net_layer 	*layer;

	g_joker_token = string_to_token("*");
	g_layers_trace_attr = string_to_token("layers_trace");

	g_layers.first = NULL;
	
	g_layers.lock = create_sem(1, "net_layers list lock");
	if (g_layers.lock < B_OK)
		return B_ERROR;

#ifdef _KERNEL_MODE
	set_sem_owner(g_layers.lock, B_SYSTEM_TEAM);
#endif

	// Load all network/* modules and let them
	// register any layer they may support by calling init()
	module_list = open_module_list(NET_MODULES_ROOT);
	if (module_list) {
		size_t sz;
		char module_name[256];
		struct net_layer_module_info *nlmi;
		
		sz = sizeof(module_name);
		while(read_next_module_name(module_list, module_name, &sz) == B_OK) {
			sz = sizeof(module_name);
			if (!strlen(module_name))
				continue;

			if (strcmp(module_name, NET_STACK_MODULE_NAME) == 0)
				continue;	// skip ourself! ;-)

			dprintf("layers_manager: loading %s layer(s) module\n", module_name);
			if (get_module(module_name, (module_info **) &nlmi) != B_OK)
				continue;
				
			// this module may have been acquire more time if he called register_layer(),
			// or have a B_KEEP_LOADED flag, so we don't care...
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
status_t stop_layers_manager(void)
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
		dprintf("layers_manager: uniniting '%s' layer\n", layer->name);
		// down the layer if currently up
		layer->module->enable(layer, false);
		layer->module->uninit(layer);

		delete_layer(layer);

		dprintf("layers_manager: unloading %s layer(s) module\n", module_name);
		put_module(module_name);

		layer = next;
	};

	puts("net layers manager stopped.");

	return B_OK;
}



// #pragma mark -

// --------------------------------------------------
status_t register_layer(const char *name, const char *type, int priority,
	net_layer_module_info *module, void *cookie, net_layer **_layer)
{
	status_t status;
	module_info *dummy;
	net_layer *layer;

	if (name == NULL ||
		strlen(name) <= 0 ||
		module == NULL)
		return B_ERROR;
	
	layer = new_layer(name, type, priority, module, cookie);
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
		
	dprintf("layers_manager: '%s' layer, registering '%s/%s' type\n", layer->name, 
		string_for_token(layer->type), string_for_token(layer->sub_type));
	
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
			dprintf("layers_manager: unregister_layer(): %p layer not found in list!\n", layer);
			release_sem(g_layers.lock);
			return B_ERROR;
		};
			
		p->next = layer->next;
	};
	release_sem(g_layers.lock);

	layer->module->uninit(layer);

	dprintf("layers_manager: '%s' layer unregistered\n", layer->name);

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

// #pragma mark -

// --------------------------------------------------
status_t add_layer_attribute(net_layer *layer, const void *id, int type, ...)
{
	status_t status;
	va_list args;
	
	va_start(args, type);
	status = add_attribute(&layer->attributes, id, type, args);
	va_end(args);
	
	return status;
}


// --------------------------------------------------
status_t remove_layer_attribute(net_layer *layer, const void *id, int index)
{
	net_attribute *attr;

	if (! layer)
		return B_BAD_VALUE;
	
	attr = find_attribute(layer->attributes, id, index, NULL, NULL, NULL);
	if (! attr)
		return B_NAME_NOT_FOUND;

	return delete_attribute(attr, &layer->attributes);
}

status_t find_layer_attribute(net_layer *layer, const void *id, int index, int *type,
	void **value, size_t *size)
{
	net_attribute *attr;

	if (! layer)
		return B_BAD_VALUE;
	
	attr = find_attribute(layer->attributes, id, index, type, value, size);
	if (! attr)
		return B_NAME_NOT_FOUND;

	return B_OK;
}

// #pragma mark -

// --------------------------------------------------
status_t send_layers_up(net_layer *layer, net_buffer *buffer)
{
	net_layer *above;
	status_t status;
	// int i;

	if (!layer)
		return B_ERROR;

	add_buffer_attribute(buffer, g_layers_trace_attr, NET_ATTRIBUTE_STRING, layer->name),

	status = acquire_sem(g_layers.lock);
	if (status != B_OK)
		return status;

	status = B_ERROR;
	// TODO: lookup thru a previoulsy (at each layer [un]registration time) built list of
	// "above" layers of caller layer.

	// dprintf("layers_manager: send_layers_up(): layer %s (%x/%x), searching a matching upper layer...\n",
	//	layer->name, layer->type, layer->sub_type);

	// HACK: today, we lookup thru ALL :-( registered layers.
	// In the future, such layers (de)assembly should be done at layer (un)resgistration time, as
	// it never change until layers list change herself...
	above = g_layers.first;
	while (above) {
		// dprintf("layers_manager: send_layers_up: Matching against layer %s (%x/%x) ?\n",
		// 	above->name, above->type, above->sub_type);
		
		if ( above->module->process_input &&
			 ( (above->type == g_joker_token) || (above->type == layer->sub_type) ) ) {
/*
			dprintf("layers_manager: send_layers_up: handing buffer from %x/%x (%s) to %x/%x (%s)...\n",
					layer->type, layer->sub_type, layer->name,
					above->type, above->sub_type, above->name);
*/		
			release_sem(g_layers.lock);
			status = above->module->process_input(above, buffer);
			if (status == B_OK) {
				atomic_add(&above->use_count, 1);
				// TODO: resort this layer with previous one, if required.								
				return status;
			};

			status = acquire_sem(g_layers.lock);
			if (status != B_OK)
				return status;
		};  
		above = above->next;
	}
	release_sem(g_layers.lock);
	return status;
}


// --------------------------------------------------
status_t send_layers_down(net_layer *layer, net_buffer *buffer)
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

	add_buffer_attribute(buffer, g_layers_trace_attr, NET_ATTRIBUTE_STRING, layer->name),
	
	status = B_ERROR;
	i = 0;
	while (layer->layers_below[i]) {
		below = layer->layers_below[i++];
		if (!below->module->process_output)
			// this layer don't process output buffer
			continue;
			
		status = below->module->process_output(below, buffer);
		if (status == B_OK) {
			atomic_add(&below->use_count, 1);
			// TODO: resort this layer with previous one, if required.								
			return status;
		};
	};

	return status;
}

// #pragma mark -

// --------------------------------------------------
static net_layer * new_layer(const char *name, const char *type, int priority,
	net_layer_module_info *module, void *cookie)
{
	net_layer *layer;

	if (! g_layers_pool)
		g_layers_pool = g_memory_pool->new_pool(sizeof(*layer), LAYERS_PER_POOL);
			
	if (! g_layers_pool)
		return NULL;
	
	layer = (net_layer *) g_memory_pool->new_pool_node(g_layers_pool);
	if (! layer)
		return NULL;

	layer->name 	= strdup(name);

	if (type) {
		char *tmp, *t, *n;
		
		tmp = strdup(type);
		t = strtok_r(tmp, "/", &n);
		layer->type = (t ? string_to_token(t) : g_joker_token);
		t =  strtok_r(NULL, " ", &n);
		layer->sub_type = (t ? string_to_token(t) : g_joker_token);
		free(tmp);
	} else
		layer->type = layer->sub_type = 0;


	layer->priority = priority; 
	layer->module 	= module;
	layer->cookie 	= cookie;

	layer->layers_above = NULL;
	layer->layers_below = NULL;
	
	layer->attributes = NULL;

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

	if (layer->attributes) {
		// Free layer attributes

		net_attribute *attr;
		net_attribute *next_attr;

		attr = layer->attributes;
		while (attr) {
			next_attr = attr->next;
			delete_attribute(attr, NULL);
			attr = next_attr;
		};
	};
	
	return g_memory_pool->delete_pool_node(g_layers_pool, layer);
}


