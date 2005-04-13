/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VNODE_STORE_H
#define VNODE_STORE_H


#include <vm.h>


struct vnode_store {
	vm_store	vm;
	void		*vnode;
	void		*file_cache_ref;
};

#endif	/* VNODE_STORE_H */
