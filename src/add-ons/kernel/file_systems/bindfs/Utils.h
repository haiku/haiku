/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILS_H
#define UTILS_H


#include <AutoDeleter.h>

#include <vfs.h>


typedef CObjectDeleter<vnode, void, vfs_put_vnode> VnodePutter;


#endif	// UTILS_H
