#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iovec.h>

#include <KernelExport.h>
#include <OS.h>

#include "net_stack.h"
#include "memory_pool.h"
#include "atomizer.h"
#include "attribute.h"

#define DPRINTF printf

#define ATTRIBUTES_PER_POOL		(64)

static memory_pool *g_attributes_pool = NULL;
extern memory_pool_module_info *g_memory_pool;

#define DEFAULT_ATOMIZER (const void *) (-1)
atomizer_module_info *g_atomizer = NULL;

// Privates prototypes
// -------------------

// LET'S GO FOR IMPLEMENTATION
// ------------------------------------

const void * register_attribute_id(const char *name)
{
	status_t status;

	if (g_atomizer == NULL) {
		status = get_module(B_ATOMIZER_MODULE_NAME, (module_info **) &g_atomizer);
		if (status != B_OK) {
			dprintf("register_attribute_id(%s): Can't load " B_ATOMIZER_MODULE_NAME " module!\n", name);
			g_atomizer = (atomizer_module_info *) -1;
		};
	};

	if (g_atomizer == (atomizer_module_info *) -1)
		return NULL;

	return g_atomizer->atomize(DEFAULT_ATOMIZER, name, true);
}


net_attribute * new_attribute(net_attribute **in_list, const void *id)
{
	net_attribute *attr;
	
	if (! g_attributes_pool)
		g_attributes_pool = g_memory_pool->new_pool(sizeof(*attr), ATTRIBUTES_PER_POOL);
			
	if (! g_attributes_pool)
		return NULL;
	
	attr = (net_attribute *) g_memory_pool->new_pool_node(g_attributes_pool);
	if (! attr)
		return NULL;
		
	attr->id = id;
	attr->type = 0;
	
	if (in_list) {
		// prepend this attribut to the list
		attr->next = *in_list;
		*in_list = attr;
	} else
		attr->next = NULL;
	
	return attr;
	
}

status_t delete_attribute(net_attribute *attr, net_attribute **from_list)
{
	if (! attr)
		return B_BAD_VALUE;
		
	if (! g_attributes_pool)
		// Uh? From where come this net_attribute!?!
		return B_ERROR;
		
	// dprintf("delete_attribute(%p, %p)\n", attr, from_list);
		
	// Remove attribut from list, if any
	if (from_list) {
		if (*from_list == attr)
			*from_list = attr->next;
		else {
			net_attribute *tmp = *from_list;
			while (tmp) {
				if (tmp->next == attr)
					break;
				tmp = tmp->next;
			}
			if (tmp)
				tmp->next = attr->next;
		}
	}
		
	// Last, delete the net_buffer itself
	return g_memory_pool->delete_pool_node(g_attributes_pool, attr);
}


net_attribute * find_attribute(net_attribute *list, const void *id, int *type,
	void **value, size_t *size)
{
	net_attribute *attr;

	if (! list)
		return NULL;
		
	// dprintf("find_attribute(%p, %p)\n", list, id);
	attr = list;
	while (attr) {
		if (attr->id == id) {
			if (type)	*type = attr->type;
			if (size) 	*size = attr->size;
			if (value) {
				switch (attr->type & NET_ATTRIBUTE_TYPE_MASK) {
				case NET_ATTRIBUTE_BOOL: 	*value = &attr->u.boolean; break;
				case NET_ATTRIBUTE_BYTE:	*value = &attr->u.byte; break;
				case NET_ATTRIBUTE_INT16:	*value = &attr->u.word; break;
				case NET_ATTRIBUTE_INT32:	*value = &attr->u.dword; break;
				case NET_ATTRIBUTE_INT64:	*value = &attr->u.ddword; break;
				
				case NET_ATTRIBUTE_DATA:
				case NET_ATTRIBUTE_STRING:
					*value = &attr->u.data;
					break;
					
				case NET_ATTRIBUTE_POINTER:
					*value = attr->u.vec[0].iov_base;
					break;
					
				case NET_ATTRIBUTE_IOVEC:
					*value = &attr->u.vec;
					break;
				};
			};
			return attr;
		};
		attr = attr->next;
	};
	return NULL;
}

