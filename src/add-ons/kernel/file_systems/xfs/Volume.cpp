/*
 * Copyright 2001 - 2017, Axel Dörfler, axeld @pinc - software.de.
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Volume.h"

#include "Checksum.h"
#include "Inode.h"


Volume::Volume(fs_volume *volume)
	: fFSVolume(volume)
{
	fFlags = 0;
	mutex_init(&fLock, "xfs volume");
	TRACE("Volume::Volume() : Initialising volume");
}


Volume::~Volume()
{
	mutex_destroy(&fLock);
	TRACE("Volume::Destructor : Removing Volume");
}


bool
Volume::IsValidSuperBlock() const
{
	return fSuperBlock.IsValid();
}


status_t
Volume::Identify(int fd, XfsSuperBlock *superBlock)
{

	TRACE("Volume::Identify() : Identifying Volume in progress");

	//Create a buffer of 512 bytes for Crc verification
	char buf[512];

	if(read_pos(fd, 0, buf, 512) != 512)
		return B_IO_ERROR;

	memcpy(superBlock, buf, sizeof(XfsSuperBlock));

	int version = B_BENDIAN_TO_HOST_INT16(superBlock->Version()) & XFS_SB_VERSION_NUMBITS;

	// if its V5 filesystem check for superblock checksum
	if (superBlock->MagicNum() == B_HOST_TO_BENDIAN_INT32(XFS_SB_MAGIC)
		&& (version == 5 || superBlock->Crc() != 0)) {

		TRACE("Superblock Crc: (%" B_PRIu32 ")\n", superBlock->Crc());

		if(!xfs_verify_cksum(buf, 512, XfsSuperBlock::Offset_crc())) {
			 ERROR("Filesystem is corrupted");
			 return B_BAD_VALUE;
		}

	}

	superBlock->SwapEndian();

	if (!superBlock->IsValid()) {
		ERROR("Volume::Identify(): Invalid Superblock!\n");
		return B_BAD_VALUE;
	}
	return B_OK;
}


status_t
Volume::Mount(const char *deviceName, uint32 flags)
{
	TRACE("Volume::Mount() : Mounting in progress");

	flags |= B_MOUNT_READ_ONLY;

	if ((flags & B_MOUNT_READ_ONLY) != 0) {
		TRACE("Volume::Mount(): Read only\n");
	} else {
		TRACE("Volume::Mount(): Read write\n");
	}

	DeviceOpener opener(deviceName, (flags & B_MOUNT_READ_ONLY) != 0
										? O_RDONLY
										: O_RDWR);
	fDevice = opener.Device();
	if (fDevice < B_OK) {
		ERROR("Volume::Mount(): couldn't open device\n");
		return fDevice;
	}

	if (opener.IsReadOnly())
		fFlags |= VOLUME_READ_ONLY;

	// read the superblock
	status_t status = Identify(fDevice, &fSuperBlock);
	if (status != B_OK) {
		ERROR("Volume::Mount(): Invalid super block!\n");
		return B_BAD_VALUE;
	}

	if ((fSuperBlock.Version() & XFS_SB_VERSION_NUMBITS) == 5)
		TRACE("Volume::Mount(): Valid Version 5 SuperBlock.\n");
	else
		TRACE("Volume::Mount(): Valid Version 4 SuperBlock.\n");


	// check if the device size is large enough to hold the file system
	off_t diskSize;
	if (opener.GetSize(&diskSize) != B_OK) {
		ERROR("Volume:Mount() Unable to get diskSize");
		return B_ERROR;
	}

	opener.Keep();

	//publish the root inode
	Inode* rootInode = new(std::nothrow) Inode(this, Root());
	if (rootInode == NULL)
		return B_NO_MEMORY;

	status = rootInode->Init();
	if (status != B_OK)
		return status;

	status = publish_vnode(FSVolume(), Root(),
		(void*)rootInode, &gxfsVnodeOps, rootInode->Mode(), 0);
	if (status != B_OK)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
Volume::Unmount()
{
	TRACE("Volume::Unmount(): Unmounting");

	TRACE("Volume::Unmount(): Closing device");
	close(fDevice);

	return B_OK;
}
