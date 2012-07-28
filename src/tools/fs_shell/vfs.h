/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _FSSH_VFS_H
#define _FSSH_VFS_H


#include "fssh_fs_interface.h"
#include "fssh_lock.h"
#include "list.h"


namespace FSShell {


/* R5 figures, but we don't use a table for monitors anyway */
#define DEFAULT_FD_TABLE_SIZE	128
#define MAX_FD_TABLE_SIZE		8192
#define DEFAULT_NODE_MONITORS	4096
#define MAX_NODE_MONITORS		65536

struct kernel_args;
struct vm_cache_ref;
struct file_descriptor;


/** The I/O context of a process/team, holds the fd array among others */
typedef struct io_context {
	struct vnode 			*cwd;
	fssh_mutex				io_mutex;
	uint32_t				table_size;
	uint32_t				num_used_fds;
	struct file_descriptor	**fds;
	uint8_t					*fds_close_on_exec;
} io_context;

struct fd_info {
	int			number;
	int32_t		open_mode;
	fssh_dev_t	device;
	fssh_ino_t	node;
};

struct vnode;

/* macro to allocate a iovec array on the stack */
#define IOVECS(name, size) \
	uint8_t _##name[sizeof(fssh_iovecs) + (size)*sizeof(fssh_iovec)]; \
	fssh_iovecs *name = (fssh_iovecs *)_##name


fssh_status_t	vfs_init(struct kernel_args *args);
fssh_status_t	vfs_bootstrap_file_systems(void);
void			vfs_mount_boot_file_system(struct kernel_args *args);
void			vfs_exec_io_context(void *context);
void*			vfs_new_io_context(void *parentContext);
fssh_status_t	vfs_free_io_context(void *context);

/* calls needed by the VM for paging and by the file cache */
int				vfs_get_vnode_from_fd(int fd, bool kernel, void **vnode);
fssh_status_t	vfs_get_vnode_from_path(const char *path, bool kernel, void **vnode);
fssh_status_t	vfs_get_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID,
					void **_vnode);
fssh_status_t	vfs_entry_ref_to_vnode(fssh_mount_id mountID,
					fssh_vnode_id directoryID, const char *name, void **_vnode);
void			vfs_vnode_to_node_ref(void *_vnode, fssh_mount_id *_mountID,
					fssh_vnode_id *_vnodeID);

fssh_status_t	vfs_lookup_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID,
					struct vnode **_vnode);
void			vfs_put_vnode(void *vnode);
void			vfs_acquire_vnode(void *vnode);
fssh_status_t	vfs_get_cookie_from_fd(int fd, void **_cookie);
fssh_status_t	vfs_read_pages(void *vnode, void *cookie, fssh_off_t pos,
					const fssh_iovec *vecs, fssh_size_t count,
					fssh_size_t *_numBytes);
fssh_status_t	vfs_write_pages(void *vnode, void *cookie,
					fssh_off_t pos, const fssh_iovec *vecs, fssh_size_t count,
					fssh_size_t *_numBytes);
fssh_status_t	vfs_get_file_map(void *_vnode, fssh_off_t offset,
					fssh_size_t size, fssh_file_io_vec *vecs,
					fssh_size_t *_count);
fssh_status_t	vfs_get_fs_node_from_path(fssh_mount_id mountID,
					const char *path, bool kernel, void **_node);
fssh_status_t	vfs_stat_vnode(void *_vnode, struct fssh_stat *stat);
fssh_status_t	vfs_get_vnode_name(void *vnode, char *name,
					fssh_size_t nameSize);
fssh_status_t	vfs_get_cwd(fssh_mount_id *_mountID, fssh_vnode_id *_vnodeID);
void			vfs_unlock_vnode_if_locked(struct file_descriptor *descriptor);
fssh_status_t	vfs_disconnect_vnode(fssh_mount_id mountID,
					fssh_vnode_id vnodeID);
void			vfs_free_unused_vnodes(int32_t level);

/* special module convenience call */
fssh_status_t	vfs_get_module_path(const char *basePath,
					const char *moduleName, char *pathBuffer,
					fssh_size_t bufferSize);

/* service call for whoever needs a normalized path */
fssh_status_t	vfs_normalize_path(const char *path, char *buffer,
					fssh_size_t bufferSize, bool kernel);

/* service call for the node monitor */
fssh_status_t	resolve_mount_point_to_volume_root(fssh_mount_id mountID,
					fssh_vnode_id nodeID, fssh_mount_id *resolvedMountID,
					fssh_vnode_id *resolvedNodeID);

// cache initialization functions defined in the respective cache implementation
extern fssh_status_t block_cache_init();
extern fssh_status_t file_cache_init();

}	// namespace FSShell


#endif	/* _FSSH_VFS_H */
