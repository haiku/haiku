/*
** Copyright 2002-04, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/*
	Part of PnP Manager
	Node attributes handling.
*/


#include "device_manager_private.h"
#include "dl_list.h"
#include <pnp_bus.h>

#include <TypeConstants.h>

#include <stdlib.h>
#include <string.h>


static bool is_fixed_attribute(const char *name);


status_t
pnp_get_attr_uint8(pnp_node_handle node, const char *name,
	uint8 *value, bool recursive)
{
	status_t status;

	benaphore_lock(&gNodeLock);
	status = pnp_get_attr_uint8_nolock(node, name, value, recursive);
	benaphore_unlock(&gNodeLock);

	return status;
}


status_t
pnp_get_attr_uint8_nolock(pnp_node_handle node, const char *name,
	uint8 *value, bool recursive)
{
	pnp_node_attr_info *attr = pnp_find_attr_nolock(node, name, recursive, B_UINT8_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*value = attr->attr.value.ui8;
	return B_OK;
}


status_t
pnp_get_attr_uint16(pnp_node_handle node, const char *name,
	uint16 *value, bool recursive)
{
	status_t status;

	benaphore_lock(&gNodeLock);
	status = pnp_get_attr_uint16_nolock(node, name, value, recursive);
	benaphore_unlock(&gNodeLock);

	return status;
}


status_t
pnp_get_attr_uint16_nolock(pnp_node_handle node, const char *name,
	uint16 *value, bool recursive)
{
	pnp_node_attr_info *attr = pnp_find_attr_nolock(node, name, recursive, B_UINT16_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*value = attr->attr.value.ui16;
	return B_OK;
}


status_t
pnp_get_attr_uint32(pnp_node_handle node, const char *name,
	uint32 *value, bool recursive)
{
	status_t status;

	benaphore_lock(&gNodeLock);
	status = pnp_get_attr_uint32_nolock(node, name, value, recursive);
	benaphore_unlock(&gNodeLock);

	return status;
}


status_t
pnp_get_attr_uint32_nolock(pnp_node_handle node, const char *name,
	uint32 *value, bool recursive)
{
	pnp_node_attr_info *attr = pnp_find_attr_nolock(node, name, recursive, B_UINT32_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*value = attr->attr.value.ui32;
	return B_OK;
}


status_t
pnp_get_attr_uint64(pnp_node_handle node, const char *name,
	uint64 *value, bool recursive)
{
	status_t status;

	benaphore_lock(&gNodeLock);
	status = pnp_get_attr_uint64_nolock(node, name, value, recursive);
	benaphore_unlock(&gNodeLock);

	return status;
}


status_t
pnp_get_attr_uint64_nolock(pnp_node_handle node, const char *name, 
	uint64 *value, bool recursive)
{
	pnp_node_attr_info *attr = pnp_find_attr_nolock(node, name, recursive, B_UINT64_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*value = attr->attr.value.ui64;
	return B_OK;
}


status_t
pnp_get_attr_string(pnp_node_handle node, const char *name,
	char **value, bool recursive)
{
	status_t status;
	const char *orig_value;

	benaphore_lock(&gNodeLock);

	status = pnp_get_attr_string_nolock(node, name, &orig_value, recursive);
	if (status == B_OK) {
		*value = strdup(orig_value);
		if (orig_value != NULL && *value == NULL)
			status = B_NO_MEMORY;
	}

	benaphore_unlock(&gNodeLock);

	return status;
}


status_t
pnp_get_attr_string_nolock(pnp_node_handle node, const char *name,
	const char **value, bool recursive)
{
	pnp_node_attr_info *attr = pnp_find_attr_nolock(node, name, recursive, B_STRING_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*value = attr->attr.value.string;
	return B_OK;
}


status_t
pnp_get_attr_raw(pnp_node_handle node, const char *name,
	void **data, size_t *len, bool recursive)
{
	const void *orig_data;
	status_t status;

	benaphore_lock(&gNodeLock);

	status = pnp_get_attr_raw_nolock( node, name, &orig_data, len, recursive );

	if (status == B_OK) {
		void *tmp_data = malloc(*len);
		if (tmp_data != NULL) {
			memcpy(tmp_data, orig_data, *len);
			*data = tmp_data;
		} else
			status = B_NO_MEMORY;
	}

	benaphore_unlock(&gNodeLock);

	return status;
}


status_t
pnp_get_attr_raw_nolock(pnp_node_handle node, const char *name,
	const void **data, size_t *len, bool recursive)
{
	pnp_node_attr_info *attr = pnp_find_attr_nolock(node, name, recursive, B_RAW_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*data = attr->attr.value.raw.data;
	*len = attr->attr.value.raw.len;
	return B_OK;
}


// free node attribute

void
pnp_free_node_attr(pnp_node_attr_info *attr)
{
	free((char *)attr->attr.name);

	switch (attr->attr.type) {
		case B_STRING_TYPE:
			free((char *)attr->attr.value.string);
			break;
		case B_RAW_TYPE:
			free(attr->attr.value.raw.data);
			break;
	}

	free(attr);
}


// duplicate node attribute

status_t
pnp_duplicate_node_attr(const pnp_node_attr *src, pnp_node_attr_info **dest_out)
{
	pnp_node_attr_info *dest;
	status_t res;

	dest = malloc(sizeof(*dest));
	if (dest == NULL)
		return B_NO_MEMORY;

	memset(dest, 0, sizeof(*dest));
	dest->ref_count = 1;
	dest->attr.name = strdup(src->name);
	dest->attr.type = src->type;

	switch (src->type) {
		case B_UINT8_TYPE:
		case B_UINT16_TYPE:
		case B_UINT32_TYPE:
		case B_UINT64_TYPE:
			dest->attr.value.ui64 = src->value.ui64;
			break;

		case B_STRING_TYPE:
			dest->attr.value.string = strdup(src->value.string);
			if (dest->attr.value.string == NULL) {
				res = B_NO_MEMORY;
				goto err;
			}
			break;

		case B_RAW_TYPE:
			dest->attr.value.raw.data = malloc(src->value.raw.len);
			if (dest->attr.value.raw.data == NULL) {
				res = B_NO_MEMORY;
				goto err;
			}

			dest->attr.value.raw.len = src->value.raw.len;
			memcpy(dest->attr.value.raw.data, src->value.raw.data, 
				src->value.raw.len);
			break;

		default:
			res = B_BAD_VALUE;
			goto err;
	}

	*dest_out = dest;
	return B_OK;

err:
	free(dest);
	return res;
}


// public: get first/next attribute of node

status_t
pnp_get_next_attr(pnp_node_handle node, pnp_node_attr_handle *attr)
{
	pnp_node_attr_info *next;

	benaphore_lock(&gNodeLock);
	
	if (*attr != NULL) {
		// next attribute
		next = (*attr)->next;
		// implicit release of previous attribute
		if (--(*attr)->ref_count == 0)
			pnp_free_node_attr(*attr);
	} else {
		// first attribute
		next = node->attributes;
	}

	++next->ref_count;
	*attr = next;

	benaphore_unlock(&gNodeLock);
	return B_OK;
}


// public: release attribute of node explicitely

status_t
pnp_release_attr(pnp_node_handle node,pnp_node_attr_handle attr)
{
	benaphore_lock(&gNodeLock);

	if (--attr->ref_count == 0)
		pnp_free_node_attr(attr);

	benaphore_unlock(&gNodeLock);	
	return B_OK;
}


// public: retrieve data of attribute

status_t
pnp_retrieve_attr(pnp_node_attr_handle attr, const pnp_node_attr **attr_content)
{
	*attr_content = &attr->attr;
	return B_OK;
}


// remove attribute from node, freeing it if not in use
// node_lock must be hold

void
pnp_remove_attr_int(pnp_node_handle node, pnp_node_attr_info *attr)
{
	REMOVE_DL_LIST(attr, node->attributes, );

	if (--attr->ref_count == 0)
		pnp_free_node_attr(attr);
}


// public: remove attribute from node

status_t
pnp_remove_attr(pnp_node_handle node, const char *name)
{
	pnp_node_attr_info *attr;

	// don't remove holy attributes
	if (is_fixed_attribute(name))
		return B_NOT_ALLOWED;

	benaphore_lock(&gNodeLock);

	// find attribute in list
	attr = pnp_find_attr_nolock(node, name, false, B_ANY_TYPE);
	if (attr != NULL) {
		// and remove it from it
		pnp_remove_attr_int(node, attr);
	}

	benaphore_unlock(&gNodeLock);

	return attr ? B_OK : B_ENTRY_NOT_FOUND;
}


// public: modify attribute's content

status_t
pnp_write_attr(pnp_node_handle node, const pnp_node_attr *attr)
{
	pnp_node_attr_info *old_attr, *new_attr;
	status_t res;

	// don't touch holy attributes
	if (is_fixed_attribute(attr->name))
		return B_NOT_ALLOWED;

	res = pnp_duplicate_node_attr(attr, &new_attr);
	if (res != B_OK)
		return res;

	benaphore_lock(&gNodeLock);

	// check whether it's an add or modify		
	for (old_attr = node->attributes; old_attr != NULL; old_attr = old_attr->next) {
		if (!strcmp(attr->name, old_attr->attr.name))
			break;
	}

	if (old_attr != NULL) {
		// it's a modify
		// we first insert new attribute after old one...
		// (so it appears at same position in attribute list)
		new_attr->prev = old_attr;
		new_attr->next = old_attr->next;

		if (old_attr->next == NULL)
			old_attr->next->prev = new_attr;

		old_attr->next = new_attr;

		// ...and get rid of the old one
		REMOVE_DL_LIST(old_attr, node->attributes, );

		if (--old_attr->ref_count == 0)
			pnp_free_node_attr(old_attr);
	} else {
		// it's an add
		// append to end of list;
		// this is really something to optimize
		if (node->attributes == NULL) {
			ADD_DL_LIST_HEAD(new_attr, node->attributes, );
		} else {
			pnp_node_attr_info *last_attr;

			for (last_attr = node->attributes; last_attr->next != NULL; last_attr = last_attr->next)
				;

			last_attr->next = new_attr;
			new_attr->prev = last_attr;
			new_attr->next = NULL;
		}
	}

	benaphore_unlock(&gNodeLock);
	return B_OK;
}


// check whether attribute cannot be modified;
// returns true, if that's forbidden

static bool
is_fixed_attribute(const char *name)
{
	static const char *forbidden_list[] = {
		PNP_DRIVER_DRIVER,			// never change driver
		PNP_DRIVER_TYPE,			// never change type
		PNP_BUS_IS_BUS,				// never switch between bus/not bus mode
		PNP_DRIVER_ALWAYS_LOADED	// don't confuse us by changing auto-load
	};
	uint32 i;

	// some attributes are to important to get modified	or removed directly
	for (i = 0; i < sizeof(forbidden_list) / sizeof(forbidden_list[0]); ++i) {
		if (!strcmp(name, forbidden_list[i]))
			return true;
	}

	return false;
}


// same as pnp_find_attr(), but node_lock must be hold and is never released

pnp_node_attr_info *
pnp_find_attr_nolock(pnp_node_handle node, const char *name,
	bool recursive, type_code type )
{
	do {
		pnp_node_attr_info *attr;

		for (attr = node->attributes; attr != NULL; attr = attr->next) {
			if (type != B_ANY_TYPE && attr->attr.type != type)
				continue;

			if (!strcmp(attr->attr.name, name))
				return attr;
		}

		node = node->parent;
	} while (node != NULL && recursive);

	return NULL;
}

