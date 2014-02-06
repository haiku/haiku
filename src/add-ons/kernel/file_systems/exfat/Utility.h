/*
 * Copyright 2001-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		John Scipione, jscipione@gmail.com
 */
#ifndef UTILITY_H
#define UTILITY_H


#include "exfat.h"


enum inode_type {
	S_DIRECTORY		= S_IFDIR,
	S_FILE			= S_IFREG,
	S_SYMLINK		= S_IFLNK,

	S_INDEX_TYPES	= (S_STR_INDEX | S_INT_INDEX | S_UINT_INDEX
						| S_LONG_LONG_INDEX | S_ULONG_LONG_INDEX
						| S_FLOAT_INDEX | S_DOUBLE_INDEX),

	S_EXTENDED_TYPES = (S_ATTR_DIR | S_ATTR | S_INDEX_DIR)
};


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


/*!	Reads the volume name from an exfat entry and writes it to
	\a name as a UTF-8 char array.

	Writes a blank string to \a name if the volume name is not set.

	\param entry The \a entry to look for the volume name in.
	\param name The \a name array to fill out.
	\param length The \a length of the name array in bytes.

	\returns A status code, \c B_OK on success or an error code otherwise.
	\retval B_OK Wrote the volume name to \a name successfully.
	\retval B_BAD_VALUE \a entry or \a name was \c NULL.
	\retval B_NAME_NOT_FOUND Volume name was not found in this \a entry.
	\retval B_NAME_TOO_LONG \a name wasn't long enough to fit the volume name.
*/
status_t get_volume_name(struct exfat_entry* entry, char* name, size_t length);


/*!	Writes a more or less descriptive volume name to \a name.

	\param partitionSize The partion size in bytes.
	\param name The \a name array to fill out.
	\param length The \a length of the name array in bytes.
*/
void get_default_volume_name(off_t partitionSize, char* name, size_t length);


#endif	// UTILITY_H
