/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
*/


#include <boot/platform.h>
#include <boot/vfs.h>
#include <boot/PathBlocklist.h>


extern "C" void
platform_load_ucode(BootVolume& volume)
{
}


status_t
packagefs_mount_file(int fd, ::Directory* systemDirectory,
	::Directory*& _mountedDirectory)
{
	return B_NOT_SUPPORTED;
}


void
packagefs_apply_path_blocklist(::Directory* systemDirectory,
	const PathBlocklist& pathBlocklist)
{
}
