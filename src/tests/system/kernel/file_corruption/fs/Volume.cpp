/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Volume.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <new>

#include <fs_cache.h>

#include "Block.h"
#include "BlockAllocator.h"
#include "checksumfs.h"
#include "checksumfs_private.h"
#include "DebugSupport.h"
#include "Directory.h"
#include "SuperBlock.h"


Volume::Volume(uint32 flags)
	:
	fFSVolume(NULL),
	fFD(-1),
	fFlags(B_FS_IS_PERSISTENT
		| (flags & B_FS_IS_READONLY != 0 ? B_FS_IS_READONLY : 0)),
	fBlockCache(NULL),
	fTotalBlocks(0),
	fName(NULL),
	fBlockAllocator(NULL),
	fRootDirectory(NULL)
{
	mutex_init(&fTransactionLock, "checksumfs transaction");
}


Volume::~Volume()
{
	delete fRootDirectory;
	delete fBlockAllocator;

	if (fBlockCache != NULL)
		block_cache_delete(fBlockCache, false);

	if (fFD >= 0)
		close(fFD);

	free(fName);

	mutex_destroy(&fTransactionLock);
}


status_t
Volume::Init(const char* device)
{
	// open the device
	fFD = open(device, IsReadOnly() ? O_RDONLY : O_RDWR);
	if (fFD < 0)
		return errno;

	// get the size
	struct stat st;
	if (fstat(fFD, &st) < 0)
		return errno;

	off_t size;
	switch (st.st_mode & S_IFMT) {
		case S_IFREG:
			size = st.st_size;
			break;
		case S_IFCHR:
		case S_IFBLK:
		{
			device_geometry geometry;
			if (ioctl(fFD, B_GET_GEOMETRY, &geometry, sizeof(geometry)) < 0)
				return errno;

			size = (off_t)geometry.bytes_per_sector * geometry.sectors_per_track
				* geometry.cylinder_count * geometry.head_count;
			break;
		}
		default:
			return B_BAD_VALUE;
	}

	return _Init(size / B_PAGE_SIZE);
}


status_t
Volume::Init(int fd, uint64 totalBlocks)
{
	fFD = dup(fd);
	if (fFD < 0)
		RETURN_ERROR(errno);

	return _Init(totalBlocks);
}


