#ifndef BFS_CONTROL_H
#define BFS_CONTROL_H
/* bfs_control - additional functionality exported via ioctl()
**
** Copyright 2001-2004, Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include "fsproto.h"


/* ioctl to check the version of BFS used - parameter is a uint32 *
 * where the number is stored
 */
#define BFS_IOCTL_VERSION			14200

/* ioctls to use the "chkbfs" feature from the outside
 * all calls use a struct check_result as single parameter
 */
#define	BFS_IOCTL_START_CHECKING	14201
#define BFS_IOCTL_STOP_CHECKING		14202
#define BFS_IOCTL_CHECK_NEXT_NODE	14203

/* all fields except "flags", and "name" must be set to zero before
 * BFS_IOCTL_START_CHECKING is called
 */
struct check_control {
	uint32		magic;
	uint32		flags;
	char		name[B_FILE_NAME_LENGTH];
	vnode_id	inode;
	uint32		mode;
	uint32		errors;
	struct {
		uint64	missing;
		uint64	already_set;
		uint64	freed;
	} stats;
	status_t	status;
	void		*cookie;
};

/* values for the flags field */
#define BFS_FIX_BITMAP_ERRORS	1
#define BFS_REMOVE_WRONG_TYPES	2
	/* files that shouldn't be part of its parent will be removed
	 * (i.e. a directory contains an attribute, ...)
	 * Works only if B_FIX_BITMAP_ERRORS is set, too
	 */
#define BFS_REMOVE_INVALID		4
	/* removes nodes that couldn't be opened at all from its parent
	 * directory.
	 * Also requires the B_FIX_BITMAP_ERRORS to be set.
	 */

/* values for the errors field */
#define BFS_MISSING_BLOCKS		1
#define BFS_BLOCKS_ALREADY_SET	2
#define BFS_INVALID_BLOCK_RUN	4
#define	BFS_COULD_NOT_OPEN		8
#define BFS_WRONG_TYPE			16
#define BFS_NAMES_DONT_MATCH	32

/* check control magic value */
#define BFS_IOCTL_CHECK_MAGIC	'BChk'

#endif	/* BFS_CONTROL_H */
