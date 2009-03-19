/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_FUSE_FUSE_FS_H
#define USERLAND_FS_FUSE_FUSE_FS_H

#include "fuse_api.h"


struct fuse_fs {
	void*			userData;
	fuse_operations	ops;
};


#endif	// USERLAND_FS_FUSE_FUSE_FS_H
