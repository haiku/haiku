/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Inode.h"


void
xfs_inode_t::SwapEndian()
{
	di_magic = B_BENDIAN_TO_HOST_INT16(di_magic);
	di_mode = B_BENDIAN_TO_HOST_INT16(di_mode);
	di_onlink = B_BENDIAN_TO_HOST_INT16(di_onlink);
	di_uid = B_BENDIAN_TO_HOST_INT32(di_uid);
	di_gid = B_BENDIAN_TO_HOST_INT32(di_gid);
	di_nlink = B_BENDIAN_TO_HOST_INT32(di_nlink);
	di_projid = B_BENDIAN_TO_HOST_INT16(di_projid);
	di_flushiter = B_BENDIAN_TO_HOST_INT16(di_flushiter);
	di_atime.t_sec = B_BENDIAN_TO_HOST_INT32(di_atime.t_sec);
	di_atime.t_nsec = B_BENDIAN_TO_HOST_INT32(di_atime.t_nsec);
	di_mtime.t_sec = B_BENDIAN_TO_HOST_INT32(di_mtime.t_sec);
	di_mtime.t_nsec = B_BENDIAN_TO_HOST_INT32(di_mtime.t_nsec);
	di_ctime.t_sec = B_BENDIAN_TO_HOST_INT32(di_ctime.t_sec);
	di_ctime.t_nsec = B_BENDIAN_TO_HOST_INT32(di_ctime.t_nsec);
	di_size = B_BENDIAN_TO_HOST_INT64(di_size);
	di_nblocks =  B_BENDIAN_TO_HOST_INT64(di_nblocks);
	di_extsize = B_BENDIAN_TO_HOST_INT32(di_extsize);
	di_nextents = B_BENDIAN_TO_HOST_INT32(di_nextents);
	di_anextents = B_BENDIAN_TO_HOST_INT16(di_anextents);
	di_dmevmask = B_BENDIAN_TO_HOST_INT32(di_dmevmask);
	di_dmstate = B_BENDIAN_TO_HOST_INT16(di_dmstate);
	di_flags = B_BENDIAN_TO_HOST_INT16(di_flags);
	di_gen = B_BENDIAN_TO_HOST_INT32(di_gen);
	di_next_unlinked = B_BENDIAN_TO_HOST_INT32(di_next_unlinked);
}


xfs_rfsblock_t
xfs_inode_t::BlockCount() const
{
	return di_nblocks;
}


xfs_fsize_t
xfs_inode_t::Size() const
{
	return di_size;
}


mode_t
xfs_inode_t::Mode() const
{
	return di_mode;
}


uint32
xfs_inode_t::UserId() const
{
	return di_uid;
}


uint32
xfs_inode_t::GroupId() const
{
	return di_gid;
}


void
xfs_inode_t::GetModificationTime(struct timespec& stamp)
{
	stamp.tv_sec = di_mtime.t_sec;
	stamp.tv_nsec = di_mtime.t_nsec;
}


void
xfs_inode_t::GetAccessTime(struct timespec& stamp)
{
	stamp.tv_sec = di_atime.t_sec;
	stamp.tv_nsec = di_atime.t_nsec;
}


void
xfs_inode_t::GetChangeTime(struct timespec& stamp)
{
	stamp.tv_sec = di_ctime.t_sec;
	stamp.tv_nsec = di_ctime.t_nsec;
}


uint32
xfs_inode_t::NLink() const
{
	return di_nlink;
}


int8
xfs_inode_t::Version() const
{
	return di_version;
}


uint16
xfs_inode_t::Flags() const
{
	return di_flags;
}


int8
xfs_inode_t::Format() const
{
	return di_format;
}


xfs_extnum_t
xfs_inode_t::DataExtentsCount() const
{
	return di_nextents;
}


Inode::Inode(Volume* volume, xfs_ino_t id)
	:
	fVolume(volume),
	fId(id)
{

}


