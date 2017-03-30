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

#include "CachedBlock.h"
#include "CRCTable.h"
#include "Inode.h"
#include "InodeJournal.h"
#include "NoJournal.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#	define FATAL(x...) dprintf("\33[34mext2:\33[0m " x)


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
			bool				IsReadOnly() const { return _IsReadOnly(fMode); }

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
ext2_super_block::IsValid()
{
	if (Magic() != (uint32)EXT2_SUPER_BLOCK_MAGIC
			|| BlockShift() > 16
			|| BlocksPerGroup() != (1UL << BlockShift()) * 8
			|| InodeSize() > (1UL << BlockShift())
			|| RevisionLevel() > EXT2_MAX_REVISION
			|| ReservedGDTBlocks() > (1UL << BlockShift()) / 4) {
		return false;
	}
	
	return true;
}


//	#pragma mark -


Volume::Volume(fs_volume* volume)
	:
	fFSVolume(volume),
	fBlockAllocator(NULL),
	fInodeAllocator(this),
	fJournalInode(NULL),
	fFlags(0),
	fGroupBlocks(NULL),
	fRootNode(NULL)
{
	mutex_init(&fLock, "ext2 volume");
}


Volume::~Volume()
{
	TRACE("Volume destructor.\n");
	delete fBlockAllocator;
	if (fGroupBlocks != NULL) {
		uint32 blockCount = (fNumGroups + fGroupsPerBlock - 1)
			/ fGroupsPerBlock;
		for (uint32 i = 0; i < blockCount; i++)
			free(fGroupBlocks[i]);

		free(fGroupBlocks);
	}
}


bool
Volume::IsValidSuperBlock()
{
	return fSuperBlock.IsValid();
}


bool
Volume::HasExtendedAttributes() const
{
	return (fSuperBlock.CompatibleFeatures() & EXT2_FEATURE_EXT_ATTR) != 0;
}


const char*
Volume::Name() const
{
	if (fSuperBlock.name[0])
		return fSuperBlock.name;

	return fName;
}


void
Volume::SetName(const char* name)
{
	strlcpy(fSuperBlock.name, name, sizeof(fSuperBlock.name));
}


