/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


//! Superblock, mounting, etc.


#include "Volume.h"

#include <errno.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs_cache.h>
#include <fs_volume.h>

#include <util/AutoLock.h>

#include "BPlusTree.h"
#include "CachedBlock.h"
#include "Chunk.h"
#include "Inode.h"


//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#	define ERROR(x...) dprintf("\33[34mbtrfs:\33[0m " x)


class DeviceOpener {
public:
								DeviceOpener(int fd, int mode);
								DeviceOpener(const char* device, int mode);
								~DeviceOpener();

			int					Open(const char* device, int mode);
			int					Open(int fd, int mode);
			void*				InitCache(off_t numBlocks, uint32 blockSize);
			void				RemoveCache(bool allowWrites);

			void				Keep();

			int					Device() const { return fDevice; }
			int					Mode() const { return fMode; }
			bool				IsReadOnly() const
									{ return _IsReadOnly(fMode); }

			status_t			GetSize(off_t* _size, uint32* _blockSize = NULL);

private:
	static	bool				_IsReadOnly(int mode)
									{ return (mode & O_RWMASK) == O_RDONLY;}
	static	bool				_IsReadWrite(int mode)
									{ return (mode & O_RWMASK) == O_RDWR;}

			int					fDevice;
			int					fMode;
			void*				fBlockCache;
};


DeviceOpener::DeviceOpener(const char* device, int mode)
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
DeviceOpener::Open(const char* device, int mode)
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
DeviceOpener::GetSize(off_t* _size, uint32* _blockSize)
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
		*_size = 1ULL * geometry.head_count * geometry.cylinder_count
			* geometry.sectors_per_track * geometry.bytes_per_sector;
	}
	if (_blockSize)
		*_blockSize = geometry.bytes_per_sector;

	return B_OK;
}


//	#pragma mark -


bool
btrfs_super_block::IsValid()
{
	// TODO: check some more values!
	if (strncmp(magic, BTRFS_SUPER_BLOCK_MAGIC, sizeof(magic)) != 0)
		return false;
	
	return true;
}


//	#pragma mark -


Volume::Volume(fs_volume* volume)
	:
	fFSVolume(volume),
	fFlags(0),
	fChunk(NULL),
	fChunkTree(NULL)
{
	mutex_init(&fLock, "btrfs volume");
}


Volume::~Volume()
{
	TRACE("Volume destructor.\n");
}


bool
Volume::IsValidSuperBlock()
{
	return fSuperBlock.IsValid();
}


const char*
Volume::Name() const
{
	if (fSuperBlock.label[0])
		return fSuperBlock.label;

	return fName;
}


