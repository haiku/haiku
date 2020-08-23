/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * Distributed under the terms of the MIT License.
 */
#ifndef _XFS_TYPES_H_
#define _XFS_TYPES_H_

#include "system_dependencies.h"

/*
Reference documentation: 
https://mirrors.edge.kernel.org/pub/linux/utils/fs/xfs/docs
/xfs_filesystem_structure.pdf

Chapter 5: Common XFS types		(Page 8)
*/


typedef uint64 xfs_ino_t;		// absolute inode number
typedef int64 xfs_off_t;		// file offset
typedef int64 xfs_daddr_t;		// device address
typedef uint32 xfs_agnumber_t;	// Allocation Group (AG) number
typedef uint32 xfs_agblock_t;	// AG relative block number
typedef uint32 xfs_extlen_t;	// extent length in blocks
typedef int32 xfs_extnum_t;		// number of extends in a file
typedef int16 xfs_aextnum_t;	// number of extents in an attribute fork
typedef uint32	xfs_dablk_t;	// block number for directories
								// and extended attributes
typedef uint32	xfs_dahash_t;	// hash of a directory file name
								// or extended attribute name
typedef uint64 xfs_fsblock_t;	// filesystem block number combining AG number
typedef uint64 xfs_rfsblock_t;	// raw filesystem block number
typedef uint64 xfs_rtblock_t;	// extent number in the real-time sub-volume
typedef uint64 xfs_fileoff_t;	// block offset into a file
typedef uint64 xfs_filblks_t;	// block count for a file
typedef int64 xfs_fsize_t;		// byte size of a file
typedef xfs_fileoff_t TreeKey;
typedef xfs_fsblock_t TreePointer;

// typedef unsigned char uuid_t[16];

#endif
