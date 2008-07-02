/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */

//! super block, mounting, etc.


#include "Volume.h"

#include <errno.h>
#include <new>
#include <stdlib.h>

#include <fs_cache.h>
#include <fs_volume.h>

#include <util/AutoLock.h>

#include "Inode.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


class DeviceOpener {
	public:
		DeviceOpener(int fd, int mode);
		DeviceOpener(const char *device, int mode);
		~DeviceOpener();

		int Open(const char *device, int mode);
		int Open(int fd, int mode);
		void *InitCache(off_t numBlocks, uint32 blockSize);
		void RemoveCache(bool allowWrites);

		void Keep();

		int Device() const { return fDevice; }
		int Mode() const { return fMode; }
		bool IsReadOnly() const { return _IsReadOnly(fMode); }

		status_t GetSize(off_t *_size, uint32 *_blockSize = NULL);

	private:
		static bool _IsReadOnly(int mode)
			{ return (mode & O_RWMASK) == O_RDONLY;}
		static bool _IsReadWrite(int mode)
			{ return (mode & O_RWMASK) == O_RDWR;}

		int		fDevice;
		int		fMode;
		void	*fBlockCache;
};


DeviceOpener::DeviceOpener(const char *device, int mode)
	:
	fBlockCache(NULL)
{
	Open(device, mode);
}


