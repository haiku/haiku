/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <fs_volume.h>
#include <syscalls.h>


status_t
fs_mount_volume(const char *fileSystem, const char *where,
	const char *device, uint32 flags, const char *parameters)
{
	return _kern_mount(where, device, fileSystem, flags, (void *)parameters);
}


status_t
fs_unmount_volume(const char *path, uint32 flags)
{
	return _kern_unmount(path/*, flags*/);
}


status_t
fs_initialize_volume(const char *fileSystem, const char *volumeName,
	const char *device, uint32 flags, const char *parameters)
{
	return B_ERROR;
}

