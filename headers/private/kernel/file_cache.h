/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_FILE_CACHE_H
#define _KERNEL_FILE_CACHE_H


#include <vfs.h>
#include <vm_types.h>
#include <module.h>


struct cache_module_info {
	module_info	info;

	void (*node_opened)(mount_id mountID, vnode_id vnodeID, off_t size);
	void (*node_closed)(mount_id mountID, vnode_id vnodeID);
};

#ifdef __cplusplus
extern "C" {
#endif

extern void cache_node_opened(vm_cache_ref *cache, mount_id mountID, vnode_id vnodeID);
extern void cache_node_closed(vm_cache_ref *cache, mount_id mountID, vnode_id vnodeID);
extern void cache_prefetch(mount_id mountID, vnode_id vnodeID);
extern status_t file_cache_init(void);

extern vm_store *vm_create_vnode_store(void *vnode);

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_FILE_CACHE_H */
