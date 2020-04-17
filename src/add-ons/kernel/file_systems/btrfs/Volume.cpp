/*
 * Copyright 2019, Les De Ridder, les@lesderid.net
 * Copyright 2017, Chế Vũ Gia Hy, cvghy116@gmail.com.
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 *
 * This file may be used under the terms of the MIT License.
 */


//! Superblock, mounting, etc.


#include "Volume.h"
#include "BTree.h"
#include "CachedBlock.h"
#include "Chunk.h"
#include "CRCTable.h"
#include "DeviceOpener.h"
#include "ExtentAllocator.h"
#include "Inode.h"
#include "Journal.h"
#include "Utility.h"

#ifdef FS_SHELL
#define RETURN_ERROR return
#else
#include "DebugSupport.h"
#endif

//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


//	#pragma mark -


bool
btrfs_super_block::IsValid() const
{
	// TODO: check some more values!
	if (strncmp(magic, BTRFS_SUPER_BLOCK_MAGIC, sizeof(magic)) != 0)
		return false;

	return true;
}


void
btrfs_super_block::Initialize(const char* name, off_t numBlocks,
		uint32 blockSize, uint32 sectorSize)
{
	memset(this, 0, sizeof(btrfs_super_block));

	uuid_generate(fsid);
	blocknum = B_HOST_TO_LENDIAN_INT64(BTRFS_SUPER_BLOCK_OFFSET);
	num_devices = B_HOST_TO_LENDIAN_INT64(1);
	strncpy(magic, BTRFS_SUPER_BLOCK_MAGIC_TEMPORARY, sizeof(magic));
	generation = B_HOST_TO_LENDIAN_INT64(1);
	root = B_HOST_TO_LENDIAN_INT64(BTRFS_RESERVED_SPACE_OFFSET + blockSize);
	chunk_root = B_HOST_TO_LENDIAN_INT64(Root() + blockSize);
	total_size = B_HOST_TO_LENDIAN_INT64(numBlocks * blockSize);
	used_size = B_HOST_TO_LENDIAN_INT64(6 * blockSize);
	sector_size = B_HOST_TO_LENDIAN_INT32(sectorSize);
	leaf_size = B_HOST_TO_LENDIAN_INT32(blockSize);
	node_size = B_HOST_TO_LENDIAN_INT32(blockSize);
	stripe_size = B_HOST_TO_LENDIAN_INT32(blockSize);
	checksum_type = B_HOST_TO_LENDIAN_INT32(BTRFS_CSUM_TYPE_CRC32);
	chunk_root_generation = B_HOST_TO_LENDIAN_INT64(1);
	// TODO(lesderid): Support configurable filesystem features
	incompat_flags = B_HOST_TO_LENDIAN_INT64(0);
	strlcpy(label, name, BTRFS_LABEL_SIZE);
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
	fSectorSize = fSuperBlock.SectorSize();
	TRACE("block size %" B_PRIu32 "\n", fBlockSize);
	TRACE("sector size %" B_PRIu32 "\n", fSectorSize);

	uint8* start = (uint8*)&fSuperBlock.system_chunk_array[0];
	uint8* end = (uint8*)&fSuperBlock.system_chunk_array[2048];
	while (start < end) {
		btrfs_key* key = (btrfs_key*)start;
		TRACE("system_chunk_array object_id 0x%" B_PRIx64 " offset 0x%"
			B_PRIx64 " type 0x%x\n", key->ObjectID(), key->Offset(),
			key->Type());
		if (key->Type() != BTRFS_KEY_TYPE_CHUNK_ITEM) {
			break;
		}

		btrfs_chunk* chunk = (btrfs_chunk*)(key + 1);
		fChunk = new(std::nothrow) Chunk(chunk, key->Offset());
		if (fChunk == NULL)
			return B_ERROR;
		start += sizeof(btrfs_key) + fChunk->Size();
	}

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

	fChunkTree = new(std::nothrow) BTree(this);
	if (fChunkTree == NULL)
		return B_NO_MEMORY;
	fChunkTree->SetRoot(fSuperBlock.ChunkRoot(), NULL);
	TRACE("Volume::Mount() chunk_root: %" B_PRIu64 " (physical block %" B_PRIu64
		")\n", fSuperBlock.ChunkRoot(), fChunkTree->RootBlock());

	fRootTree = new(std::nothrow) BTree(this);
	if (fRootTree == NULL)
		return B_NO_MEMORY;
	fRootTree->SetRoot(fSuperBlock.Root(), NULL);
	TRACE("Volume::Mount() root: %" B_PRIu64 " (physical block %" B_PRIu64 ")\n",
		fSuperBlock.Root(), fRootTree->RootBlock());

	BTree::Path path(fRootTree);

	TRACE("Volume::Mount(): Searching extent root\n");
	btrfs_key search_key;
	search_key.SetOffset(0);
	search_key.SetType(BTRFS_KEY_TYPE_ROOT_ITEM);
	search_key.SetObjectID(BTRFS_OBJECT_ID_EXTENT_TREE);
	btrfs_root* root;
	status = fRootTree->FindExact(&path, search_key, (void**)&root);
	if (status != B_OK) {
		ERROR("Volume::Mount(): Couldn't find extent root\n");
		return status;
	}
	TRACE("Volume::Mount(): Found extent root: %" B_PRIu64 "\n",
		root->LogicalAddress());
	fExtentTree = new(std::nothrow) BTree(this);
	if (fExtentTree == NULL)
		return B_NO_MEMORY;
	fExtentTree->SetRoot(root->LogicalAddress(), NULL);
	free(root);

	TRACE("Volume::Mount(): Searching fs root\n");
	search_key.SetOffset(0);
	search_key.SetObjectID(BTRFS_OBJECT_ID_FS_TREE);
	status = fRootTree->FindExact(&path, search_key, (void**)&root);
	if (status != B_OK) {
		ERROR("Volume::Mount(): Couldn't find fs root\n");
		return status;
	}
	TRACE("Volume::Mount(): Found fs root: %" B_PRIu64 "\n",
		root->LogicalAddress());
	fFSTree = new(std::nothrow) BTree(this);
	if (fFSTree == NULL)
		return B_NO_MEMORY;
	fFSTree->SetRoot(root->LogicalAddress(), NULL);
	free(root);

	TRACE("Volume::Mount(): Searching dev root\n");
	search_key.SetOffset(0);
	search_key.SetObjectID(BTRFS_OBJECT_ID_DEV_TREE);
	status = fRootTree->FindExact(&path, search_key, (void**)&root);
	if (status != B_OK) {
		ERROR("Volume::Mount(): Couldn't find dev root\n");
		return status;
	}
	TRACE("Volume::Mount(): Found dev root: %" B_PRIu64 "\n",
		root->LogicalAddress());
	fDevTree = new(std::nothrow) BTree(this);
	if (fDevTree == NULL)
		return B_NO_MEMORY;
	fDevTree->SetRoot(root->LogicalAddress(), NULL);
	free(root);

	TRACE("Volume::Mount(): Searching checksum root\n");
	search_key.SetOffset(0);
	search_key.SetObjectID(BTRFS_OBJECT_ID_CHECKSUM_TREE);
	status = fRootTree->FindExact(&path, search_key, (void**)&root);
	if (status != B_OK) {
		ERROR("Volume::Mount(): Couldn't find checksum root\n");
		return status;
	}
	TRACE("Volume::Mount(): Found checksum root: %" B_PRIu64 "\n",
		root->LogicalAddress());
	fChecksumTree = new(std::nothrow) BTree(this);
	if (fChecksumTree == NULL)
		return B_NO_MEMORY;
	fChecksumTree->SetRoot(root->LogicalAddress(), NULL);
	free(root);

	search_key.SetObjectID(-1);
	search_key.SetType(0);
	status = fFSTree->FindPrevious(&path, search_key, NULL);
	if (status != B_OK) {
		ERROR("Volume::Mount() Couldn't find any inode!!\n");
		return status;
	}
	fLargestInodeID = search_key.ObjectID();
	TRACE("Volume::Mount() Find larget inode id % " B_PRIu64 "\n",
		fLargestInodeID);

	if ((flags & B_MOUNT_READ_ONLY) != 0) {
		fJournal = NULL;
		fExtentAllocator = NULL;
	} else {
		// Initialize Journal
		fJournal = new(std::nothrow) Journal(this);
		if (fJournal == NULL)
			return B_NO_MEMORY;

		// Initialize ExtentAllocator;
		fExtentAllocator = new(std::nothrow) ExtentAllocator(this);
		if (fExtentAllocator == NULL)
			return B_NO_MEMORY;
		status = fExtentAllocator->Initialize();
		if (status != B_OK) {
			ERROR("could not initalize extent allocator!\n");
			return status;
		}
	}

	// ready
	status = get_vnode(fFSVolume, BTRFS_FIRST_SUBVOLUME,
		(void**)&fRootNode);
	if (status != B_OK) {
		ERROR("could not create root node: get_vnode() failed!\n");
		return status;
	}

	TRACE("Volume::Mount(): Found root node: %" B_PRIu64 " (%s)\n",
		fRootNode->ID(), strerror(fRootNode->InitCheck()));

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
Volume::Initialize(int fd, const char* label, uint32 blockSize,
	uint32 sectorSize)
{
	TRACE("Volume::Initialize()\n");

	// label must != NULL and may not contain '/' or '\\'
	if (label == NULL
		|| strchr(label, '/') != NULL || strchr(label, '\\') != NULL) {
		return B_BAD_VALUE;
	}

	if ((blockSize != 1024 && blockSize != 2048 && blockSize != 4096
		&& blockSize != 8192 && blockSize != 16384)
		|| blockSize % sectorSize != 0) {
		return B_BAD_VALUE;
	}

	DeviceOpener opener(fd, O_RDWR);
	if (opener.Device() < B_OK)
		return B_BAD_VALUE;

	if (opener.IsReadOnly())
		return B_READ_ONLY_DEVICE;

	fDevice = opener.Device();

	uint32 deviceBlockSize;
	off_t deviceSize;
	if (opener.GetSize(&deviceSize, &deviceBlockSize) < B_OK)
		return B_ERROR;
	off_t numBlocks = deviceSize / sectorSize;

	// create valid superblock

	fSuperBlock.Initialize(label, numBlocks, blockSize, sectorSize);

	fBlockSize = fSuperBlock.BlockSize();
	fSectorSize = fSuperBlock.SectorSize();

	// TODO(lesderid):	Initialize remaining core structures
	//					(extent tree, chunk tree, fs tree, etc.)

	status_t status = WriteSuperBlock();
	if (status < B_OK)
		return status;

	fBlockCache = opener.InitCache(fSuperBlock.TotalSize() / fBlockSize,
		fBlockSize);
	if (fBlockCache == NULL)
		return B_ERROR;

	fJournal = new(std::nothrow) Journal(this);
	if (fJournal == NULL)
		RETURN_ERROR(B_ERROR);

	// TODO(lesderid):	Perform secondary initialization (in transactions)
	//					(add block groups to extent tree, create root dir, etc.)
	Transaction transaction(this);

	// TODO(lesderid): Write all superblocks when transactions are committed
	status = transaction.Done();
	if (status < B_OK)
		return status;

	opener.RemoveCache(true);

	TRACE("Volume::Initialize(): Done\n");
	return B_OK;
}


status_t
Volume::Unmount()
{
	TRACE("Volume::Unmount()\n");
	delete fRootTree;
	delete fExtentTree;
	delete fChunkTree;
	delete fChecksumTree;
	delete fFSTree;
	delete fDevTree;
	delete fJournal;
	delete fExtentAllocator;
	fRootTree = NULL;
	fExtentTree = NULL;
	fChunkTree = NULL;
	fChecksumTree = NULL;
	fFSTree = NULL;
	fDevTree = NULL;
	fJournal = NULL;
	fExtentAllocator = NULL;

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
Volume::FindBlock(off_t logical, fsblock_t& physicalBlock)
{
	off_t physical;
	status_t status = FindBlock(logical, physical);
	if (status != B_OK)
		return status;
	physicalBlock = physical / fBlockSize;
	return B_OK;
}


status_t
Volume::FindBlock(off_t logical, off_t& physical)
{
	if (fChunkTree == NULL
		|| (logical >= (off_t)fChunk->Offset()
			&& logical < (off_t)fChunk->End())) {
		// try with fChunk
		return fChunk->FindBlock(logical, physical);
	}

	btrfs_key search_key;
	search_key.SetOffset(logical);
	search_key.SetType(BTRFS_KEY_TYPE_CHUNK_ITEM);
	search_key.SetObjectID(BTRFS_OBJECT_ID_FIRST_CHUNK_TREE);
	btrfs_chunk* chunk;
	BTree::Path path(fChunkTree);
	status_t status = fChunkTree->FindPrevious(&path, search_key,
		(void**)&chunk);
	if (status != B_OK)
		return status;

	Chunk _chunk(chunk, search_key.Offset());
	free(chunk);
	status = _chunk.FindBlock(logical, physical);
	if (status != B_OK)
			return status;
	TRACE("Volume::FindBlock(): logical: %" B_PRIdOFF ", physical: %" B_PRIdOFF
		"\n", logical, physical);
	return B_OK;
}


status_t
Volume::WriteSuperBlock()
{
	uint32 checksum = calculate_crc((uint32)~1,
			(uint8 *)(&fSuperBlock + sizeof(fSuperBlock.checksum)),
			sizeof(fSuperBlock) - sizeof(fSuperBlock.checksum));

	fSuperBlock.checksum[0] = (checksum >>  0) & 0xFF;
	fSuperBlock.checksum[1] = (checksum >>  8) & 0xFF;
	fSuperBlock.checksum[2] = (checksum >> 16) & 0xFF;
	fSuperBlock.checksum[3] = (checksum >> 24) & 0xFF;

	if (write_pos(fDevice, BTRFS_SUPER_BLOCK_OFFSET, &fSuperBlock,
			sizeof(btrfs_super_block))
		!= sizeof(btrfs_super_block))
		return B_IO_ERROR;

	return B_OK;
}


/* Wrapper function for allocating new block
 */
status_t
Volume::GetNewBlock(uint64& logical, fsblock_t& physical, uint64 start,
	uint64 flags)
{
	status_t status = fExtentAllocator->AllocateTreeBlock(logical, start, flags);
	if (status != B_OK)
		return status;

	return FindBlock(logical, physical);
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