status_t
Volume::Mount(const char* deviceName, uint32 flags)
{
	// flags |= B_MOUNT_READ_ONLY;
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
		FATAL("Volume::Mount(): couldn't open device\n");
		return fDevice;
	}

	if (opener.IsReadOnly())
		fFlags |= VOLUME_READ_ONLY;

	TRACE("features %" B_PRIx32 ", incompatible features %" B_PRIx32
		", read-only features %" B_PRIx32 "\n",
		fSuperBlock.CompatibleFeatures(), fSuperBlock.IncompatibleFeatures(),
		fSuperBlock.ReadOnlyFeatures());

	// read the superblock
	status_t status = Identify(fDevice, &fSuperBlock);
	if (status != B_OK) {
		FATAL("Volume::Mount(): Identify() failed\n");
		return status;
	}
	
	// check read-only features if mounting read-write
	if (!IsReadOnly() && _UnsupportedReadOnlyFeatures(fSuperBlock) != 0)
		return B_UNSUPPORTED;

	// initialize short hands to the superblock (to save byte swapping)
	fBlockShift = fSuperBlock.BlockShift();
	if (fBlockShift < 10 || fBlockShift > 16)
		return B_ERROR;
	fBlockSize = 1UL << fBlockShift;
	fFirstDataBlock = fSuperBlock.FirstDataBlock();

	fFreeBlocks = fSuperBlock.FreeBlocks(Has64bitFeature());
	fFreeInodes = fSuperBlock.FreeInodes();

	off_t numBlocks = fSuperBlock.NumBlocks(Has64bitFeature()) - fFirstDataBlock;
	uint32 blocksPerGroup = fSuperBlock.BlocksPerGroup();
	fNumGroups = numBlocks / blocksPerGroup;
	if (numBlocks % blocksPerGroup != 0)
		fNumGroups++;

	if (Has64bitFeature()) {
		fGroupDescriptorSize = fSuperBlock.GroupDescriptorSize();
		if (fGroupDescriptorSize < sizeof(ext2_block_group))
			return B_ERROR;
	} else
		fGroupDescriptorSize = EXT2_BLOCK_GROUP_NORMAL_SIZE;
	fGroupsPerBlock = fBlockSize / fGroupDescriptorSize;
	fNumInodes = fSuperBlock.NumInodes();

	TRACE("block size %" B_PRIu32 ", num groups %" B_PRIu32 ", groups per "
		"block %" B_PRIu32 ", first %" B_PRIu32 "\n", fBlockSize, fNumGroups,
		fGroupsPerBlock, fFirstDataBlock);
	
	uint32 blockCount = (fNumGroups + fGroupsPerBlock - 1) / fGroupsPerBlock;

	fGroupBlocks = (uint8**)malloc(blockCount * sizeof(uint8*));
	if (fGroupBlocks == NULL)
		return B_NO_MEMORY;

	memset(fGroupBlocks, 0, blockCount * sizeof(uint8*));
	fInodesPerBlock = fBlockSize / InodeSize();

	// check if the device size is large enough to hold the file system
	off_t diskSize;
	status = opener.GetSize(&diskSize);
	if (status != B_OK)
		return status;
	if (diskSize < ((off_t)NumBlocks() << BlockShift()))
		return B_BAD_VALUE;

	fBlockCache = opener.InitCache(NumBlocks(), fBlockSize);
	if (fBlockCache == NULL)
		return B_ERROR;
	
	TRACE("Volume::Mount(): Initialized block cache: %p\n", fBlockCache);

	// initialize journal if mounted read-write
	if (!IsReadOnly() &&
		(fSuperBlock.CompatibleFeatures() & EXT2_FEATURE_HAS_JOURNAL) != 0) {
		// TODO: There should be a mount option to ignore the existent journal
		if (fSuperBlock.JournalInode() != 0) {
			fJournalInode = new(std::nothrow) Inode(this, 
				fSuperBlock.JournalInode());

			if (fJournalInode == NULL)
				return B_NO_MEMORY;

			TRACE("Opening an on disk, inode mapped journal.\n");
			fJournal = new(std::nothrow) InodeJournal(fJournalInode);
		} else {
			// TODO: external journal
			TRACE("Can not open an external journal.\n");
			return B_UNSUPPORTED;
		}
	} else {
		TRACE("Opening a fake journal (NoJournal).\n");
		fJournal = new(std::nothrow) NoJournal(this);
	}

	if (fJournal == NULL) {
		TRACE("No memory to create the journal\n");
		return B_NO_MEMORY;
	}

	TRACE("Volume::Mount(): Checking if journal was initialized\n");
	status = fJournal->InitCheck();
	if (status != B_OK) {
		FATAL("could not initialize journal!\n");
		return status;
	}

	// TODO: Only recover if asked to
	TRACE("Volume::Mount(): Asking journal to recover\n");
	status = fJournal->Recover();
	if (status != B_OK) {
		FATAL("could not recover journal!\n");
		return status;
	}

	TRACE("Volume::Mount(): Restart journal log\n");
	status = fJournal->StartLog();
	if (status != B_OK) {
		FATAL("could not initialize start journal!\n");
		return status;
	}

	if (!IsReadOnly()) {
		// Initialize allocators
		fBlockAllocator = new(std::nothrow) BlockAllocator(this);
		if (fBlockAllocator != NULL) {
			TRACE("Volume::Mount(): Initialize block allocator\n");
			status = fBlockAllocator->Initialize();
		}
		if (fBlockAllocator == NULL || status != B_OK) {
			delete fBlockAllocator;
			fBlockAllocator = NULL;
			FATAL("could not initialize block allocator, going read-only!\n");
			fFlags |= VOLUME_READ_ONLY;
			fJournal->Uninit();
			delete fJournal;
			delete fJournalInode;
			fJournalInode = NULL;
			fJournal = new(std::nothrow) NoJournal(this);
		}
	}

	// ready
	status = get_vnode(fFSVolume, EXT2_ROOT_NODE, (void**)&fRootNode);
	if (status != B_OK) {
		FATAL("could not create root node: get_vnode() failed!\n");
		return status;
	}

	// all went fine
	opener.Keep();

	if (!fSuperBlock.name[0]) {
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

		snprintf(fName, sizeof(fName), "%g %cB Ext2 Volume",
			size / 10, unit);
	}

	return B_OK;
}


