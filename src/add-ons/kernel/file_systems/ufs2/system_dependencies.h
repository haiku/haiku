/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_DEPENDENCIES_H
#define _SYSTEM_DEPENDENCIES_H

#ifdef FS_SHELL
// This needs to be included before the fs_shell wrapper

#include "fssh_api_wrapper.h"
#include "fssh_auto_deleter.h"
//#include <new>

#else // !FS_SHELL

#include <AutoDeleter.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/SinglyLinkedList.h>
#include <util/Stack.h>

#include <ByteOrder.h>

#include <tracing.h>
#include <driver_settings.h>
#include <fs_attr.h>
#include <fs_cache.h>
#include <fs_index.h>
#include <fs_info.h>
#include <fs_interface.h>
#include <fs_query.h>
#include <fs_volume.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <NodeMonitor.h>
#include <SupportDefs.h>
#include <TypeConstants.h>
#endif // !FS_SHELL

#endif // _SYSTEM_DEPENDENCIES
