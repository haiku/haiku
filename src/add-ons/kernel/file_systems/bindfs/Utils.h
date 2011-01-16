/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILS_H
#define UTILS_H


#include <AutoDeleter.h>

#include <vfs.h>


using BPrivate::AutoDeleter;


struct VnodePut
{
	inline void operator()(vnode* _vnode)
	{
		if (_vnode != NULL)
			vfs_put_vnode(_vnode);
	}
};

struct VnodePutter : AutoDeleter<vnode, VnodePut>
{
	VnodePutter() : AutoDeleter<vnode, VnodePut>() {}

	VnodePutter(vnode* _vnode) : AutoDeleter<vnode, VnodePut>(_vnode) {}
};


#endif	// UTILS_H
