/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

/*
Important:
All fields in XFS metadata structures are in big-endian byte order
except for log items which are formatted in host order.
*/

#ifndef _XFS_SB_H_
#define _XFS_SB_H_

#include "system_dependencies.h"
#include "xfs_types.h"

#define XFS_SB_MAGIC 0x58465342 /* Identifies XFS. "XFSB" */
#define XFS_SB_MAXSIZE 512
#define BBLOCKLOG 9		/* Log of block size should be 9 */
#define BBLOCKSIZE 1<<BBLOCKLOG		/* The size of a basic block should be 512 */


/*	Version 4 superblock definition	*/
class XfsSuperBlock
{
public:

			bool 			IsValid();
			char*			Name();
			uint32			BlockSize();
			xfs_rfsblock_t	TotalBlocks();
			xfs_rfsblock_t	TotalBlocksWithLog();
			uint64			FreeBlocks();
			uint64			UsedBlocks();
			uint32			Size();
			void			SwapEndian();

private:

			uint32			sb_magicnum;
			uint32			sb_blocksize;
			xfs_rfsblock_t	sb_dblocks;
			xfs_rfsblock_t	sb_rblocks;
			xfs_rtblock_t	sb_rextents;
			uuid_t			sb_uuid;
			xfs_fsblock_t	sb_logstart;
			xfs_ino_t		sb_rootino;
			xfs_ino_t		sb_rbmino;
			xfs_ino_t		sb_rsumino;
			xfs_agblock_t	sb_rextsize;
			xfs_agblock_t	sb_agblocks;
			xfs_agnumber_t	sb_agcount;
			xfs_extlen_t	sb_rbmblocks;
			xfs_extlen_t	sb_logblocks;
			uint16			sb_versionnum;
			uint16			sb_sectsize;
			uint16			sb_inodesize;
			uint16			sb_inopblock;
			char			sb_fname[12];
			uint8			sb_blocklog;
			uint8			sb_sectlog;
			uint8			sb_inodelog;
			uint8			sb_inopblog;
			uint8			sb_agblklog;
			uint8			sb_rextslog;
			uint8			sb_inprogress;
			uint8			sb_imax_pct;
			uint64			sb_icount;
			uint64			sb_ifree;
			uint64			sb_fdblocks;
			uint64			sb_frextents;
			xfs_ino_t		sb_uquotino;
			xfs_ino_t		sb_gquotino;
			uint16			sb_qflags;
			uint8			sb_flags;
			uint8			sb_shared_vn;
			xfs_extlen_t	sb_inoalignmt;
			uint32			sb_unit;
			uint32			sb_width;
			uint8			sb_dirblklog;
			uint8			sb_logsectlog;
			uint16			sb_logsectsize;
			uint32			sb_logsunit;
			uint32			sb_features2;
};

/*
 These flags indicate features introduced over time.

 If the lower nibble of sb_versionnum >=4 then the following features are
 checked. If it's equal to 5, it's version 5.
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

#define XFS_SB_VERSION2_LAZYSBCOUNTBIT											\
	0x0001	/*update global free space											\
			and inode on clean unmount*/
#define XFS_SB_VERSION2_ATTR2BIT												\
	0x0002	/* optimises the inode layout of ext-attr */
#define XFS_SB_VERSION2_PARENTBIT 0x0004	/* Parent pointers */
#define XFS_SB_VERSION2_PROJID32BIT 0x0008	/* 32-bit project id */
#define XFS_SB_VERSION2_CRCBIT 0x0010		/* Metadata checksumming */
#define XFS_SB_VERSION2_FTYPE 0x0020


/* AG Free Space Block */

/* 
index 0 for free space B+Tree indexed by block number
index 1 for free space B+Tree indexed by extent size

Version 5 has XFS_BT_NUM_AGF defined as 3. This is because the index 2 is
for reverse-mapped B+Trees. I have spare0/1 defined here instead.
*/

#define XFS_BTNUM_AGF 2
#define XFS_AG_MAGICNUM 0x58414746
class AGFreeSpace{
	public:
			//TODO:
			void			SwapEndian();
	private:
			uint32			magicnum;
			uint32			versionnum;		// should be set to 1
			uint32			seqno;	// defines the ag number for the sector
			uint32			length;	// size of ag in fs blocks
			uint32			roots[XFS_BTNUM_AGF];	// roots of trees
			uint32			spare0;		//spare
			uint32			levels[XFS_BTNUM_AGF];	// tree levels
			uint32			spare1;		//spare
			uint32			flfirst;	// index to first free list block
			uint32			fllast;		// index to last free list block
			uint32			flcount;	// number of blocks in freelist
			uint32			freeblks;	// current num of free blocks in AG
			uint32			longest;	// no.of blocks of longest
											// contiguous free space in the AG

			/*number of blocks used for the free space B+trees.
			This is only used if the XFS_SB_VERSION2_LAZYSBCOUNTBIT
			bit is set in sb_features2
			*/

			uint32			btreeblks;
};


#endif