status_t
Volume::Unmount()
{
	TRACE("Volume::Unmount()\n");

	status_t status = fJournal->Uninit();

	delete fJournal;
	delete fJournalInode;

	TRACE("Volume::Unmount(): Putting root node\n");
	put_vnode(fFSVolume, RootNode()->ID());
	TRACE("Volume::Unmount(): Deleting the block cache\n");
	block_cache_delete(fBlockCache, !IsReadOnly());
	TRACE("Volume::Unmount(): Closing device\n");
	close(fDevice);

	TRACE("Volume::Unmount(): Done\n");
	return status;
}


status_t
Volume::GetInodeBlock(ino_t id, off_t& block)
{
	ext2_block_group* group;
	status_t status = GetBlockGroup((id - 1) / fSuperBlock.InodesPerGroup(),
		&group);
	if (status != B_OK)
		return status;

	block = group->InodeTable(Has64bitFeature())
		+ ((id - 1) % fSuperBlock.InodesPerGroup()) / fInodesPerBlock;
	return B_OK;
}


uint32
Volume::InodeBlockIndex(ino_t id) const
{
	return ((id - 1) % fSuperBlock.InodesPerGroup()) % fInodesPerBlock;
}


/*static*/ uint32
Volume::_UnsupportedIncompatibleFeatures(ext2_super_block& superBlock)
{
	uint32 supportedIncompatible = EXT2_INCOMPATIBLE_FEATURE_FILE_TYPE
		| EXT2_INCOMPATIBLE_FEATURE_RECOVER
		| EXT2_INCOMPATIBLE_FEATURE_JOURNAL
		| EXT2_INCOMPATIBLE_FEATURE_EXTENTS
		| EXT2_INCOMPATIBLE_FEATURE_FLEX_GROUP;
		/*| EXT2_INCOMPATIBLE_FEATURE_META_GROUP*/;
	uint32 unsupported = superBlock.IncompatibleFeatures() 
		& ~supportedIncompatible;

	if (unsupported != 0) {
		FATAL("ext2: incompatible features not supported: %" B_PRIx32
			" (extents %x)\n", unsupported, EXT2_INCOMPATIBLE_FEATURE_EXTENTS);
	}

	return unsupported;
}


/*static*/ uint32
Volume::_UnsupportedReadOnlyFeatures(ext2_super_block& superBlock)
{
	uint32 supportedReadOnly = EXT2_READ_ONLY_FEATURE_SPARSE_SUPER
		| EXT2_READ_ONLY_FEATURE_LARGE_FILE
		| EXT2_READ_ONLY_FEATURE_HUGE_FILE
		| EXT2_READ_ONLY_FEATURE_EXTRA_ISIZE
		| EXT2_READ_ONLY_FEATURE_DIR_NLINK
		| EXT2_READ_ONLY_FEATURE_GDT_CSUM;
	// TODO actually implement EXT2_READ_ONLY_FEATURE_SPARSE_SUPER when
	// implementing superblock backup copies

	uint32 unsupported = superBlock.ReadOnlyFeatures() & ~supportedReadOnly;

	if (unsupported != 0) {
		FATAL("ext2: readonly features not supported: %" B_PRIx32 "\n",
			unsupported);
	}

	return unsupported;
}


uint32
Volume::_GroupDescriptorBlock(uint32 blockIndex)
{
	if ((fSuperBlock.IncompatibleFeatures()
			& EXT2_INCOMPATIBLE_FEATURE_META_GROUP) == 0
		|| blockIndex < fSuperBlock.FirstMetaBlockGroup())
		return fFirstDataBlock + blockIndex + 1;

	panic("meta block");
	return 0;
}


