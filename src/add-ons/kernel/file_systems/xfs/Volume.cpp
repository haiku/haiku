/*
 * Copyright 2001 - 2017, Axel Dörfler, axeld @pinc - software.de.
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Volume.h"


class DeviceOpener
{
public:
							DeviceOpener(int fd, int mode);
							DeviceOpener(const char *device, int mode);
							~DeviceOpener();

			int				Open(const char *device, int mode);
			int				Open(int fd, int mode);
			void*			InitCache(off_t numBlocks, uint32 blockSize);
			void			RemoveCache(bool allowWrites);

			void			Keep();

			int				Device() const { return fDevice; }
			int				Mode() const { return fMode; }
			bool			IsReadOnly() const
								{ return _IsReadOnly(fMode); }

			status_t		GetSize(off_t* _size, uint32* _blockSize = NULL);

private:
	static	bool 			_IsReadOnly(int mode)
								{ return (mode & O_RWMASK) == O_RDONLY; }
	static	bool			_IsReadWrite(int mode)
								{ return (mode & O_RWMASK) == O_RDWR; }

			int				fDevice;
			int				fMode;
			void*			fBlockCache;
};


DeviceOpener::DeviceOpener(const char *device, int mode)
	: fBlockCache(NULL)
{
	Open(device, mode);
}


DeviceOpener::DeviceOpener(int fd, int mode)
	: fBlockCache(NULL)
{
	Open(fd, mode);
}


DeviceOpener::~DeviceOpener()
{
	if (fDevice >= 0) {
		RemoveCache(false);
		close(fDevice);
	}
}


int
DeviceOpener::Open(const char *device, int mode)
{
	fDevice = open(device, mode | O_NOCACHE);
	if (fDevice < 0)
		fDevice = errno;

	if (fDevice < 0 && _IsReadWrite(mode)){
		// try again to open read-only (don't rely on a specific error code)
		return Open(device, O_RDONLY | O_NOCACHE);
	}

	if (fDevice >= 0) {
		// opening succeeded
		fMode = mode;
		if (_IsReadWrite(mode)) {
			// check out if the device really allows for read/write access
			device_geometry geometry;
			if (!ioctl(fDevice, B_GET_GEOMETRY, &geometry)) {

				if (geometry.read_only) {
					// reopen device read-only
					close(fDevice);
					return Open(device, O_RDONLY | O_NOCACHE);
				}
			}
		}
	}

	return fDevice;
}


int
DeviceOpener::Open(int fd, int mode)
{
	fDevice = dup(fd);
	if (fDevice < 0)
		return errno;

	fMode = mode;

	return fDevice;
}


void*
DeviceOpener::InitCache(off_t numBlocks, uint32 blockSize)
{
	return fBlockCache = block_cache_create(fDevice, numBlocks, blockSize,
											IsReadOnly());
}


void
DeviceOpener::RemoveCache(bool allowWrites)
{
	if (fBlockCache == NULL)
		return;

	block_cache_delete(fBlockCache, allowWrites);
	fBlockCache = NULL;
}


void
DeviceOpener::Keep()
{
	fDevice = -1;
}


/*!	Returns the size of the device in bytes. It uses B_GET_GEOMETRY
	to compute the size, or fstat() if that failed.
*/
status_t
DeviceOpener::GetSize(off_t *_size, uint32 *_blockSize)
{
	device_geometry geometry;
	if (ioctl(fDevice, B_GET_GEOMETRY, &geometry) < 0) {
		// maybe it's just a file
		struct stat stat;
		if (fstat(fDevice, &stat) < 0)
			return B_ERROR;

		if (_size)
			*_size = stat.st_size;
		if (_blockSize)				// that shouldn't cause us any problems
			*_blockSize = 512;

		return B_OK;
	}

	if (_size) {
		*_size = 1LL * geometry.head_count * geometry.cylinder_count
					* geometry.sectors_per_track * geometry.bytes_per_sector;
	}
	if (_blockSize)
		*_blockSize = geometry.bytes_per_sector;

	return B_OK;
}


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
Volume::IsValidSuperBlock()
{
	return fSuperBlock.IsValid();
}


status_t
Volume::Identify(int fd, XfsSuperBlock *superBlock)
{

	TRACE("Volume::Identify() : Identifying Volume in progress");

	if (read_pos(fd, 0, superBlock, sizeof(XfsSuperBlock))
		!= sizeof(XfsSuperBlock))
			return B_IO_ERROR;

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

	TRACE("Volume::Mount(): Valid SuperBlock.\n");

	// check if the device size is large enough to hold the file system
	off_t diskSize;
	if (opener.GetSize(&diskSize) != B_OK) {
		ERROR("Volume:Mount() Unable to get diskSize");
		return B_ERROR;
	}

	opener.Keep();
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