status_t
Volume::Mount(const char* deviceName, uint32 flags)
{
	flags |= B_MOUNT_READ_ONLY;
		// we only support read-only for now
	
	if ((flags & B_MOUNT_READ_ONLY) != 0) {
		TRACE("Volume::Mount(): Read only\n");
	} else {
		TRACE("Volume::Mount(): Read write\n");
	}

	DeviceOpener opener(deviceName, (flags & B_MOUNT_READ_ONLY) != 0
		? O_RDONLY : O_RDWR);
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
		ERROR("Volume::Mount(): Identify() failed\n");
		return status;
	}
	
	fBlockSize = fSuperBlock.BlockSize();
	TRACE("block size %ld\n", fBlockSize);

	uint8* start = (uint8*)&fSuperBlock.system_chunk_array[0];
	uint8* end = (uint8*)&fSuperBlock.system_chunk_array[2048];
	while (start < end) {
		struct btrfs_key* key = (struct btrfs_key*)start;
		TRACE("system_chunk_array object_id 0x%llx offset 0x%llx type 0x%x\n",
			key->ObjectID(), key->Offset(), key->Type());
		if (key->Type() != BTRFS_KEY_TYPE_CHUNK_ITEM) {
			break;
		}
		
		struct btrfs_chunk* chunk = (struct btrfs_chunk*)(key + 1);
		fChunk = new(std::nothrow) Chunk(chunk, key->Offset());
		if (fChunk == NULL)
			return B_ERROR;
		start += sizeof(struct btrfs_key) + fChunk->Size();
	}

	TRACE("Volume::Mount() generation: %lld\n",	fSuperBlock.Generation());
	fsblock_t physical = 0;
	FindBlock(fSuperBlock.Root(), physical);
	TRACE("Volume::Mount() root: %lld (physical %lld)\n",
		fSuperBlock.Root(), physical);
	FindBlock(fSuperBlock.ChunkRoot(), physical);
	TRACE("Volume::Mount() chunk_root: %lld (physical %lld)\n",
		fSuperBlock.ChunkRoot(), physical);
	FindBlock(fSuperBlock.LogRoot(), physical);
	TRACE("Volume::Mount() log_root: %lld (physical %lld)\n",
		fSuperBlock.LogRoot(), physical);
		
	// check if the device size is large enough to hold the file system
	off_t diskSize;
	status = opener.GetSize(&diskSize);
	if (status != B_OK)
		return status;
	if (diskSize < (off_t)fSuperBlock.TotalSize())
		return B_BAD_VALUE;

	fBlockCache = opener.InitCache(fSuperBlock.TotalSize() / fBlockSize,
		fBlockSize);
	if (fBlockCache == NULL)
		return B_ERROR;
	
	TRACE("Volume::Mount(): Initialized block cache: %p\n", fBlockCache);

	fChunkTree = new(std::nothrow) BPlusTree(this, fSuperBlock.ChunkRoot());
	if (fChunkTree == NULL)
		return B_NO_MEMORY;

	FindBlock(fSuperBlock.Root(), physical);
	TRACE("Volume::Mount() root: %lld (physical %lld)\n",
		fSuperBlock.Root(), physical);
	FindBlock(fSuperBlock.ChunkRoot(), physical);
	TRACE("Volume::Mount() chunk_root: %lld (physical %lld)\n",
		fSuperBlock.ChunkRoot(), physical);
	FindBlock(fSuperBlock.LogRoot(), physical);
	TRACE("Volume::Mount() log_root: %lld (physical %lld)\n",
		fSuperBlock.LogRoot(), physical);

	fRootTree = new(std::nothrow) BPlusTree(this, fSuperBlock.Root());
	if (fRootTree == NULL)
		return B_NO_MEMORY;
	TRACE("Volume::Mount(): Searching extent root\n");
	struct btrfs_key search_key;
	search_key.SetOffset(0);
	search_key.SetType(BTRFS_KEY_TYPE_ROOT_ITEM);
	search_key.SetObjectID(BTRFS_OBJECT_ID_EXTENT_TREE);
	struct btrfs_root *root;
	if (fRootTree->FindNext(search_key, (void**)&root) != B_OK) {
		ERROR("Volume::Mount(): Couldn't find extent root\n");
		return B_ERROR;
	}
	TRACE("Volume::Mount(): Found extent root: %lld\n", root->BlockNum());
	fExtentTree = new(std::nothrow) BPlusTree(this, root->BlockNum());
	free(root);
	if (fExtentTree == NULL)
		return B_NO_MEMORY;
	
	search_key.SetOffset(0);
	search_key.SetObjectID(BTRFS_OBJECT_ID_FS_TREE);
	if (fRootTree->FindNext(search_key, (void**)&root) != B_OK) {
		ERROR("Volume::Mount(): Couldn't find fs root\n");
		return B_ERROR;
	}
	TRACE("Volume::Mount(): Found fs root: %lld\n", root->BlockNum());
	fFSTree = new(std::nothrow) BPlusTree(this, root->BlockNum());
	free(root);
	if (fFSTree == NULL)
		return B_NO_MEMORY;
	
	search_key.SetOffset(0);
	search_key.SetObjectID(BTRFS_OBJECT_ID_DEV_TREE);
	if (fRootTree->FindNext(search_key, (void**)&root) != B_OK) {
		ERROR("Volume::Mount(): Couldn't find dev root\n");
		return B_ERROR;
	}
	TRACE("Volume::Mount(): Found dev root: %lld\n", root->BlockNum());
	fDevTree = new(std::nothrow) BPlusTree(this, root->BlockNum());
	free(root);
	if (fDevTree == NULL)
		return B_NO_MEMORY;

	search_key.SetOffset(0);
	search_key.SetObjectID(BTRFS_OBJECT_ID_CHECKSUM_TREE);
	if (fRootTree->FindNext(search_key, (void**)&root) != B_OK) {
		ERROR("Volume::Mount(): Couldn't find checksum root\n");
		return B_ERROR;
	}
	TRACE("Volume::Mount(): Found checksum root: %lld\n", root->BlockNum());
	fChecksumTree = new(std::nothrow) BPlusTree(this, root->BlockNum());
	free(root);
	if (fChecksumTree == NULL)
		return B_NO_MEMORY;
	
	// ready
	status = get_vnode(fFSVolume, BTRFS_OBJECT_ID_CHUNK_TREE,
		(void**)&fRootNode);
	if (status != B_OK) {
		ERROR("could not create root node: get_vnode() failed!\n");
		return status;
	}

	TRACE("Volume::Mount(): Found root node: %lld (%s)\n", fRootNode->ID(),
		strerror(fRootNode->InitCheck()));

	// all went fine
	opener.Keep();

	if (!fSuperBlock.label[0]) {
		// generate a more or less descriptive volume name
		off_t divisor = 1ULL << 40;
		char unit = 'T';
		if (diskSize < divisor) {
			divisor = 1UL << 30;
			unit = 'G';
			if (diskSize < divisor) {
				divisor = 1UL << 20;
				unit = 'M';
			}
		}

		double size = double((10 * diskSize + divisor - 1) / divisor);
			// %g in the kernel does not support precision...

		snprintf(fName, sizeof(fName), "%g %cB Btrfs Volume",
			size / 10, unit);
	}

	return B_OK;
}