DeviceOpener::DeviceOpener(int fd, int mode)
	:
	fBlockCache(NULL)
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

	if (fDevice < 0 && _IsReadWrite(mode)) {
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


void *
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
		if (_blockSize)	// that shouldn't cause us any problems
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


//	#pragma mark -


bool
ext2_super_block::IsValid()
{
	// TODO: check some more values!
	if (Magic() != (uint32)EXT2_SUPER_BLOCK_MAGIC)
		return false;

	return true;
}


//	#pragma mark -


Volume::Volume(fs_volume* volume)
	:
	fFSVolume(volume),
	fFlags(0),
	fGroupBlocks(NULL),
	fRootNode(NULL)
{
	mutex_init(&fLock, "ext2 volume");
}


Volume::~Volume()
{
	if (fGroupBlocks != NULL) {
		uint32 blockCount = (fNumGroups + fGroupsPerBlock - 1)
			/ fGroupsPerBlock;
		for (uint32 i = 0; i < blockCount; i++) {
			free(fGroupBlocks[i]);
		}

		free(fGroupBlocks);
	}
}


bool
Volume::IsValidSuperBlock()
{
	return fSuperBlock.IsValid();
}


const char*
Volume::Name() const
{
	if (fSuperBlock.name[0])
		return fSuperBlock.name;

	return "Unnamed Ext2 Volume";
}


status_t
Volume::Mount(const char* deviceName, uint32 flags)
{
	flags |= B_MOUNT_READ_ONLY;
		// we only support read-only for now

	DeviceOpener opener(deviceName, (flags & B_MOUNT_READ_ONLY) != 0
		? O_RDONLY : O_RDWR);
	fDevice = opener.Device();
	if (fDevice < B_OK)
		return fDevice;

	if (opener.IsReadOnly())
		fFlags |= VOLUME_READ_ONLY;

	// read the super block
	if (Identify(fDevice, &fSuperBlock) != B_OK) {
		//FATAL(("invalid super block!\n"));
		return B_BAD_VALUE;
	}

	if ((fSuperBlock.IncompatibleFeatures()
			& EXT2_INCOMPATIBLE_FEATURE_COMPRESSION) != 0) {
		dprintf("ext2: compression not supported.\n");
		return B_NOT_SUPPORTED;
	}

	// initialize short hands to the super block (to save byte swapping)
	fBlockShift = fSuperBlock.BlockShift();
	fBlockSize = 1UL << fSuperBlock.BlockShift();
	fFirstDataBlock = fSuperBlock.FirstDataBlock();

	fNumGroups = (fSuperBlock.NumBlocks() - fFirstDataBlock - 1)
		/ fSuperBlock.BlocksPerGroup() + 1;
	fGroupsPerBlock = fBlockSize / sizeof(ext2_block_group);

	TRACE("block size %ld, num groups %ld, groups per block %ld, first %lu\n",
		fBlockSize, fNumGroups, fGroupsPerBlock, fFirstDataBlock);
	TRACE("features %lx, incompatible features %lx, read-only features %lx\n",
		fSuperBlock.CompatibleFeatures(), fSuperBlock.IncompatibleFeatures(),
		fSuperBlock.ReadOnlyFeatures());

	uint32 blockCount = (fNumGroups + fGroupsPerBlock - 1) / fGroupsPerBlock;

	fGroupBlocks = (ext2_block_group**)malloc(blockCount * sizeof(void*));
	if (fGroupBlocks == NULL)
		return B_NO_MEMORY;

	memset(fGroupBlocks, 0, blockCount * sizeof(void*));
	fInodesPerBlock = fBlockSize / sizeof(ext2_inode);

	// check if the device size is large enough to hold the file system
	off_t diskSize;
	status_t status = opener.GetSize(&diskSize);
	if (status != B_OK)
		return status;
	if (diskSize < (NumBlocks() << BlockShift()))
		return B_BAD_VALUE;

	if ((fBlockCache = opener.InitCache(NumBlocks(), fBlockSize)) == NULL)
		return B_ERROR;

	status = get_vnode(fFSVolume, EXT2_ROOT_NODE, (void**)&fRootNode);
	if (status == B_OK) {
		// all went fine
		opener.Keep();
		return B_OK;
	} else {
		TRACE("could not create root node: get_vnode() failed!\n");
	}

	return status;
}


status_t
Volume::Unmount()
{
	//put_vnode(fVolume, ToVnode(Root()));
	//block_cache_delete(fBlockCache, !IsReadOnly());
	close(fDevice);

	return B_OK;
}


status_t
Volume::GetInodeBlock(ino_t id, uint32& block)
{
	ext2_block_group* group;
	status_t status = GetBlockGroup((id - 1) / fSuperBlock.InodesPerGroup(),
		&group);
	if (status != B_OK)
		return status;

	block = group->InodeTable()
		+ ((id - 1) % fSuperBlock.InodesPerGroup()) / fInodesPerBlock;
	return B_OK;
}


uint32
Volume::InodeBlockIndex(ino_t id) const
{
	return ((id - 1) % fSuperBlock.InodesPerGroup()) % fInodesPerBlock;
}


off_t
Volume::_GroupBlockOffset(uint32 blockIndex)
{
	if ((fSuperBlock.IncompatibleFeatures()
			& EXT2_INCOMPATIBLE_FEATURE_META_GROUP) == 0
		|| blockIndex < fSuperBlock.FirstMetaBlockGroup())
		return off_t(fFirstDataBlock + blockIndex + 1) << fBlockShift;

	panic("meta block");
	return 0;
}


/*!	Makes the requested block group available.
	The block groups are loaded on demand, but are kept in memory until the
	volume is unmounted; therefore we don't use the block cache.
*/
status_t
Volume::GetBlockGroup(int32 index, ext2_block_group** _group)
{
	if (index < 0 || (uint32)index > fNumGroups)
		return B_BAD_VALUE;

	int32 blockIndex = index / fGroupsPerBlock;

	MutexLocker _(fLock);

	if (fGroupBlocks[blockIndex] == NULL) {
		ext2_block_group* groupBlock = (ext2_block_group*)malloc(fBlockSize);
		if (groupBlock == NULL)
			return B_NO_MEMORY;

		ssize_t bytesRead = read_pos(fDevice, _GroupBlockOffset(blockIndex),
			groupBlock, fBlockSize);
		if (bytesRead >= B_OK && (uint32)bytesRead != fBlockSize)
			bytesRead = B_IO_ERROR;
		if (bytesRead < B_OK) {
			free(groupBlock);
			return bytesRead;
		}

		fGroupBlocks[blockIndex] = groupBlock;

		TRACE("group [%ld]: inode table %ld\n", index,
			(fGroupBlocks[blockIndex] + index % fGroupsPerBlock)->InodeTable());
	}

	*_group = fGroupBlocks[blockIndex] + index % fGroupsPerBlock;
	return B_OK;
}


//	#pragma mark - Disk scanning and initialization


/*static*/ status_t
Volume::Identify(int fd, ext2_super_block* superBlock)
{
	if (read_pos(fd, EXT2_SUPER_BLOCK_OFFSET, superBlock,
			sizeof(ext2_super_block)) != sizeof(ext2_super_block))
		return B_IO_ERROR;

	if (!superBlock->IsValid())
		return B_BAD_VALUE;

	return B_OK;
}

