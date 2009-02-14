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

status_t	mount_overlay(fs_volume *volume, const char *device, uint32 flags,
				const char *args, ino_t *rootID);

#endif	// _VFS_OVERLAY_H