status_t
Volume::Unmount()
{
	TRACE("Volume::Unmount()\n");
	delete fExtentTree;
	delete fChecksumTree;
	delete fFSTree;
	delete fDevTree;
	fExtentTree = NULL;
	fChecksumTree = NULL;
	fFSTree = NULL;
	fDevTree = NULL;

	TRACE("Volume::Unmount(): Putting root node\n");
	put_vnode(fFSVolume, RootNode()->ID());
	TRACE("Volume::Unmount(): Deleting the block cache\n");
	block_cache_delete(fBlockCache, !IsReadOnly());
	TRACE("Volume::Unmount(): Closing device\n");
	close(fDevice);

	TRACE("Volume::Unmount(): Done\n");
	return B_OK;
}


status_t
Volume::LoadSuperBlock()
{
	CachedBlock cached(this);
	const uint8* block = cached.SetTo(BTRFS_SUPER_BLOCK_OFFSET / fBlockSize);

	if (block == NULL)
		return B_IO_ERROR;

	memcpy(&fSuperBlock, block + BTRFS_SUPER_BLOCK_OFFSET % fBlockSize,
		sizeof(fSuperBlock));

	return B_OK;
}


status_t
Volume::FindBlock(off_t logical, fsblock_t &physicalBlock)
{
	off_t physical;
	status_t status = FindBlock(logical, physical);
	if (status != B_OK)
		return status;
	physicalBlock = physical / fBlockSize;
	return B_OK;
}


status_t
Volume::FindBlock(off_t logical, off_t &physical)
{
	if (fChunkTree == NULL
		|| (logical >= fChunk->Offset() && logical < fChunk->End())) {
		// try with fChunk
		return fChunk->FindBlock(logical, physical);
	}

	struct btrfs_key search_key;
	search_key.SetOffset(logical);
	search_key.SetType(BTRFS_KEY_TYPE_CHUNK_ITEM);
	search_key.SetObjectID(BTRFS_OBJECT_ID_CHUNK_TREE);
	struct btrfs_chunk *chunk;
	size_t chunk_length;
	status_t status = fChunkTree->FindPrevious(search_key, (void**)&chunk,
		&chunk_length);
	if (status != B_OK)
		return status;

	Chunk _chunk(chunk, search_key.Offset());
	free(chunk);
	status = _chunk.FindBlock(logical, physical);
	if (status != B_OK)
			return status;
	TRACE("Volume::FindBlock(): logical: %lld, physical: %lld\n", logical,
		physical);
	return B_OK;
}


//	#pragma mark - Disk scanning and initialization


/*static*/ status_t
Volume::Identify(int fd, btrfs_super_block* superBlock)
{
	if (read_pos(fd, BTRFS_SUPER_BLOCK_OFFSET, superBlock,
			sizeof(btrfs_super_block)) != sizeof(btrfs_super_block))
		return B_IO_ERROR;

	if (!superBlock->IsValid()) {
		ERROR("invalid superblock!\n");
		return B_BAD_VALUE;
	}

	return B_OK;
}

