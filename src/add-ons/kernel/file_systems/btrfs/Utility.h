/*
 * Copyright 2017, Chế Vũ Gia Hy, cvghy116@gmail.com.
 * Copyright 2001-2009, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H


#include "btrfs.h"


enum inode_type {
	S_DIRECTORY		= S_IFDIR,
	S_FILE			= S_IFREG,
	S_SYMLINK		= S_IFLNK,

	S_INDEX_TYPES	= (S_STR_INDEX | S_INT_INDEX | S_UINT_INDEX
						| S_LONG_LONG_INDEX | S_ULONG_LONG_INDEX
						| S_FLOAT_INDEX | S_DOUBLE_INDEX),

	S_EXTENDED_TYPES = (S_ATTR_DIR | S_ATTR | S_INDEX_DIR)
};


inline uint8
get_filetype(int32 mode)
{
	mode &= S_IFMT;
	switch (mode)
	{
		case S_IFSOCK:
			return BTRFS_FILETYPE_SOCKET;
		case S_IFLNK:
			return BTRFS_FILETYPE_SYMLINK;
		case S_IFREG:
			return BTRFS_FILETYPE_REGULAR;
		case S_IFBLK:
			return BTRFS_FILETYPE_BLKDEV;
		case S_IFDIR:
			return BTRFS_FILETYPE_DIRECTORY;
		case S_IFCHR:
			return BTRFS_FILETYPE_CHRDEV;
		case S_IFIFO:
			return BTRFS_FILETYPE_FIFO;
		default:
			return BTRFS_FILETYPE_UNKNOWN;
	}
}


/*!	Converts the open mode, the open flags given to bfs_open(), into
	access modes, e.g. since O_RDONLY requires read access to the
	file, it will be converted to R_OK.
*/
inline int
open_mode_to_access(int openMode)
{
	openMode &= O_RWMASK;
	if (openMode == O_RDONLY)
		return R_OK;
	if (openMode == O_WRONLY)
		return W_OK;

	return R_OK | W_OK;
}


#endif	// UTILITY_H
