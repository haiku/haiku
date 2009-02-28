/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_HAIKU_VFS_H
#define USERLAND_FS_HAIKU_VFS_H

#include <fs_interface.h>


struct vnode;

extern "C" {


status_t vfs_get_file_map(struct vnode *vnode, off_t offset, size_t size,
	struct file_io_vec *vecs, size_t *_count);
status_t vfs_lookup_vnode(dev_t mountID, ino_t vnodeID, struct vnode **_vnode);


}	// extern "C"

#endif	// USERLAND_FS_HAIKU_VFS_H
