/*
* Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
* All rights reserved. Distributed under the terms of the MIT License.
*/

#include "xfs.h"

#include "Inode.h"


uint8
XfsSuperBlock::Flags() const
{
	return sb_flags;
}


bool
XfsSuperBlock::IsValidVersion() const
{
	// Version 5 is supported
	if ((Version() & XFS_SB_VERSION_NUMBITS) == 5) {
		return true;
	}

	// Version below 4 are not supported
	if ((Version() & XFS_SB_VERSION_NUMBITS) < 4) {
		ERROR("xfs version below 4 is not supported");
		return false;
	}

	// V4 filesystems need v2 directories and unwritten extents
	if (!(Version() & XFS_SB_VERSION_DIRV2BIT)) {
		ERROR("xfs version 4 uses version 2 directories");
		return false;
	}
	if (!(Version() & XFS_SB_VERSION_EXTFLGBIT)) {
		ERROR("xfs version 4 uses unwritten extents");
		return false;
	}

	// V4 should not have any unknown v4 feature bits set
	if ((Version()  & ~XFS_SB_VERSION_OKBITS) ||
	    ((Version()  & XFS_SB_VERSION_MOREBITSBIT) &&
	     (Features2() & ~XFS_SB_VERSION2_OKBITS))) {
		ERROR("xfs version 4 unknown feature bit detected");
		return false;
	}

	// Valid V4 filesytem
	return true;
}


bool
XfsSuperBlock::IsValidFeatureMask() const
{
	// Version 5 superblock feature mask validation
	if(sb_features_compat & XFS_SB_FEAT_COMPAT_UNKNOWN) {
		ERROR("Superblock has unknown compatible features enabled");
		ERROR("Use more recent kernal");
	}

    // We cannot have write support if this is set
	if(sb_features_ro_compat & XFS_SB_FEAT_RO_COMPAT_UNKNOWN) {
		ERROR("Superblock has unknown read-only compatible features enabled");
		ERROR("Filesystem is read-only");
	}

	// check for incompatible features
	if(sb_features_incompat & XFS_SB_FEAT_INCOMPAT_UNKNOWN) {
		ERROR("Superblock has unknown incompatible features enabled");
		return false;
	}

	return true;
}


bool
XfsSuperBlock::IsValid() const
{
	if (sb_magicnum != XFS_SB_MAGIC) {
		ERROR("Invalid Superblock");
		return false;
	}

	// For version 4
	if (XFS_MIN_BLOCKSIZE > sb_blocksize) {
		ERROR("Basic block is less than 512 bytes!");
		return false;
	}

	// Checking correct version of filesystem
	if(!(IsValidVersion())) {
		return false;
	}

	if ((Version() & XFS_SB_VERSION_NUMBITS) == 4) {

		if(sb_qflags & (XFS_PQUOTA_ENFD | XFS_GQUOTA_ENFD |
				XFS_PQUOTA_CHKD | XFS_GQUOTA_CHKD)) {
			ERROR("V4 Superblock has XFS_{P|G}QUOTA_{ENFD|CHKD} bits");
			return false;
		}

		return true;
	}

	if(!(IsValidFeatureMask())) {
		return false;
	}

	// For V5
    if (XFS_MIN_CRC_BLOCKSIZE > sb_blocksize) {
		ERROR("Basic block is less than 1024 bytes!");
		return false;
	}

	// V5 has a separate project quota inode
	if (sb_qflags & (XFS_OQUOTA_ENFD | XFS_OQUOTA_CHKD)) {
		ERROR("Version 5 of Super block has XFS_OQUOTA bits");
		return false;
	}

	/*TODO: check if full Inode chunks are aligned to
			inode chunk size when sparse inodes are
			enabled to support the sparse chunk allocation
			algorithm and prevent overlapping inode record.
	*/

	// Sanity Checking
	if(sb_agcount <= 0
		||	sb_sectsize < XFS_MIN_SECTORSIZE
	    ||	sb_sectsize > XFS_MAX_SECTORSIZE
	    ||	sb_sectlog < XFS_MIN_SECTORSIZE_LOG
	    ||	sb_sectlog > XFS_MAX_SECTORSIZE_LOG
	    ||	sb_sectsize != (1 << sb_sectlog)
	    ||	sb_blocksize < XFS_MIN_BLOCKSIZE
	    ||	sb_blocksize > XFS_MAX_BLOCKSIZE
	    ||	sb_blocklog < XFS_MIN_BLOCKSIZE_LOG
	    ||	sb_blocklog > XFS_MAX_BLOCKSIZE_LOG
	    ||	sb_blocksize != (uint32)(1 << sb_blocklog)
	    ||	sb_dirblklog + sb_blocklog > XFS_MAX_BLOCKSIZE_LOG
	    ||	sb_inodesize < INODE_MIN_SIZE
	    ||	sb_inodesize > INODE_MAX_SIZE
	    ||	sb_inodelog < INODE_MINSIZE_LOG
	    ||	sb_inodelog > INODE_MAXSIZE_LOG
	    ||	sb_inodesize != (1 << sb_inodelog)) {

		ERROR("Sanity checking failed");
		return false;
	}

	// Valid V5 Superblock
	return true;
}


