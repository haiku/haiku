/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Volume.h"

#include "DeviceOpener.h"

//#define TRACE_UFS2
#ifdef TRACE_UFS2
#	define TRACE(x...) dprintf("\33[34mufs2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#   define ERROR(x...) dprintf("\33[34mexfat:\33[0m " x)


bool
ufs2_super_block::IsValid()
{
	if (ufs2_magic != UFS2_SUPER_BLOCK_MAGIC)
		return false;

	return true;
}


bool
Volume::IsValidSuperBlock()
{
	return fSuperBlock.IsValid();
}


Volume::Volume(fs_volume *volume)
    : fFSVolume(volume)
{
	fFlags = 0;
	mutex_init(&fLock, "ufs2 volume");
	TRACE("Volume::Volume() : Initialising volume");
}


Volume::~Volume()
{
	mutex_destroy(&fLock);
	TRACE("Volume::Destructor : Removing Volume");
}


status_t
Volume::Identify(int fd, ufs2_super_block *superBlock)
{
	if (read_pos(fd, UFS2_SUPER_BLOCK_OFFSET, superBlock,
		sizeof(ufs2_super_block)) != sizeof(ufs2_super_block))
		return B_IO_ERROR;

	if (!superBlock->IsValid()) {
		ERROR("invalid superblock! Identify failed!!\n");
		return B_BAD_VALUE;
	}

	return B_OK;
}


status_t
Volume::Mount(const char *deviceName, uint32 flags)
{
	TRACE("Mounting volume... Please wait.\n");
	flags |= B_MOUNT_READ_ONLY;
	if ((flags & B_MOUNT_READ_ONLY) != 0)
	{
		TRACE("Volume is read only\n");
	}
	else
	{
		TRACE("Volume is read write\n");
	}

	DeviceOpener opener(deviceName, (flags & B_MOUNT_READ_ONLY) != 0 
									? O_RDONLY:O_RDWR);
	fDevice = opener.Device();
	if (fDevice < B_OK) {
		ERROR("Could not open device\n");
		return fDevice;
	}

	if (opener.IsReadOnly())
		fFlags |= VOLUME_READ_ONLY;

	status_t status = Identify(fDevice, &fSuperBlock);
	if (status != B_OK) {
		ERROR("Invalid super block\n");
		return status;
	}

	TRACE("Valid super block\n");

	opener.Keep();
	return B_OK;

}


status_t
Volume::Unmount()
{
	TRACE("Unmounting the volume");

	TRACE("Closing device");
	close(fDevice);

	return B_OK;
}