status_t
Inode::Init()
{
	fNode = new(std::nothrow) xfs_inode_t;
	if (fNode == NULL)
		return B_NO_MEMORY;

	uint16 inodeSize = fVolume->InodeSize();
	fBuffer = new(std::nothrow) char[inodeSize];
	if (fBuffer == NULL)
		return B_NO_MEMORY;

	status_t status = GetFromDisk();
	if (status == B_OK) {
		if (fNode->di_magic == INODE_MAGIC) {
			TRACE("Init(): Inode successfully read.\n");
			status = B_OK;
		} else {
			TRACE("Init(): Inode wasn't read successfully!\n");
			status = B_BAD_VALUE;
		}
	}

	return status;
}


bool
Inode::HasFileTypeField() const
{
	return fVolume->SuperBlockFeatures2() & XFS_SB_VERSION2_FTYPE;
}


status_t
Inode::GetFromDisk()
{
	xfs_agnumber_t agNo = INO_TO_AGNO(fId, fVolume);
		// Get the AG number from the inode
	uint32 agRelativeInodeNo = INO_TO_AGINO(fId, fVolume->AgInodeBits());
		// AG relative ino #
	xfs_agblock_t agBlock = INO_TO_AGBLOCK(agRelativeInodeNo, fVolume);
		// AG relative block number
	xfs_off_t offset = INO_TO_BLOCKOFFSET(fId, fVolume);
		// Offset into the block1
	uint32 len = fVolume->InodeSize();
		// Inode len to read (size of inode)

	TRACE("AgNumber: (%d), AgRelativeIno: (%d), AgRelativeBlockNum: (%d),"
		"Offset: (%d), len: (%d)\n", agNo,
		agRelativeInodeNo, agBlock, offset, len);

	if (agNo > fVolume->AgCount()) {
		ERROR("Inode::GetFromDisk : AG Number more than number of AGs");
		return B_ENTRY_NOT_FOUND;
	}

	xfs_agblock_t numberOfBlocksInAg = fVolume->AgBlocks();

	xfs_fsblock_t blockToRead = FSBLOCKS_TO_BASICBLOCKS(fVolume->BlockLog(),
		((uint64)(agNo * numberOfBlocksInAg) + agBlock));

	xfs_daddr_t readPos = blockToRead * BASICBLOCKSIZE + offset * len;

	if (read_pos(fVolume->Device(), readPos, fBuffer, len) != len) {
		ERROR("Inode::Inode(): IO Error");
		return B_IO_ERROR;
	}

	memcpy(fNode, fBuffer, INODE_CORE_UNLINKED_SIZE);
	fNode->SwapEndian();

	return B_OK;
}


Inode::~Inode()
{
	delete fBuffer;
	delete fNode;
}


/*
 * Basically take 4 characters at a time as long as you can, and xor with
 * previous hashVal after rotating 4 bits of hashVal. Likewise, continue
 * xor and rotating. This is quite a generic hash function.
*/
uint32
hashfunction(const char* name, int length)
{
	uint32 hashVal = 0;
	int lengthCovered = 0;
	int index = 0;
	if (length >= 4) {
		for (;index < length && (length - index) >= 4; index += 4)
		{
			lengthCovered += 4;
			hashVal = (name[index] << 21) ^ (name[index+1] << 14)
				^ (name[index+2] << 7) ^ (name[index+3] << 0)
				^ ((hashVal << 28) | (hashVal >> (4)));
		}
	}

	int leftToCover = length - lengthCovered;
	if (leftToCover == 3) {
		hashVal = (name[index] << 14) ^ (name[index+1] << 7)
			^ (name[index+2] << 0) ^ ((hashVal << 21) | (hashVal >> (11)));
	}
	if (leftToCover == 2) {
		hashVal = (name[index] << 7) ^ (name[index+1] << 0)
			^ ((hashVal << 14) | (hashVal >> (32 - 14)));
	}
	if (leftToCover == 1) {
		hashVal = (name[index] << 0)
			^ ((hashVal << 7) | (hashVal >> (32 - 7)));
	}

	return hashVal;
}