uint16
Volume::_GroupCheckSum(ext2_block_group *group, int32 index)
{
	uint16 checksum = 0;
	if (HasChecksumFeature()) {
		int32 number = B_HOST_TO_LENDIAN_INT32(index);
		checksum = calculate_crc(0xffff, fSuperBlock.uuid,
			sizeof(fSuperBlock.uuid));
		checksum = calculate_crc(checksum, (uint8*)&number, sizeof(number));
		checksum = calculate_crc(checksum, (uint8*)group, 30);
		if (Has64bitFeature()) {
			checksum = calculate_crc(checksum, (uint8*)group + 34, 
				fGroupDescriptorSize - 34);
		}
	}
	return checksum;
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
	int32 blockOffset = index % fGroupsPerBlock;

	MutexLocker _(fLock);

	if (fGroupBlocks[blockIndex] == NULL) {
		CachedBlock cached(this);
		const uint8* block = cached.SetTo(_GroupDescriptorBlock(blockIndex));
		if (block == NULL)
			return B_IO_ERROR;

		fGroupBlocks[blockIndex] = (uint8*)malloc(fBlockSize);
		if (fGroupBlocks[blockIndex] == NULL)
			return B_NO_MEMORY;

		memcpy(fGroupBlocks[blockIndex], block, fBlockSize);

		TRACE("group [%" B_PRId32 "]: inode table %" B_PRIu64 "\n", index,
			((ext2_block_group*)(fGroupBlocks[blockIndex] + blockOffset 
				* fGroupDescriptorSize))->InodeTable(Has64bitFeature()));
	}

	*_group = (ext2_block_group*)(fGroupBlocks[blockIndex]
		+ blockOffset * fGroupDescriptorSize);
	if (HasChecksumFeature() 
		&& (*_group)->checksum != _GroupCheckSum(*_group, index)) {
		return B_BAD_DATA;
	}
	return B_OK;
}


status_t
Volume::WriteBlockGroup(Transaction& transaction, int32 index)
{
	if (index < 0 || (uint32)index > fNumGroups)
		return B_BAD_VALUE;

	TRACE("Volume::WriteBlockGroup()\n");

	int32 blockIndex = index / fGroupsPerBlock;
	int32 blockOffset = index % fGroupsPerBlock;

	MutexLocker _(fLock);

	if (fGroupBlocks[blockIndex] == NULL)
		return B_BAD_VALUE;

	ext2_block_group *group = (ext2_block_group*)(fGroupBlocks[blockIndex]
		+ blockOffset * fGroupDescriptorSize);
	
	group->checksum = _GroupCheckSum(group, index);
	TRACE("Volume::WriteBlockGroup() checksum 0x%x for group %" B_PRId32 " "
		"(free inodes %" B_PRIu32 ", unused %" B_PRIu32 ")\n", group->checksum,
		index, group->FreeInodes(Has64bitFeature()),
		group->UnusedInodes(Has64bitFeature()));

	CachedBlock cached(this);
	uint8* block = cached.SetToWritable(transaction,
		_GroupDescriptorBlock(blockIndex));
	if (block == NULL)
		return B_IO_ERROR;

	memcpy(block, (const uint8*)fGroupBlocks[blockIndex], fBlockSize);

	// TODO: Write copies

	return B_OK;
}


status_t
Volume::ActivateLargeFiles(Transaction& transaction)
{
	if ((fSuperBlock.ReadOnlyFeatures() 
		& EXT2_READ_ONLY_FEATURE_LARGE_FILE) != 0)
		return B_OK;
	
	fSuperBlock.SetReadOnlyFeatures(fSuperBlock.ReadOnlyFeatures()
		| EXT2_READ_ONLY_FEATURE_LARGE_FILE);
	
	return WriteSuperBlock(transaction);
}


status_t
Volume::ActivateDirNLink(Transaction& transaction)
{
	if ((fSuperBlock.ReadOnlyFeatures() 
		& EXT2_READ_ONLY_FEATURE_DIR_NLINK) != 0)
		return B_OK;
	
	fSuperBlock.SetReadOnlyFeatures(fSuperBlock.ReadOnlyFeatures()
		| EXT2_READ_ONLY_FEATURE_DIR_NLINK);
	
	return WriteSuperBlock(transaction);
}


