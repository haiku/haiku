/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef VNODE_STORE_H
#define VNODE_STORE_H


#include <vm.h>


struct vnode_store {
	vm_store	vm;
	void		*vnode;
	off_t		size;
		// the file system will maintain this field through the file cache API
};

#endif	/* VNODE_STORE_H */