status_t
Volume::Mount(fs_volume* fsVolume)
{
	fFSVolume = fsVolume;

	// load the superblock
	Block block;
	if (!block.GetReadable(this, kCheckSumFSSuperBlockOffset / B_PAGE_SIZE))
		RETURN_ERROR(B_ERROR);

	SuperBlock* superBlock = (SuperBlock*)block.Data();
	if (!superBlock->Check(fTotalBlocks))
		RETURN_ERROR(B_BAD_DATA);

	// copy the volume name
	fName = strdup(superBlock->Name());
	if (fName == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	// init the block allocator
	status_t error = fBlockAllocator->Init(superBlock->BlockBitmap(),
		superBlock->FreeBlocks());
	if (error != B_OK)
		RETURN_ERROR(error);

	// load the root directory
	Node* rootNode;
	error = ReadNode(superBlock->RootDirectory(), rootNode);
	if (error != B_OK)
		RETURN_ERROR(error);

	fRootDirectory = dynamic_cast<Directory*>(rootNode);
	if (fRootDirectory == NULL) {
		delete rootNode;
		RETURN_ERROR(B_BAD_DATA);
	}

	error = PublishNode(fRootDirectory, 0);
	if (error != B_OK) {
		delete fRootDirectory;
		fRootDirectory = NULL;
		return error;
	}

	return B_OK;
}


void
Volume::Unmount()
{
	status_t error = block_cache_sync(fBlockCache);
	if (error != B_OK) {
		dprintf("checksumfs: Error: Failed to sync block cache when "
			"unmounting!\n");
	}
}


status_t
Volume::Initialize(const char* name)
{
	fName = strdup(name);
	if (fName == NULL)
		return B_NO_MEMORY;

	Transaction transaction(this);
	status_t error = transaction.Start();
	if (error != B_OK)
		return error;

	error = fBlockAllocator->Initialize(transaction);
	if (error != B_OK)
		return error;

	// create the root directory
	error = CreateDirectory(S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH,
		transaction, fRootDirectory);
	if (error != B_OK)
		return error;

	transaction.KeepNode(fRootDirectory);
	fRootDirectory->SetHardLinks(1);

	// write the super block
	Block block;
	if (!block.GetZero(this, kCheckSumFSSuperBlockOffset / B_PAGE_SIZE,
			transaction)) {
		return B_ERROR;
	}

	SuperBlock* superBlock = (SuperBlock*)block.Data();
	superBlock->Initialize(this);

	block.Put();

	// commit the transaction and flush the block cache
	error = transaction.Commit();
	if (error != B_OK)
		return error;

	return block_cache_sync(fBlockCache);
}


void
Volume::GetInfo(fs_info& info)
{
	info.flags = fFlags;
	info.block_size = B_PAGE_SIZE;
	info.io_size = B_PAGE_SIZE * 16;	// random value
	info.total_blocks = fTotalBlocks;
	info.free_blocks = fBlockAllocator->FreeBlocks();
	info.total_nodes = 1;	// phew, who cares?
	info.free_nodes = info.free_blocks;
	strlcpy(info.volume_name, fName, sizeof(info.volume_name));
		// TODO: We need locking once we are able to change the name!
}


status_t
Volume::PublishNode(Node* node, uint32 flags)
{
	return publish_vnode(fFSVolume, node->BlockIndex(), node,
		&gCheckSumFSVnodeOps, node->Mode(), flags);
}


status_t
Volume::GetNode(uint64 blockIndex, Node*& _node)
{
	return get_vnode(fFSVolume, blockIndex, (void**)&_node);
}


status_t
Volume::PutNode(Node* node)
{
	return put_vnode(fFSVolume, node->BlockIndex());
}


status_t
Volume::RemoveNode(Node* node)
{
	return remove_vnode(fFSVolume, node->BlockIndex());
}


status_t
Volume::ReadNode(uint64 blockIndex, Node*& _node)
{
	if (blockIndex == 0 || blockIndex >= fTotalBlocks)
		return B_BAD_VALUE;

	// load the node's block
	Block block;
	if (!block.GetReadable(this, blockIndex))
		return B_ERROR;

	checksumfs_node* nodeData = (checksumfs_node*)block.Data();

	// create the Node object
	Node* node;
	switch (nodeData->mode & S_IFMT) {
			// TODO: Don't directly access mode!
		case S_IFDIR:
			node = new(std::nothrow) Directory(this, blockIndex, *nodeData);
			break;
		default:
			node = new(std::nothrow) Node(this, blockIndex, *nodeData);
			break;
	}

	if (node == NULL)
		return B_NO_MEMORY;

	// TODO: Sanity check the node!

	_node = node;
	return B_OK;
}


status_t
Volume::CreateDirectory(mode_t mode, Transaction& transaction,
	Directory*& _directory)
{
	// allocate a free block
	AllocatedBlock allocatedBlock(fBlockAllocator, transaction);
	status_t error = allocatedBlock.Allocate();
	if (error != B_OK)
		return error;

	// create the directory
	Directory* directory = new(std::nothrow) Directory(this,
		allocatedBlock.Index(), (mode & ~(mode_t)S_IFMT) | S_IFDIR);
	if (directory == NULL)
		return B_NO_MEMORY;

	// attach the directory to the transaction
	error = transaction.AddNode(directory, TRANSACTION_DELETE_NODE);
	if (error != B_OK) {
		delete directory;
		return error;
	}

	allocatedBlock.Detach();
	_directory = directory;

	return B_OK;
}


status_t
Volume::DeleteNode(Node* node)
{
	Transaction transaction(this);
	status_t error = transaction.Start();
	if (error == B_OK) {
		error = fBlockAllocator->Free(node->BlockIndex(), 1, transaction);
		if (error == B_OK) {
			error = transaction.Commit();
			if (error != B_OK) {
				ERROR("Failed to commit transaction for delete node at %"
					B_PRIu64 "\n", node->BlockIndex());
			}
		} else {
			ERROR("Failed to free block for node at %" B_PRIu64 "\n",
				node->BlockIndex());
		}
	} else {
		ERROR("Failed to start transaction for delete node at %" B_PRIu64 "\n",
			node->BlockIndex());
	}

	delete node;
	return error;
}


status_t
Volume::_Init(uint64 totalBlocks)
{
	fTotalBlocks = totalBlocks;
	if (fTotalBlocks * B_PAGE_SIZE < kCheckSumFSMinSize)
		RETURN_ERROR(B_BAD_VALUE);

	// create a block cache
	fBlockCache = block_cache_create(fFD, fTotalBlocks, B_PAGE_SIZE,
		IsReadOnly());
	if (fBlockCache == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	// create the block allocator
	fBlockAllocator = new(std::nothrow) BlockAllocator(this);
	if (fBlockAllocator == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	return B_OK;
}
