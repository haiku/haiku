/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_DEPENDENCIES_H
#define _SYSTEM_DEPENDENCIES_H


#ifdef FS_SHELL

#include "fssh_api_wrapper.h"
#include "fssh_auto_deleter.h"

#else	// !FS_SHELL

#include <AutoDeleter.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/SinglyLinkedList.h>
#include <util/Stack.h>

#include <ByteOrder.h>

#ifndef _BOOT_MODE
#	include <tracing.h>

#	include <driver_settings.h>
#	include <fs_attr.h>
#	include <fs_cache.h>
#	include <fs_index.h>
#	include <fs_info.h>
#	include <fs_interface.h>
#	include <fs_query.h>
#	include <fs_volume.h>
#	include <Drivers.h>
#	include <KernelExport.h>
#	include <NodeMonitor.h>
#	include <SupportDefs.h>
#	include <TypeConstants.h>
#endif	// _BOOT_MODE

#include <ctype.h>
#include <errno.h>
#include <new>
#include <null.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#endif	// !FS_SHELL


#endif	// _SYSTEM_DEPENDENCIES_H
