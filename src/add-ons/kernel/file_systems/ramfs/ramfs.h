/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef RAM_FS_H
#define RAM_FS_H

#include <SupportDefs.h>

extern struct fs_vnode_ops gRamFSVnodeOps;
extern struct fs_volume_ops gRamFSVolumeOps;
const size_t kMaxIndexKeyLength = 256;

#endif	// RAM_FS_H
