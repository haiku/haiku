#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iovec.h>
#include <stdarg.h>

#include <KernelExport.h>
#include <OS.h>

#include "net_stack.h"
#include "memory_pool.h"
#include "attribute.h"

#define DPRINTF printf

#define ATTRIBUTES_PER_POOL		(64)

static memory_pool *g_attributes_pool = NULL;
extern memory_pool_module_info *g_memory_pool;

// Privates prototypes
// -------------------

// LET'S GO FOR IMPLEMENTATION
// ------------------------------------

status_t add_attribute(net_attribute **in_list, string_token id, int type, va_list args)
{
	net_attribute *attr;
	
	attr = new_attribute(in_list, id);
	if (! attr)
		return B_NO_MEMORY;
		
	attr->type = type;
	switch(type & NET_ATTRIBUTE_TYPE_MASK) {
	case NET_ATTRIBUTE_BOOL:
		attr->u.boolean = va_arg(args, bool);
		break;
					
	case NET_ATTRIBUTE_BYTE:
		attr->u.byte = va_arg(args, uint8);
		break;
					
	case NET_ATTRIBUTE_INT16:
		attr->u.word = va_arg(args, uint16);
		break;
					
	case NET_ATTRIBUTE_INT32:
		attr->u.dword = va_arg(args, uint32);
		break;
					
	case NET_ATTRIBUTE_INT64:
		attr->u.ddword = va_arg(args, uint64);
		break;
				
	case NET_ATTRIBUTE_DATA:
		{ 	// void *data, size_t data_len
		void *data = va_arg(args, void *);
		size_t sz = va_arg(args, size_t);

		if (sz > MAX_ATTRIBUTE_DATA_LEN)
			sz = MAX_ATTRIBUTE_DATA_LEN;
		
		memcpy(attr->u.data, data, sz);
		break;
		}

	case NET_ATTRIBUTE_STRING:
		strncpy(attr->u.data, va_arg(args, char *), MAX_ATTRIBUTE_DATA_LEN);
		break;
					
	case NET_ATTRIBUTE_POINTER:
		// NB: data behind pointer ARE NOT copied
		attr->u.ptr = va_arg(args, void *);
		break;
					
	case NET_ATTRIBUTE_IOVEC:
		{ 	// int nb, iovec *vec
		int nb 		= va_arg(args, int);
		iovec * vec = va_arg(args, iovec *);
		if (nb > MAX_ATTRIBUTE_IOVECS)
			nb = MAX_ATTRIBUTE_IOVECS;
		
		memcpy(&attr->u.vec, vec, nb * sizeof(iovec));
		break;
		}
	}
	
	return B_OK;
}


net_attribute * new_attribute(net_attribute **in_list, string_token id)
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


net_attribute * find_attribute(net_attribute *list, string_token id, int index,
	int *type, void **value, size_t *size)
{
	net_attribute *attr;

	if (! list)
		return NULL;
		
	// dprintf("find_attribute(%p, %p)\n", list, id);
	attr = list;
	while (attr) {
		if (attr->id == id && index-- == 0) {
			if (type)	*type = attr->type;
			if (size) 	*size = attr->size;
			if (value) {
				switch (attr->type & NET_ATTRIBUTE_TYPE_MASK) {
				case NET_ATTRIBUTE_BOOL:
					*value = &attr->u.boolean;
					break;
					
				case NET_ATTRIBUTE_BYTE:
					*value = &attr->u.byte;
					break;
					
				case NET_ATTRIBUTE_INT16:
					*value = &attr->u.word;
					break;
					
				case NET_ATTRIBUTE_INT32:
					*value = &attr->u.dword;
					break;
					
				case NET_ATTRIBUTE_INT64:
					*value = &attr->u.ddword;
					break;
				
				case NET_ATTRIBUTE_DATA:
				case NET_ATTRIBUTE_STRING:
					*value = &attr->u.data;
					break;
					
				case NET_ATTRIBUTE_POINTER:
					*value = attr->u.ptr;
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