status_t
Volume::SaveOrphan(Transaction& transaction, ino_t newID, ino_t& oldID)
{
	oldID = fSuperBlock.LastOrphan();
	TRACE("Volume::SaveOrphan(): Old: %d, New: %d\n", (int)oldID, (int)newID);
	fSuperBlock.SetLastOrphan(newID);

	return WriteSuperBlock(transaction);
}


status_t
Volume::RemoveOrphan(Transaction& transaction, ino_t id)
{
	ino_t currentID = fSuperBlock.LastOrphan();
	TRACE("Volume::RemoveOrphan(): ID: %d\n", (int)id);
	if (currentID == 0)
		return B_OK;

	CachedBlock cached(this);

	off_t blockNum;
	status_t status = GetInodeBlock(currentID, blockNum);
	if (status != B_OK)
		return status;

	uint8* block = cached.SetToWritable(transaction, blockNum);
	if (block == NULL)
		return B_IO_ERROR;

	ext2_inode* inode = (ext2_inode*)(block
		+ InodeBlockIndex(currentID) * InodeSize());
	
	if (currentID == id) {
		TRACE("Volume::RemoveOrphan(): First entry. Updating head to: %d\n",
			(int)inode->NextOrphan());
		fSuperBlock.SetLastOrphan(inode->NextOrphan());

		return WriteSuperBlock(transaction);
	}

	currentID = inode->NextOrphan();
	if (currentID == 0)
		return B_OK;

	do {
		off_t lastBlockNum = blockNum;
		status = GetInodeBlock(currentID, blockNum);
		if (status != B_OK)
			return status;

		if (blockNum != lastBlockNum) {
			block = cached.SetToWritable(transaction, blockNum);
			if (block == NULL)
				return B_IO_ERROR;
		}

		ext2_inode* inode = (ext2_inode*)(block
			+ InodeBlockIndex(currentID) * InodeSize());

		currentID = inode->NextOrphan();
		if (currentID == 0)
			return B_OK;
	} while(currentID != id);

	CachedBlock cachedRemoved(this);

	status = GetInodeBlock(id, blockNum);
	if (status != B_OK)
		return status;

	uint8* removedBlock = cachedRemoved.SetToWritable(transaction, blockNum);
	if (removedBlock == NULL)
		return B_IO_ERROR;

	ext2_inode* removedInode = (ext2_inode*)(removedBlock
		+ InodeBlockIndex(id) * InodeSize());

	// Next orphan is stored inside deletion time
	inode->deletion_time = removedInode->deletion_time;
	TRACE("Volume::RemoveOrphan(): Updated pointer to %d\n",
		(int)inode->NextOrphan());

	return status;
}


status_t
Volume::AllocateInode(Transaction& transaction, Inode* parent, int32 mode,
	ino_t& id)
{
	status_t status = fInodeAllocator.New(transaction, parent, mode, id);
	if (status != B_OK)
		return status;

	--fFreeInodes;

	return WriteSuperBlock(transaction);
}


status_t
Volume::FreeInode(Transaction& transaction, ino_t id, bool isDirectory)
{
	status_t status = fInodeAllocator.Free(transaction, id, isDirectory);
	if (status != B_OK)
		return status;

	++fFreeInodes;

	return WriteSuperBlock(transaction);
}


status_t
Volume::AllocateBlocks(Transaction& transaction, uint32 minimum, uint32 maximum,
	uint32& blockGroup, fsblock_t& start, uint32& length)
{
	TRACE("Volume::AllocateBlocks()\n");
	if (IsReadOnly())
		return B_READ_ONLY_DEVICE;

	TRACE("Volume::AllocateBlocks(): Calling the block allocator\n");

	status_t status = fBlockAllocator->AllocateBlocks(transaction, minimum,
		maximum, blockGroup, start, length);
	if (status != B_OK)
		return status;

	TRACE("Volume::AllocateBlocks(): Allocated %" B_PRIu32 " blocks\n",
		length);

	fFreeBlocks -= length;

	return WriteSuperBlock(transaction);
}


