/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _SYSTEM_DEPENDENCIES_H_
#define _SYSTEM_DEPENDENCIES_H_

#ifdef FS_SHELL
// This needs to be included before the fs_shell wrapper

#include "fssh_api_wrapper.h"
#include "fssh_auto_deleter.h"
#include "Debug.h"

#ifdef __cplusplus
extern "C"
{
#endif
	typedef unsigned char uuid_t[16];

	void uuid_generate(uuid_t out);
#ifdef __cplusplus
}
#endif

#else // !FS_SHELL

#include <AutoDeleter.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/SinglyLinkedList.h>
#include <util/Stack.h>

#include <ByteOrder.h>
#include <uuid.h>

#include <tracing.h>
#include <driver_settings.h>
#include <fs_attr.h>
#include <fs_cache.h>
#include <fs_index.h>
#include <fs_info.h>
#include <fs_interface.h>
#include <fs_query.h>
#include <fs_volume.h>
#include "Debug.h"
#include <Drivers.h>
#include <KernelExport.h>
#include <NodeMonitor.h>
#include <SupportDefs.h>
#include <TypeConstants.h>
#include <sys/stat.h>
#endif // !FS_SHELL

#endif // _SYSTEM_DEPENDENCIES
