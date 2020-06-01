/*
* Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
* All rights reserved. Distributed under the terms of the MIT License.
*/

#include "Inode.h"


void
xfs_inode::SwapEndian()
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
xfs_inode::NoOfBlocks() const
{
	return di_nblocks;
}


xfs_fsize_t
xfs_inode::Size() const
{
	return di_size;
}


mode_t
xfs_inode::Mode()
{
	return di_mode;
}


uint32
xfs_inode::UserId()
{
	return di_uid;
}


uint32
xfs_inode::GroupId()
{
	return di_gid;
}


void
xfs_inode::GetModificationTime(struct timespec& stamp)
{
	stamp.tv_sec = di_mtime.t_sec;
	stamp.tv_nsec = di_mtime.t_nsec;
}


void
xfs_inode::GetAccessTime(struct timespec& stamp)
{
	stamp.tv_sec = di_atime.t_sec;
	stamp.tv_nsec = di_atime.t_nsec;
}


void
xfs_inode::GetChangeTime(struct timespec& stamp)
{
	stamp.tv_sec = di_ctime.t_sec;
	stamp.tv_nsec = di_ctime.t_nsec;
}


uint32
xfs_inode::NLink()
{
	return di_nlink;
}


int8
xfs_inode::Version()
{
	return di_version;
}


uint16
xfs_inode::Flags()
{
	return di_flags;
}


int8
xfs_inode::Format()
{
	return di_format;
}


Inode::Inode(Volume* volume, xfs_ino_t id)
{
	fVolume = volume;
	fId = id;
	fNode = new(std::nothrow) xfs_inode_t;

	uint16 inodeSize = volume->InodeSize();
	fBuffer = new(std::nothrow) char[inodeSize];

	status_t status = GetFromDisk();
	if (status == B_OK) {
	status = InitCheck();
		if (status) {
			TRACE("Inode successfully read!");
		} else {
			TRACE("Inode wasn't read successfully!");
		}
	}
}


status_t
Inode::GetFromDisk()
{
	xfs_agnumber_t agNo = INO_TO_AGNO(fId, fVolume);
		// Get the AG number from the inode
	uint32 agInodeNo = INO_TO_AGINO(fId, fVolume->AgInodeBits());
		// AG relative ino #
	xfs_agblock_t agBlock = INO_TO_AGBLOCK(agInodeNo, fVolume);
		// AG relative block number
	xfs_off_t offset = INO_TO_OFFSET(fId, fVolume);
		// Offset into the block
	uint32 len = fVolume->InodeSize();
		// Inode len to read (size of inode)
	int fd = fVolume->Device();

	if (agNo > fVolume->AgCount()) {
		ERROR("Inode::GetFromDisk : AG Number more than number of AGs");
		return B_ENTRY_NOT_FOUND;
	}

	xfs_agblock_t agBlocks = fVolume->AgBlocks();

	xfs_fsblock_t blockToRead =
		FSB_TO_BB(fVolume->BlockLog(), ((uint64)(agNo * agBlocks) + agBlock));

	xfs_daddr_t readPos = blockToRead*(BBLOCKSIZE) + offset;

	if (read_pos(fd, readPos, fBuffer, len) != len) {
		ERROR("Inode::Inode(): IO Error");
		return B_IO_ERROR;
	}

	memcpy(fNode, fBuffer, INODE_CORE_UNLINKED_SIZE);
	fNode->SwapEndian();

	return B_OK;
}


bool
Inode::InitCheck()
{
	return fNode->di_magic == INODE_MAGIC;
}


Inode::~Inode()
{
	delete fBuffer;
	delete fNode;
}