bool
XfsSuperBlock::IsVersion5() const
{
	return (Version() & XFS_SB_VERSION_NUMBITS) == 5;
}


bool
XfsSuperBlock::XfsHasIncompatFeature() const
{
	return (sb_features_incompat & XFS_SB_FEAT_INCOMPAT_FTYPE) != 0;
}


uint16
XfsSuperBlock::Version() const
{
	return sb_versionnum;
}


uint32
XfsSuperBlock::Features2() const
{
	return sb_features2;
}


uint32
XfsSuperBlock::BlockSize() const
{
	return sb_blocksize;
}


uint32
XfsSuperBlock::DirBlockLog() const
{
	return sb_dirblklog;
}


uint8
XfsSuperBlock::BlockLog() const
{
	return sb_blocklog;
}


uint32
XfsSuperBlock::DirBlockSize() const
{
	return BlockSize() * (1 << sb_dirblklog);
}


uint8
XfsSuperBlock::AgInodeBits() const
{
	return AgBlocksLog() + InodesPerBlkLog();
}


uint8
XfsSuperBlock::InodesPerBlkLog() const
{
	return sb_inopblog;
}


uint8
XfsSuperBlock::AgBlocksLog() const
{
	return sb_agblklog;
}


uint32
XfsSuperBlock::Size() const
{
	return XFS_SB_MAXSIZE;
}


uint16
XfsSuperBlock::InodeSize() const
{
	return sb_inodesize;
}


xfs_rfsblock_t
XfsSuperBlock::TotalBlocks() const
{
	return sb_dblocks;
}


xfs_rfsblock_t
XfsSuperBlock::TotalBlocksWithLog() const
{
	return TotalBlocks() + sb_logblocks;
}


uint64
XfsSuperBlock::FreeBlocks() const
{
	return sb_fdblocks;
}


uint64
XfsSuperBlock::UsedBlocks() const
{
	return TotalBlocks() - FreeBlocks();
}


const char*
XfsSuperBlock::Name() const
{
	return sb_fname;
}


xfs_ino_t
XfsSuperBlock::Root() const
{
	return sb_rootino;
}


xfs_agnumber_t
XfsSuperBlock::AgCount() const
{
	return sb_agcount;
}


xfs_agblock_t
XfsSuperBlock::AgBlocks() const
{
	return sb_agblocks;
}


uint32
XfsSuperBlock::Crc() const
{
	return sb_crc;
}


uint32
XfsSuperBlock::MagicNum() const
{
	return sb_magicnum;
}


bool
XfsSuperBlock::UuidEquals(const uuid_t& u1)
{
	if((sb_features_incompat & XFS_SB_FEAT_INCOMPAT_META_UUID) != 0) {
		return memcmp(&u1, &sb_meta_uuid, sizeof(uuid_t)) == 0;
	} else {
		return memcmp(&u1, &sb_uuid, sizeof(uuid_t)) == 0;
	}
	return false;
}


