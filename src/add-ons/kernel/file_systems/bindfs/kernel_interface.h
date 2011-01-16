/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_INTERFACE_H
#define KERNEL_INTERFACE_H


#include <fs_interface.h>


extern fs_volume_ops gBindFSVolumeOps;
extern fs_vnode_ops gBindFSVnodeOps;


#endif	// KERNEL_INTERFACE_H
