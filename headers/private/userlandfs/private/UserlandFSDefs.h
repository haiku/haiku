/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_USERLAND_FS_DEFS_H
#define USERLAND_FS_USERLAND_FS_DEFS_H

#include "FSCapabilities.h"
#include "Port.h"


namespace UserlandFSUtil {

struct fs_init_info {
	FSCapabilities	capabilities;
	client_fs_type	clientFSType;
	int32			portInfoCount;
	Port::Info		portInfos[0];
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::fs_init_info;

#endif	// USERLAND_FS_USERLAND_FS_DEFS_H
