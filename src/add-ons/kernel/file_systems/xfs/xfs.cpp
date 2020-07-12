/*
* Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
* All rights reserved. Distributed under the terms of the MIT License.
*/

#include "xfs.h"


uint8
XfsSuperBlock::Flags() const
{
	return sb_flags;
}


bool
XfsSuperBlock::IsValid() const
{
	if (sb_magicnum != XFS_SB_MAGIC)
		return false;

	if (BASICBLOCKSIZE > sb_blocksize) {
		ERROR("Basic block is less than 512 bytes!");
		return false;
	}

	// Checking version 4 file system
	ASSERT((Version() & 0x000f) == 4)

	return true;
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


void
XfsSuperBlock::SwapEndian()
{
	sb_magicnum		=	B_BENDIAN_TO_HOST_INT32(sb_magicnum);
	sb_blocksize	=	B_BENDIAN_TO_HOST_INT32(sb_blocksize);
	sb_dblocks		=	B_BENDIAN_TO_HOST_INT64(sb_dblocks);
	sb_rblocks		=	B_BENDIAN_TO_HOST_INT64(sb_rblocks);
	sb_rextents		=	B_BENDIAN_TO_HOST_INT64(sb_rextents);
	sb_logstart		=	B_BENDIAN_TO_HOST_INT64(sb_logstart);
	sb_rootino		=	B_BENDIAN_TO_HOST_INT64(sb_rootino);
	sb_rbmino		=	B_BENDIAN_TO_HOST_INT64(sb_rbmino);
	sb_rsumino		=	B_BENDIAN_TO_HOST_INT64(sb_rsumino);
	sb_rextsize		=	B_BENDIAN_TO_HOST_INT32(sb_rextsize);
	sb_agblocks		=	B_BENDIAN_TO_HOST_INT32(sb_agblocks);
	sb_agcount		=	B_BENDIAN_TO_HOST_INT32(sb_agcount);
	sb_rbmblocks	=	B_BENDIAN_TO_HOST_INT32(sb_rbmblocks);
	sb_logblocks	=	B_BENDIAN_TO_HOST_INT32(sb_logblocks);
	sb_versionnum	=	B_BENDIAN_TO_HOST_INT16(sb_versionnum);
	sb_sectsize		=	B_BENDIAN_TO_HOST_INT16(sb_sectsize);
	sb_inodesize	=	B_BENDIAN_TO_HOST_INT16(sb_inodesize);
	sb_inopblock	=	B_BENDIAN_TO_HOST_INT16(sb_inopblock);
	sb_icount		=	B_BENDIAN_TO_HOST_INT64(sb_icount);
	sb_ifree		=	B_BENDIAN_TO_HOST_INT64(sb_ifree);
	sb_fdblocks		=	B_BENDIAN_TO_HOST_INT64(sb_fdblocks);
	sb_frextents	=	B_BENDIAN_TO_HOST_INT64(sb_frextents);
	sb_uquotino		=	B_BENDIAN_TO_HOST_INT64(sb_uquotino);
	sb_gquotino		=	B_BENDIAN_TO_HOST_INT64(sb_gquotino);
	sb_qflags		=	B_BENDIAN_TO_HOST_INT16(sb_qflags);
	sb_inoalignmt	=	B_BENDIAN_TO_HOST_INT32(sb_inoalignmt);
	sb_unit			=	B_BENDIAN_TO_HOST_INT32(sb_unit);
	sb_width		=	B_BENDIAN_TO_HOST_INT32(sb_width);
	sb_logsectsize	=	B_BENDIAN_TO_HOST_INT16(sb_logsectsize);
	sb_logsunit		=	B_BENDIAN_TO_HOST_INT32(sb_logsunit);
	sb_features2	=	B_BENDIAN_TO_HOST_INT32(sb_features2);
}
