/*
 * Copyright 2018 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _KERNEL_VM_UTILS_H
#define _KERNEL_VM_UTILS_H


#include <disk_device_manager/KPartition.h>
#include <fs/KPath.h>


status_t
get_mount_point(KPartition* partition, KPath* mountPoint);


#endif // _KERNEL_VM_UTILS_H
