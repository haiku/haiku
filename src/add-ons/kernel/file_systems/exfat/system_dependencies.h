/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _SYSTEM_DEPENDENCIES_H
#define _SYSTEM_DEPENDENCIES_H

#ifdef FS_SHELL
// This needs to be included before the fs_shell wrapper

#include "fssh_api_wrapper.h"
#include "fssh_auto_deleter.h"
#include "fssh_convertutf.h"

#include <util/SplayTree.h>

#else // !FS_SHELL

#include <AutoDeleter.h>
#include <util/AutoLock.h>
#include <util/convertutf.h>
#include <util/DoublyLinkedList.h>
#include <util/kernel_cpp.h>
#include <util/SinglyLinkedList.h>
#include <util/Stack.h>
#include <util/SplayTree.h>


#include <ByteOrder.h>
//#include <uuid.h>

#include <dirent.h>
#include <driver_settings.h>
#include "Debug.h"
#include <Drivers.h>
#include <errno.h>
#include <Errors.h>
#include <fs_attr.h>
#include <fs_cache.h>
#include <fs_index.h>
#include <fs_info.h>
#include <fs_interface.h>
#include <fs_query.h>
#include <fs_volume.h>
#include <io_requests.h>
#include <kernel.h>
#include <KernelExport.h>
#include <lock.h>
#include <new>
#include <NodeMonitor.h>
#include <real_time_clock.h>
#include <SupportDefs.h>
#include <StorageDefs.h>
#include <sys/stat.h>
#include <tracing.h>
#include <TypeConstants.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif // !FS_SHELL

#endif // _SYSTEM_DEPENDENCIES
