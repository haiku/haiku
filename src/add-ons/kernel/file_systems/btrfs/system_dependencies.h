#ifndef _SYSTEM_DEPENDENCIES_H
#define _SYSTEM_DEPENDENCIES_H


#ifdef FS_SHELL

// This needs to be included before the fs_shell wrapper
#include <zlib.h>
#include <new>
#include <util/AVLTree.h>

#include "fssh_api_wrapper.h"
#include "fssh_auto_deleter.h"

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
#include <zlib.h>

#endif	// !FS_SHELL


#endif	// _SYSTEM_DEPENDENCIES
