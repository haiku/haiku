/*
 * Copyright 2001-2009, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H

#include "ext2.h"

#include <file_systems/fs_ops_support.h>


enum inode_type {
	S_DIRECTORY		= S_IFDIR,
	S_FILE			= S_IFREG,
	S_SYMLINK		= S_IFLNK,

	S_INDEX_TYPES	= (S_STR_INDEX | S_INT_INDEX | S_UINT_INDEX
						| S_LONG_LONG_INDEX | S_ULONG_LONG_INDEX
						| S_FLOAT_INDEX | S_DOUBLE_INDEX),

	S_EXTENDED_TYPES = (S_ATTR_DIR | S_ATTR | S_INDEX_DIR)
};


#endif	// UTILITY_H
