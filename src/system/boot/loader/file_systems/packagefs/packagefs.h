/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOT_LOADER_FILE_SYSTEMS_PACKAGEFS_H
#define BOOT_LOADER_FILE_SYSTEMS_PACKAGEFS_H


#include <sys/cdefs.h>

#include <SupportDefs.h>


class Directory;
class Node;


__BEGIN_DECLS


status_t packagefs_mount_file(int fd, Directory*& _mountedDirectory);


__END_DECLS


#endif	// BOOT_LOADER_FILE_SYSTEMS_PACKAGEFS_H
