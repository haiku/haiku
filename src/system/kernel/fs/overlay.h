/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#ifndef _VFS_OVERLAY_H
#define _VFS_OVERLAY_H

#include <fs_interface.h>

status_t	create_overlay_vnode(fs_volume *superVolume, fs_vnode *vnode);

#endif	// _VFS_OVERLAY_H
