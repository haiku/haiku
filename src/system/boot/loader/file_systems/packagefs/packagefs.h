/*
 * Copyright 2011-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOT_LOADER_FILE_SYSTEMS_PACKAGEFS_H
#define BOOT_LOADER_FILE_SYSTEMS_PACKAGEFS_H


#include <sys/cdefs.h>

#include <SupportDefs.h>


class PathBlacklist;
class Directory;
class Node;


status_t packagefs_mount_file(int fd, Directory* systemDirectory,
	Directory*& _mountedDirectory);

void packagefs_apply_path_blacklist(Directory* systemDirectory,
	const PathBlacklist& pathBlacklist);


#endif	// BOOT_LOADER_FILE_SYSTEMS_PACKAGEFS_H
