/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_FILE_CACHE_H
#define _KERNEL_FILE_CACHE_H


#include <vfs.h>
#include <vm_types.h>
#include <module.h>


// temporary/optional cache syscall API
#define CACHE_SYSCALLS "cache"

#define CACHE_CLEAR			1	// takes no parameters
#define CACHE_SET_MODULE	2	// gets the module name as parameter

#define CACHE_MODULES_NAME	"file_cache"

#define FILE_CACHE_SEQUENTIAL_ACCESS	0x01
#define FILE_CACHE_LOADED_COMPLETELY 	0x02
#define FILE_CACHE_NO_IO				0x04

struct cache_module_info {
	module_info	info;

	void (*node_opened)(void *vnode, int32 fdType, mount_id mountID, vnode_id parentID,
				vnode_id vnodeID, const char *name, off_t size);
	void (*node_closed)(void *vnode, int32 fdType, mount_id mountID, vnode_id vnodeID,
				int32 accessType);
	void (*node_launched)(size_t argCount, char * const *args);
};

#ifdef __cplusplus
extern "C" {
#endif

extern void cache_node_opened(void *vnode, int32 fdType, vm_cache_ref *cache,
				mount_id mountID, vnode_id parentID, vnode_id vnodeID, const char *name);
extern void cache_node_closed(void *vnode, int32 fdType, vm_cache_ref *cache,
				mount_id mountID, vnode_id vnodeID);
extern void cache_node_launched(size_t argCount, char * const *args);
extern void cache_prefetch_vnode(void *vnode, off_t offset, size_t size);
extern void cache_prefetch(mount_id mountID, vnode_id vnodeID, off_t offset, size_t size);
extern status_t file_cache_init_post_boot_device(void);
extern status_t file_cache_init(void);

extern vm_store *vm_create_vnode_store(void *vnode);

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_FILE_CACHE_H */
