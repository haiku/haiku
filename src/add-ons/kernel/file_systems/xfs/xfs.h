/*
 * Copyright 2001-2017, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

/*
 *Important:
 *All fields in XFS metadata structures are in big-endian byte order
 *except for log items which are formatted in host order.
 *
 *This file contains all global structure definitions.
 */
#ifndef _XFS_SB_H_
#define _XFS_SB_H_


#include "system_dependencies.h"
#include "xfs_types.h"


extern fs_vnode_ops gxfsVnodeOps;
extern fs_volume_ops gxfsVolumeOps;


#define XFS_SB_MAGIC 0x58465342
	// Identifies XFS. "XFSB"
#define XFS_SB_MAXSIZE 512
#define BASICBLOCKLOG 9
	// Log of block size should be 9
#define BASICBLOCKSIZE (1 << BASICBLOCKLOG)
	// The size of a basic block should be 512
#define XFS_OPEN_MODE_USER_MASK 0x7fffffff

/* B+Tree related macros
*/
#define XFS_BTREE_SBLOCK_SIZE	18
	// Header for Short Format btree
#define XFS_BTREE_LBLOCK_SIZE	24
	// Header for Long Format btree
#define XFS_BMAP_MAGIC 0x424d4150
#define MAX_TREE_DEPTH 5
#define XFS_KEY_SIZE sizeof(xfs_fileoff_t)
#define XFS_PTR_SIZE sizeof(xfs_fsblock_t)

struct file_cookie {
	bigtime_t last_notification;
	off_t	last_size;
	int		open_mode;
};


/*	Version 4 superblock definition	*/
class XfsSuperBlock {
public:

			bool 				IsValid() const;
			const char*			Name() const;
			uint32				BlockSize() const;
			uint8				BlockLog() const;
			uint32				DirBlockSize() const;
				// maximum 65536
			uint32				DirBlockLog() const;
			uint8				AgInodeBits() const;
			uint8				InodesPerBlkLog() const;
			uint8				AgBlocksLog() const;
			xfs_rfsblock_t		TotalBlocks() const;
			xfs_rfsblock_t		TotalBlocksWithLog() const;
			uint64				FreeBlocks() const;
			uint64				UsedBlocks() const;
			uint32				Size() const;
			uint16				InodeSize() const;
			void				SwapEndian();
			xfs_ino_t			Root() const;
			xfs_agnumber_t		AgCount() const;
			xfs_agblock_t		AgBlocks() const;
			uint8				Flags() const;
			uint16				Version() const;
			uint32				Features2() const;
private:

			uint32				sb_magicnum;
			uint32				sb_blocksize;
			xfs_rfsblock_t		sb_dblocks;
			xfs_rfsblock_t		sb_rblocks;
			xfs_rtblock_t		sb_rextents;
			uuid_t				sb_uuid;
			xfs_fsblock_t		sb_logstart;
			xfs_ino_t			sb_rootino;
			xfs_ino_t			sb_rbmino;
			xfs_ino_t			sb_rsumino;
			xfs_agblock_t		sb_rextsize;
			xfs_agblock_t		sb_agblocks;
			xfs_agnumber_t		sb_agcount;
			xfs_extlen_t		sb_rbmblocks;
			xfs_extlen_t		sb_logblocks;
			uint16				sb_versionnum;
			uint16				sb_sectsize;
			uint16				sb_inodesize;
			uint16				sb_inopblock;
			char				sb_fname[12];
			uint8				sb_blocklog;
			uint8				sb_sectlog;
			uint8				sb_inodelog;
			uint8				sb_inopblog;
			uint8				sb_agblklog;
			uint8				sb_rextslog;
			uint8				sb_inprogress;
			uint8				sb_imax_pct;
			uint64				sb_icount;
			uint64				sb_ifree;
			uint64				sb_fdblocks;
			uint64				sb_frextents;
			xfs_ino_t			sb_uquotino;
			xfs_ino_t			sb_gquotino;
			uint16				sb_qflags;
			uint8				sb_flags;
			uint8				sb_shared_vn;
			xfs_extlen_t		sb_inoalignmt;
			uint32				sb_unit;
			uint32				sb_width;
			uint8				sb_dirblklog;
			uint8				sb_logsectlog;
			uint16				sb_logsectsize;
			uint32				sb_logsunit;
			uint32				sb_features2;
};


/*
*These flags indicate features introduced over time.
*
*If the lower nibble of sb_versionnum >=4 then the following features are
*checked. If it's equal to 5, it's version 5.
*/

#define XFS_SB_VERSION_ATTRBIT 0x0010
#define XFS_SB_VERSION_NLINKBIT 0x0020
#define XFS_SB_VERSION_QUOTABIT 0x0040
#define XFS_SB_VERSION_ALIGNBIT 0x0080
#define XFS_SB_VERSION_DALIGNBIT 0x0100
#define XFS_SB_VERSION_SHAREDBIT 0x0200
#define XFS_SB_VERSION_LOGV2BIT 0x0400
#define XFS_SB_VERSION_SECTORBIT 0x0800
#define XFS_SB_VERSION_EXTFLGBIT 0x1000
#define XFS_SB_VERSION_DIRV2BIT 0x2000
#define XFS_SB_VERSION_MOREBITSBIT 0x4000


/*
Superblock quota flags - sb_qflags
*/
#define XFS_UQUOTA_ACCT 0x0001
#define XFS_UQUOTA_ENFD 0x0002
#define XFS_UQUOTA_CHKD 0x0004
#define XFS_PQUOTA_ACCT 0x0008
#define XFS_OQUOTA_ENFD 0x0010
#define XFS_OQUOTA_CHKD 0x0020
#define XFS_GQUOTA_ACCT 0x0040
#define XFS_GQUOTA_ENFD 0x0080
#define XFS_GQUOTA_CHKD 0x0100
#define XFS_PQUOTA_ENFD 0x0200
#define XFS_PQUOTA_CHKD 0x0400


/*
	Superblock flags - sb_flags
*/
#define XFS_SBF_READONLY 0x1


/*
	Extended v4 Superblock flags - sb_features2
*/

#define XFS_SB_VERSION2_LAZYSBCOUNTBIT	0x00000001
	// update global free space and inode on clean unmount
#define XFS_SB_VERSION2_ATTR2BIT	0x00000002
	// optimises the inode layout of ext-attr
#define XFS_SB_VERSION2_PARENTBIT 0x00000010	/* Parent pointers */
#define XFS_SB_VERSION2_PROJID32BIT 0x00000080	/* 32-bit project id */
#define XFS_SB_VERSION2_CRCBIT 0x00000100		/* Metadata checksumming */
#define XFS_SB_VERSION2_FTYPE 0x00000200

#endif
