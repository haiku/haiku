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
#define XFS_MIN_BLOCKSIZE_LOG	9
#define XFS_MAX_BLOCKSIZE_LOG	16
#define XFS_MIN_BLOCKSIZE	(1 << XFS_MIN_BLOCKSIZE_LOG)
#define XFS_MAX_BLOCKSIZE	(1 << XFS_MAX_BLOCKSIZE_LOG)
#define XFS_MIN_CRC_BLOCKSIZE	(1 << (XFS_MIN_BLOCKSIZE_LOG + 1))
#define XFS_MIN_SECTORSIZE_LOG	9
#define XFS_MAX_SECTORSIZE_LOG	15
#define XFS_MIN_SECTORSIZE	(1 << XFS_MIN_SECTORSIZE_LOG)
#define XFS_MAX_SECTORSIZE	(1 << XFS_MAX_SECTORSIZE_LOG)
#define XFS_SB_MAXSIZE 512


#define XFS_OPEN_MODE_USER_MASK 0x7fffffff
#define	XFS_SB_VERSION_NUMBITS	0x000f
#define	XFS_SB_VERSION_ALLFBITS	0xfff0


/* B+Tree related macros
*/
#define XFS_BMAP_MAGIC 0x424d4150
#define XFS_BMAP_CRC_MAGIC 0x424d4133
#define MAX_TREE_DEPTH 5
#define XFS_KEY_SIZE sizeof(xfs_fileoff_t)
#define XFS_PTR_SIZE sizeof(xfs_fsblock_t)


struct file_cookie {
	bigtime_t last_notification;
	off_t	last_size;
	int		open_mode;
};


class XfsSuperBlock {
public:

			bool 				IsValid() const;
			bool				IsValidVersion() const;
			bool 				IsValidFeatureMask() const;
			bool				IsVersion5() const;
			bool				XfsHasIncompatFeature() const;
			bool				UuidEquals(const uuid_t& u1);
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
			uint32				Crc() const;
			uint32				MagicNum() const;
	static size_t				Offset_crc()
								{ return offsetof(XfsSuperBlock, sb_crc);}
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
			uint32				sb_bad_features2;

			// version 5

			uint32				sb_features_compat;
			uint32				sb_features_ro_compat;
			uint32				sb_features_incompat;
			uint32				sb_features_log_incompat;
			uint32				sb_crc;
private:
			xfs_extlen_t		sb_spino_align;

			xfs_ino_t			sb_pquotino;
			xfs_lsn_t			sb_lsn;
			uuid_t				sb_meta_uuid;
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

#define	XFS_SB_VERSION_OKBITS		\
	((XFS_SB_VERSION_NUMBITS | XFS_SB_VERSION_ALLFBITS) & \
		~XFS_SB_VERSION_SHAREDBIT)

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

#define	XFS_SB_VERSION2_OKBITS		\
	(XFS_SB_VERSION2_LAZYSBCOUNTBIT	| \
	 XFS_SB_VERSION2_ATTR2BIT	| \
	 XFS_SB_VERSION2_PROJID32BIT	| \
	 XFS_SB_VERSION2_FTYPE)

/*
	Extended Version 5 Superblock Read-Only compatibility flags
*/

#define XFS_SB_FEAT_RO_COMPAT_FINOBT   (1 << 0)		/* free inode btree */
#define XFS_SB_FEAT_RO_COMPAT_RMAPBT   (1 << 1)		/* reverse map btree */
#define XFS_SB_FEAT_RO_COMPAT_REFLINK  (1 << 2)		/* reflinked files */
#define XFS_SB_FEAT_RO_COMPAT_INOBTCNT (1 << 3)		/* inobt block counts */

/*
	Extended Version 5 Superblock Read-Write incompatibility flags
*/

#define XFS_SB_FEAT_INCOMPAT_FTYPE	(1 << 0)	/* filetype in dirent */
#define XFS_SB_FEAT_INCOMPAT_SPINODES	(1 << 1)	/* sparse inode chunks */
#define XFS_SB_FEAT_INCOMPAT_META_UUID	(1 << 2)	/* metadata UUID */
#define XFS_SB_FEAT_INCOMPAT_BIGTIME	(1 << 3)	/* large timestamps */

/*
	 Extended v5 superblock feature masks. These are to be used for new v5
 	 superblock features only.
*/

#define XFS_SB_FEAT_COMPAT_ALL 0
#define XFS_SB_FEAT_COMPAT_UNKNOWN	~XFS_SB_FEAT_COMPAT_ALL

#define XFS_SB_FEAT_RO_COMPAT_ALL \
		(XFS_SB_FEAT_RO_COMPAT_FINOBT | \
		 XFS_SB_FEAT_RO_COMPAT_RMAPBT | \
		 XFS_SB_FEAT_RO_COMPAT_REFLINK| \
		 XFS_SB_FEAT_RO_COMPAT_INOBTCNT)
#define XFS_SB_FEAT_RO_COMPAT_UNKNOWN	~XFS_SB_FEAT_RO_COMPAT_ALL

#define XFS_SB_FEAT_INCOMPAT_ALL \
		(XFS_SB_FEAT_INCOMPAT_FTYPE|	\
		 XFS_SB_FEAT_INCOMPAT_SPINODES|	\
		 XFS_SB_FEAT_INCOMPAT_META_UUID| \
		 XFS_SB_FEAT_INCOMPAT_BIGTIME)

#define XFS_SB_FEAT_INCOMPAT_UNKNOWN	~XFS_SB_FEAT_INCOMPAT_ALL

#endif
