/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VNODE_STORE_H
#define VNODE_STORE_H


#include <vm_types.h>


struct vnode_store {
	vm_store		vm;
	struct vnode*	vnode;
	void*			file_cache_ref;
};

#endif	/* VNODE_STORE_H */
