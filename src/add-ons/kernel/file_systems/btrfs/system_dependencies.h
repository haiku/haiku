/*
 * Copyright 2017, Chế Vũ Gia Hy, cvghy116@gmail.com.
 * This file may be used under the terms of the MIT License.
 */
#ifndef _SYSTEM_DEPENDENCIES_H
#define _SYSTEM_DEPENDENCIES_H


#ifdef FS_SHELL

#include "fssh_api_wrapper.h"
#include "fssh_auto_deleter.h"

#include <util/AVLTree.h>
#include <zlib.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef unsigned char uuid_t[16];

void uuid_generate(uuid_t out);
#ifdef __cplusplus
}
#endif


#else	// !FS_SHELL

#include <AutoDeleter.h>
#include <util/kernel_cpp.h>
#include <util/AutoLock.h>
#include <util/SinglyLinkedList.h>
#include <util/Stack.h>
#include <util/AVLTree.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ByteOrder.h>
#include <driver_settings.h>
#include <fs_cache.h>
#include <fs_interface.h>
#include <fs_info.h>
#include <fs_volume.h>
#include <KernelExport.h>
#include <io_requests.h>
#include <NodeMonitor.h>
#include <SupportDefs.h>
#include <lock.h>
#include <errno.h>
#include <new>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uuid.h>
#include <zlib.h>

#endif	// !FS_SHELL


#endif	// _SYSTEM_DEPENDENCIES