void
XfsSuperBlock::SwapEndian()
{
	sb_magicnum				=	B_BENDIAN_TO_HOST_INT32(sb_magicnum);
	sb_blocksize			=	B_BENDIAN_TO_HOST_INT32(sb_blocksize);
	sb_dblocks				=	B_BENDIAN_TO_HOST_INT64(sb_dblocks);
	sb_rblocks				=	B_BENDIAN_TO_HOST_INT64(sb_rblocks);
	sb_rextents				=	B_BENDIAN_TO_HOST_INT64(sb_rextents);
	sb_logstart				=	B_BENDIAN_TO_HOST_INT64(sb_logstart);
	sb_rootino				=	B_BENDIAN_TO_HOST_INT64(sb_rootino);
	sb_rbmino				=	B_BENDIAN_TO_HOST_INT64(sb_rbmino);
	sb_rsumino				=	B_BENDIAN_TO_HOST_INT64(sb_rsumino);
	sb_rextsize				=	B_BENDIAN_TO_HOST_INT32(sb_rextsize);
	sb_agblocks				=	B_BENDIAN_TO_HOST_INT32(sb_agblocks);
	sb_agcount				=	B_BENDIAN_TO_HOST_INT32(sb_agcount);
	sb_rbmblocks			=	B_BENDIAN_TO_HOST_INT32(sb_rbmblocks);
	sb_logblocks			=	B_BENDIAN_TO_HOST_INT32(sb_logblocks);
	sb_versionnum			=	B_BENDIAN_TO_HOST_INT16(sb_versionnum);
	sb_sectsize				=	B_BENDIAN_TO_HOST_INT16(sb_sectsize);
	sb_inodesize			=	B_BENDIAN_TO_HOST_INT16(sb_inodesize);
	sb_inopblock			=	B_BENDIAN_TO_HOST_INT16(sb_inopblock);
	sb_icount				=	B_BENDIAN_TO_HOST_INT64(sb_icount);
	sb_ifree				=	B_BENDIAN_TO_HOST_INT64(sb_ifree);
	sb_fdblocks				=	B_BENDIAN_TO_HOST_INT64(sb_fdblocks);
	sb_frextents			=	B_BENDIAN_TO_HOST_INT64(sb_frextents);
	sb_uquotino				=	B_BENDIAN_TO_HOST_INT64(sb_uquotino);
	sb_gquotino				=	B_BENDIAN_TO_HOST_INT64(sb_gquotino);
	sb_qflags				=	B_BENDIAN_TO_HOST_INT16(sb_qflags);
	sb_inoalignmt			=	B_BENDIAN_TO_HOST_INT32(sb_inoalignmt);
	sb_unit					=	B_BENDIAN_TO_HOST_INT32(sb_unit);
	sb_width				=	B_BENDIAN_TO_HOST_INT32(sb_width);
	sb_logsectsize			=	B_BENDIAN_TO_HOST_INT16(sb_logsectsize);
	sb_logsunit				=	B_BENDIAN_TO_HOST_INT32(sb_logsunit);
	sb_features2			=	B_BENDIAN_TO_HOST_INT32(sb_features2);
	sb_bad_features2		=	B_BENDIAN_TO_HOST_INT32(sb_bad_features2);
	sb_features_compat		=	B_BENDIAN_TO_HOST_INT32(sb_features_compat);
	sb_features_ro_compat	=	B_BENDIAN_TO_HOST_INT32(sb_features_ro_compat);
	sb_features_incompat 	=	B_BENDIAN_TO_HOST_INT32(sb_features_incompat);
	sb_features_log_incompat =	B_BENDIAN_TO_HOST_INT32(sb_features_log_incompat);
	// crc is only used on disk, not in memory; just init to 0 here
	sb_crc					=	0;
	sb_spino_align			=	B_BENDIAN_TO_HOST_INT32(sb_spino_align);
	sb_pquotino				=	B_BENDIAN_TO_HOST_INT64(sb_pquotino);
	sb_lsn					=	B_BENDIAN_TO_HOST_INT64(sb_lsn);
}