status_t
Volume::FreeBlocks(Transaction& transaction, fsblock_t start, uint32 length)
{
	TRACE("Volume::FreeBlocks(%" B_PRIu64 ", %" B_PRIu32 ")\n", start, length);
	if (IsReadOnly())
		return B_READ_ONLY_DEVICE;

	status_t status = fBlockAllocator->Free(transaction, start, length);
	if (status != B_OK)
		return status;

	TRACE("Volume::FreeBlocks(): number of free blocks (before): %" B_PRIdOFF
		"\n", fFreeBlocks);
	fFreeBlocks += length;
	TRACE("Volume::FreeBlocks(): number of free blocks (after): %" B_PRIdOFF
		"\n", fFreeBlocks);

	return WriteSuperBlock(transaction);
}


status_t
Volume::LoadSuperBlock()
{
	CachedBlock cached(this);
	const uint8* block = cached.SetTo(fFirstDataBlock);

	if (block == NULL)
		return B_IO_ERROR;

	if (fFirstDataBlock == 0)
		memcpy(&fSuperBlock, block + 1024, sizeof(fSuperBlock));
	else
		memcpy(&fSuperBlock, block, sizeof(fSuperBlock));

	fFreeBlocks = fSuperBlock.FreeBlocks(Has64bitFeature());
	fFreeInodes = fSuperBlock.FreeInodes();

	return B_OK;
}


status_t
Volume::WriteSuperBlock(Transaction& transaction)
{
	TRACE("Volume::WriteSuperBlock()\n");
	fSuperBlock.SetFreeBlocks(fFreeBlocks, Has64bitFeature());
	fSuperBlock.SetFreeInodes(fFreeInodes);
	// TODO: Rest of fields that can be modified

	TRACE("Volume::WriteSuperBlock(): free blocks: %" B_PRIu64 ", free inodes:"
		" %" B_PRIu32 "\n", fSuperBlock.FreeBlocks(Has64bitFeature()),
		fSuperBlock.FreeInodes());

	CachedBlock cached(this);
	uint8* block = cached.SetToWritable(transaction, fFirstDataBlock);

	if (block == NULL)
		return B_IO_ERROR;

	TRACE("Volume::WriteSuperBlock(): first data block: %" B_PRIu32 ", block:"
		" %p, superblock: %p\n", fFirstDataBlock, block, &fSuperBlock);

	if (fFirstDataBlock == 0)
		memcpy(block + 1024, &fSuperBlock, sizeof(fSuperBlock));
	else
		memcpy(block, &fSuperBlock, sizeof(fSuperBlock));

	TRACE("Volume::WriteSuperBlock(): Done\n");

	return B_OK;
}


status_t
Volume::FlushDevice()
{
	TRACE("Volume::FlushDevice(): %p, %p\n", this, fBlockCache);
	return block_cache_sync(fBlockCache);
}


status_t
Volume::Sync()
{
	TRACE("Volume::Sync()\n");
	return fJournal->Uninit();
}


//	#pragma mark - Disk scanning and initialization


/*static*/ status_t
Volume::Identify(int fd, ext2_super_block* superBlock)
{
	if (read_pos(fd, EXT2_SUPER_BLOCK_OFFSET, superBlock,
			sizeof(ext2_super_block)) != sizeof(ext2_super_block))
		return B_IO_ERROR;

	if (!superBlock->IsValid()) {
		FATAL("invalid superblock!\n");
		return B_BAD_VALUE;
	}

	return _UnsupportedIncompatibleFeatures(*superBlock) == 0
		? B_OK : B_UNSUPPORTED;
}


void
Volume::TransactionDone(bool success)
{
	if (!success) {
		status_t status = LoadSuperBlock();
		if (status != B_OK)
			panic("Failed to reload ext2 superblock.\n");
	}
}


void
Volume::RemovedFromTransaction()
{
	// TODO: Does it make a difference?
}
