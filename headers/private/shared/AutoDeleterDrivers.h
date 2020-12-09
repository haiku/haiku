/*
 * Copyright 2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AUTO_DELETER_DRIVERS_H
#define _AUTO_DELETER_DRIVERS_H


#include <AutoDeleter.h>
#include <driver_settings.h>
#if defined(_KERNEL_MODE) && !defined(_BOOT_MODE)
#include <vfs.h>
#include <fs/fd.h>
#endif


namespace BPrivate {


typedef CObjectDeleter<void, status_t, unload_driver_settings> DriverSettingsUnloader;

#if defined(_KERNEL_MODE) && !defined(_BOOT_MODE)

typedef CObjectDeleter<struct vnode, void, vfs_put_vnode> VnodePutter;
typedef CObjectDeleter<file_descriptor, void, put_fd> DescriptorPutter;

#endif


}


using ::BPrivate::DriverSettingsUnloader;

#if defined(_KERNEL_MODE) && !defined(_BOOT_MODE)

using ::BPrivate::VnodePutter;
using ::BPrivate::DescriptorPutter;

#endif


#endif	// _AUTO_DELETER_DRIVERS_H
