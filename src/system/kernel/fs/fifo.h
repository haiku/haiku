/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VFS_FIFO_H
#define _VFS_FIFO_H

#include <fs_interface.h>


status_t	create_fifo_vnode(fs_volume* superVolume, fs_vnode* vnode);


#endif	// _VFS_FIFO_H
