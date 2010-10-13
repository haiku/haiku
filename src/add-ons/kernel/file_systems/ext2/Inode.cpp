/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Inode.h"

#include <fs_cache.h>
#include <string.h>
#include <util/AutoLock.h>

#include "CachedBlock.h"
#include "DataStream.h"
#include "DirectoryIterator.h"
#include "HTree.h"
#include "Utility.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


Inode::Inode(Volume* volume, ino_t id)
	:
	fVolume(volume),
	fID(id),
	fCache(NULL),
	fMap(NULL),
	fCached(false),
	fAttributesBlock(NULL)
{
	rw_lock_init(&fLock, "ext2 inode");

	TRACE("Inode::Inode(): ext2_inode: %lu, disk inode: %lu\n",
		sizeof(ext2_inode), fVolume->InodeSize());
	fNodeSize = sizeof(ext2_inode) > fVolume->InodeSize()
		? fVolume->InodeSize() : sizeof(ext2_inode);

	fInitStatus = UpdateNodeFromDisk();
	if (fInitStatus == B_OK) {
		fHasExtraAttributes = (fNodeSize == sizeof(ext2_inode)
			&& fNode.ExtraInodeSize() + EXT2_INODE_NORMAL_SIZE
				== sizeof(ext2_inode));
		
		if (IsDirectory() || (IsSymLink() && Size() < 60)) {
			TRACE("Inode::Inode(): Not creating the file cache\n");
			fCached = false;

			fInitStatus = B_OK;
		} else
			fInitStatus = EnableFileCache();
	} else
		TRACE("Inode: Failed initialization\n");
}


Inode::Inode(Volume* volume)
	:
	fVolume(volume),
	fID(0),
	fCache(NULL),
	fMap(NULL),
	fCached(false),
	fAttributesBlock(NULL),
	fInitStatus(B_NO_INIT)
{
	rw_lock_init(&fLock, "ext2 inode");

	TRACE("Inode::Inode(): ext2_inode: %lu, disk inode: %lu\n",
		sizeof(ext2_inode), fVolume->InodeSize());
	fNodeSize = sizeof(ext2_inode) > fVolume->InodeSize()
		? fVolume->InodeSize() : sizeof(ext2_inode);
}


Inode::~Inode()
{
	TRACE("Inode destructor\n");

	if (fCached) {
		TRACE("Deleting the file cache and file map\n");
		file_cache_delete(FileCache());
		file_map_delete(Map());
	}

	if (fAttributesBlock) {
		TRACE("Returning the attributes block\n");
		uint32 block = B_LENDIAN_TO_HOST_INT32(Node().file_access_control);
		block_cache_put(fVolume->BlockCache(), block);
	}

	TRACE("Inode destructor: Done\n");
}


status_t
Inode::InitCheck()
{
	return fInitStatus;
}


void
Inode::WriteLockInTransaction(Transaction& transaction)
{
	acquire_vnode(fVolume->FSVolume(), ID());

	TRACE("Inode::WriteLockInTransaction(): Locking\n");
	rw_lock_write_lock(&fLock);

	transaction.AddListener(this);
}


status_t
Inode::WriteBack(Transaction& transaction)
{
	uint32 inodeBlock;

	status_t status = fVolume->GetInodeBlock(fID, inodeBlock);
	if (status != B_OK)
		return status;

	CachedBlock cached(fVolume);
	uint8* inodeBlockData = cached.SetToWritable(transaction, inodeBlock);
	if (inodeBlockData == NULL)
		return B_IO_ERROR;

	TRACE("Inode::WriteBack(): Inode ID: %d, inode block: %lu, data: %p, "
		"index: %lu, inode size: %lu, node size: %lu, this: %p, node: %p\n",
		(int)fID, inodeBlock, inodeBlockData, fVolume->InodeBlockIndex(fID),
		fVolume->InodeSize(), fNodeSize, this, &fNode);
	memcpy(inodeBlockData + 
			fVolume->InodeBlockIndex(fID) * fVolume->InodeSize(),
		(uint8*)&fNode, fNodeSize);

	TRACE("Inode::WriteBack() finished\n");

	return B_OK;
}


status_t
Inode::UpdateNodeFromDisk()
{
	uint32 block;

	status_t status = fVolume->GetInodeBlock(fID, block);
	if (status != B_OK)
		return status;

	TRACE("inode %Ld at block %lu\n", fID, block);

	CachedBlock cached(fVolume);
	const uint8* inodeBlock = cached.SetTo(block);

	if (inodeBlock == NULL)
		return B_IO_ERROR;

	TRACE("Inode size: %lu, inode index: %lu\n", fVolume->InodeSize(),
		fVolume->InodeBlockIndex(fID));
	ext2_inode* inode = (ext2_inode*)(inodeBlock
		+ fVolume->InodeBlockIndex(fID) * fVolume->InodeSize());

	TRACE("Attempting to copy inode data from %p to %p, ext2_inode "
		"size: %lu\n", inode, &fNode, fNodeSize);

	memcpy(&fNode, inode, fNodeSize);

	uint32 numLinks = fNode.NumLinks();
	fUnlinked = numLinks == 0 || (IsDirectory() && numLinks == 1);

	return B_OK;
}


status_t
Inode::CheckPermissions(int accessMode) const
{
	uid_t user = geteuid();
	gid_t group = getegid();

	// you never have write access to a read-only volume
	if (accessMode & W_OK && fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// root users always have full access (but they can't execute files without
	// any execute permissions set)
	if (user == 0) {
		if (!((accessMode & X_OK) != 0 && (Mode() & S_IXUSR) == 0)
			|| S_ISDIR(Mode()))
			return B_OK;
	}

	// shift mode bits, to check directly against accessMode
	mode_t mode = Mode();
	if (user == (uid_t)fNode.UserID())
		mode >>= 6;
	else if (group == (gid_t)fNode.GroupID())
		mode >>= 3;

	if (accessMode & ~(mode & S_IRWXO))
		return B_NOT_ALLOWED;

	return B_OK;
}


status_t
Inode::FindBlock(off_t offset, uint32& block)
{
	uint32 perBlock = fVolume->BlockSize() / 4;
	uint32 perIndirectBlock = perBlock * perBlock;
	uint32 index = offset >> fVolume->BlockShift();

	if (offset >= Size()) {
		TRACE("FindBlock: offset larger than inode size\n");
		return B_ENTRY_NOT_FOUND;
	}

	// TODO: we could return the size of the sparse range, as this might be more
	// than just a block

	if (index < EXT2_DIRECT_BLOCKS) {
		// direct blocks
		block = B_LENDIAN_TO_HOST_INT32(Node().stream.direct[index]);
	} else if ((index -= EXT2_DIRECT_BLOCKS) < perBlock) {
		// indirect blocks
		CachedBlock cached(fVolume);
		uint32* indirectBlocks = (uint32*)cached.SetTo(B_LENDIAN_TO_HOST_INT32(
			Node().stream.indirect));
		if (indirectBlocks == NULL)
			return B_IO_ERROR;

		block = B_LENDIAN_TO_HOST_INT32(indirectBlocks[index]);
	} else if ((index -= perBlock) < perIndirectBlock) {
		// double indirect blocks
		CachedBlock cached(fVolume);
		uint32* indirectBlocks = (uint32*)cached.SetTo(B_LENDIAN_TO_HOST_INT32(
			Node().stream.double_indirect));
		if (indirectBlocks == NULL)
			return B_IO_ERROR;

		uint32 indirectIndex
			= B_LENDIAN_TO_HOST_INT32(indirectBlocks[index / perBlock]);
		if (indirectIndex == 0) {
			// a sparse indirect block
			block = 0;
		} else {
			indirectBlocks = (uint32*)cached.SetTo(indirectIndex);
			if (indirectBlocks == NULL)
				return B_IO_ERROR;

			block = B_LENDIAN_TO_HOST_INT32(
				indirectBlocks[index & (perBlock - 1)]);
		}
	} else if ((index -= perIndirectBlock) / perBlock < perIndirectBlock) {
		// triple indirect blocks
		CachedBlock cached(fVolume);
		uint32* indirectBlocks = (uint32*)cached.SetTo(B_LENDIAN_TO_HOST_INT32(
			Node().stream.triple_indirect));
		if (indirectBlocks == NULL)
			return B_IO_ERROR;

		uint32 indirectIndex
			= B_LENDIAN_TO_HOST_INT32(indirectBlocks[index / perIndirectBlock]);
		if (indirectIndex == 0) {
			// a sparse indirect block
			block = 0;
		} else {
			indirectBlocks = (uint32*)cached.SetTo(indirectIndex);
			if (indirectBlocks == NULL)
				return B_IO_ERROR;

			indirectIndex = B_LENDIAN_TO_HOST_INT32(
				indirectBlocks[(index / perBlock) & (perBlock - 1)]);
			if (indirectIndex == 0) {
				// a sparse indirect block
				block = 0;
			} else {
				indirectBlocks = (uint32*)cached.SetTo(indirectIndex);
				if (indirectBlocks == NULL)
					return B_IO_ERROR;

				block = B_LENDIAN_TO_HOST_INT32(
					indirectBlocks[index & (perBlock - 1)]);
			}
		}
	} else {
		// Outside of the possible data stream
		dprintf("ext2: block outside datastream!\n");
		return B_ERROR;
	}

	TRACE("inode %Ld: FindBlock(offset %Ld): %lu\n", ID(), offset, block);
	return B_OK;
}


status_t
Inode::ReadAt(off_t pos, uint8* buffer, size_t* _length)
{
	size_t length = *_length;

	// set/check boundaries for pos/length
	if (pos < 0) {
		TRACE("inode %Ld: ReadAt failed(pos %Ld, length %lu)\n", ID(), pos,
			length);
		return B_BAD_VALUE;
	}

	if (pos >= Size() || length == 0) {
		TRACE("inode %Ld: ReadAt 0 (pos %Ld, length %lu)\n", ID(), pos, length);
		*_length = 0;
		return B_NO_ERROR;
	}

	return file_cache_read(FileCache(), NULL, pos, buffer, _length);
}


status_t
Inode::WriteAt(Transaction& transaction, off_t pos, const uint8* buffer,
	size_t* _length)
{
	TRACE("Inode::WriteAt(%lld, %p, *(%p) = %ld)\n", (long long)pos, buffer,
		_length, (long)*_length);
	ReadLocker readLocker(fLock);

	if (IsFileCacheDisabled())
		return B_BAD_VALUE;

	if (pos < 0)
		return B_BAD_VALUE;

	readLocker.Unlock();

	TRACE("Inode::WriteAt(): Starting transaction\n");
	transaction.Start(fVolume->GetJournal());

	WriteLocker writeLocker(fLock);

	TRACE("Inode::WriteAt(): Updating modification time\n");
	struct timespec timespec;
	_BigtimeToTimespec(real_time_clock_usecs(), &timespec);
	SetModificationTime(&timespec);

	// NOTE: Debugging info to find why sometimes resize doesn't happen
	size_t length = *_length;
	off_t oldEnd = pos + length;
	TRACE("Inode::WriteAt(): Old calc for end? %x:%x\n",
		(int)(oldEnd >> 32), (int)(oldEnd & 0xFFFFFFFF));

	off_t end = pos + (off_t)length;
	off_t oldSize = Size();

	TRACE("Inode::WriteAt(): Old size: %x:%x, new size: %x:%x\n",
		(int)(oldSize >> 32), (int)(oldSize & 0xFFFFFFFF),
		(int)(end >> 32), (int)(end & 0xFFFFFFFF));

	if (end > oldSize) {
		status_t status = Resize(transaction, end);
		if (status != B_OK) {
			*_length = 0;
			WriteLockInTransaction(transaction);
			return status;
		}

		status = WriteBack(transaction);
		if (status != B_OK) {
			*_length = 0;
			WriteLockInTransaction(transaction);
			return status;
		}
	}

	writeLocker.Unlock();

	if (oldSize < pos)
		FillGapWithZeros(oldSize, pos);

	if (length == 0) {
		// Probably just changed the file size with the pos parameter
		return B_OK;
	}

	TRACE("Inode::WriteAt(): Performing write: %p, %d, %p, %d\n",
		FileCache(), (int)pos, buffer, (int)*_length);
	status_t status = file_cache_write(FileCache(), NULL, pos, buffer, _length);

	WriteLockInTransaction(transaction);

	TRACE("Inode::WriteAt(): Done\n");

	return status;
}


status_t
Inode::FillGapWithZeros(off_t start, off_t end)
{
	TRACE("Inode::FileGapWithZeros(%ld - %ld)\n", (long)start, (long)end);

	while (start < end) {
		size_t size;

		if (end > start + 1024 * 1024 * 1024)
			size = 1024 * 1024 * 1024;
		else
			size = end - start;

		TRACE("Inode::FillGapWithZeros(): Calling file_cache_write(%p, NULL, "
			"%ld, NULL, &(%ld) = %p)\n", fCache, (long)start, (long)size,
			&size);
		status_t status = file_cache_write(fCache, NULL, start, NULL,
			&size);
		if (status != B_OK)
			return status;

		start += size;
	}

	return B_OK;
}


status_t
Inode::Resize(Transaction& transaction, off_t size)
{
	TRACE("Inode::Resize(): size: %ld\n", (long)size);
	if (size < 0)
		return B_BAD_VALUE;

	off_t oldSize = Size();

	if (size == oldSize)
		return B_OK;

	TRACE("Inode::Resize(): old size: %ld, new size: %ld\n", (long)oldSize,
		(long)size);

	status_t status;
	if (size > oldSize) {
		status = _EnlargeDataStream(transaction, size);
		if (status != B_OK) {
			// Restore original size
			_ShrinkDataStream(transaction, oldSize);
		}
	} else
		status = _ShrinkDataStream(transaction, size);

	TRACE("Inode::Resize(): Updating file map and cache\n");

	if (status != B_OK)
		return status;

	file_cache_set_size(FileCache(), size);
	file_map_set_size(Map(), size);

	TRACE("Inode::Resize(): Writing back inode changes. Size: %ld\n",
		(long)Size());

	return WriteBack(transaction);
}


status_t
Inode::AttributeBlockReadAt(off_t pos, uint8* buffer, size_t* _length)
{
	TRACE("Inode::%s(%Ld, , %lu)\n", __FUNCTION__, pos, *_length);
	size_t length = *_length;

	if (!fAttributesBlock) {
		uint32 block = B_LENDIAN_TO_HOST_INT32(Node().file_access_control);

		if (block == 0)
			return B_ENTRY_NOT_FOUND;

		TRACE("inode %Ld attributes at block %lu\n", ID(), block);
		fAttributesBlock = (ext2_xattr_header*)block_cache_get(
			GetVolume()->BlockCache(), block);
	}

	if (!fAttributesBlock)
		return B_ENTRY_NOT_FOUND;

	if (pos < 0LL || ((uint32)pos + length) > GetVolume()->BlockSize())
		return ERANGE;

	memcpy(buffer, ((uint8 *)fAttributesBlock) + (uint32)pos, length);

	*_length = length;
	return B_NO_ERROR;
}


status_t
Inode::InitDirectory(Transaction& transaction, Inode* parent)
{
	TRACE("Inode::InitDirectory()\n");
	uint32 blockSize = fVolume->BlockSize();

	status_t status = Resize(transaction, blockSize);
	if (status != B_OK)
		return status;

	uint32 blockNum;
	status = FindBlock(0, blockNum);
	if (status != B_OK)
		return status;

	CachedBlock cached(fVolume);
	uint8* block = cached.SetToWritable(transaction, blockNum, true);

	HTreeRoot* root = (HTreeRoot*)block;
	root->dot.inode_id = fID;
	root->dot.entry_length = 12;
	root->dot.name_length = 1;
	root->dot.file_type = EXT2_TYPE_DIRECTORY;
	root->dot_entry_name[0] = '.';

	root->dotdot.inode_id = parent == NULL ? fID : parent->ID();
	root->dotdot.entry_length = blockSize - 12;
	root->dotdot.name_length = 2;
	root->dotdot.file_type = EXT2_TYPE_DIRECTORY;
	root->dotdot_entry_name[0] = '.';
	root->dotdot_entry_name[1] = '.';

	parent->Node().SetNumLinks(parent->Node().NumLinks() + 1);

	return parent->WriteBack(transaction);
}


status_t
Inode::Unlink(Transaction& transaction)
{
	uint32 numLinks = fNode.NumLinks();
	TRACE("Inode::Unlink(): Current links: %lu\n", numLinks);

	if (numLinks == 0)
		return B_BAD_VALUE;

	if ((IsDirectory() && numLinks == 2) || (numLinks == 1))  {
		fUnlinked = true;

		TRACE("Inode::Unlink(): Putting inode in orphan list\n");
		ino_t firstOrphanID;
		status_t status = fVolume->SaveOrphan(transaction, fID, firstOrphanID);
		if (status != B_OK)
			return status;

		if (firstOrphanID != 0) {
			Vnode firstOrphan(fVolume, firstOrphanID);
			Inode* nextOrphan;
		
			status = firstOrphan.Get(&nextOrphan);
			if (status != B_OK)
				return status;

			fNode.SetNextOrphan(nextOrphan->ID());
		} else {
			// Next orphan link is stored in deletion time
			fNode.deletion_time = 0;
		}

		fNode.num_links = 0;

		status = remove_vnode(fVolume->FSVolume(), fID);
		if (status != B_OK)
			return status;
	} else
		fNode.SetNumLinks(--numLinks);

	return WriteBack(transaction);
}


/*static*/ status_t
Inode::Create(Transaction& transaction, Inode* parent, const char* name,
	int32 mode, int openMode, uint8 type, bool* _created, ino_t* _id,
	Inode** _inode, fs_vnode_ops* vnodeOps, uint32 publishFlags)
{
	TRACE("Inode::Create()\n");
	Volume* volume = transaction.GetVolume();

	DirectoryIterator* entries = NULL;
	ObjectDeleter<DirectoryIterator> entriesDeleter;

	if (parent != NULL) {
		parent->WriteLockInTransaction(transaction);

		TRACE("Inode::Create(): Looking up entry destination\n");
		HTree htree(volume, parent);

		status_t status = htree.Lookup(name, &entries);
		if (status == B_ENTRY_NOT_FOUND) {
			panic("We need to add the first node.\n");
			return B_ERROR;
		}
		if (status != B_OK)
			return status;
		entriesDeleter.SetTo(entries);

		TRACE("Inode::Create(): Looking up to see if file already exists\n");
		ino_t entryID;

		status = entries->FindEntry(name, &entryID);
		if (status == B_OK) {
			// File already exists
			TRACE("Inode::Create(): File already exists\n");
			if (S_ISDIR(mode) || S_ISLNK(mode) || (openMode & O_EXCL) != 0)
				return B_FILE_EXISTS;

			Vnode vnode(volume, entryID);
			Inode* inode;
	
			status = vnode.Get(&inode);
			if (status != B_OK) {
				TRACE("Inode::Create() Failed to get the inode from the "
					"vnode\n");
				return B_ENTRY_NOT_FOUND;
			}

			if (inode->IsDirectory() && (openMode & O_RWMASK) != O_RDONLY)
				return B_IS_A_DIRECTORY;
			if ((openMode & O_DIRECTORY) != 0 && !inode->IsDirectory())
				return B_NOT_A_DIRECTORY;

			if (inode->CheckPermissions(open_mode_to_access(openMode)
					| ((openMode & O_TRUNC) != 0 ? W_OK : 0)) != B_OK)
				return B_NOT_ALLOWED;

			if ((openMode & O_TRUNC) != 0) {
				// Truncate requested
				TRACE("Inode::Create(): Truncating file\n");
				inode->WriteLockInTransaction(transaction);

				status = inode->Resize(transaction, 0);
				if (status != B_OK)
					return status;
			}

			if (_created != NULL)
				*_created = false;
			if (_id != NULL)
				*_id = inode->ID();
			if (_inode != NULL)
				*_inode = inode;

			if (_id != NULL || _inode != NULL)
				vnode.Keep();

			TRACE("Inode::Create(): Done opening file\n");
			return B_OK;
		/*} else if ((mode & S_ATTR_DIR) == 0) {
			TRACE("Inode::Create(): (mode & S_ATTR_DIR) == 0\n");
			return B_BAD_VALUE;*/
		} else if ((openMode & O_DIRECTORY) != 0) {
			TRACE("Inode::Create(): (openMode & O_DIRECTORY) != 0\n");
			return B_ENTRY_NOT_FOUND;
		}

		// Return to initial position
		TRACE("Inode::Create(): Restarting iterator\n");
		entries->Restart();
	}

	status_t status;
	if (parent != NULL) {
		status = parent->CheckPermissions(W_OK);
		if (status != B_OK)
			return status;
	}

	TRACE("Inode::Create(): Allocating inode\n");
	ino_t id;
	status = volume->AllocateInode(transaction, parent, mode, id);
	if (status != B_OK)
		return status;

	if (entries != NULL) {
		size_t nameLength = strlen(name);
		status = entries->AddEntry(transaction, name, nameLength, id, type);
		if (status != B_OK)
			return status;
	}

	TRACE("Inode::Create(): Creating inode\n");
	Inode* inode = new(std::nothrow) Inode(volume);
	if (inode == NULL)
		return B_NO_MEMORY;

	TRACE("Inode::Create(): Getting node structure\n");
	ext2_inode& node = inode->Node();
	TRACE("Inode::Create(): Initializing inode data\n");
	memset(&node, 0, sizeof(ext2_inode));
	node.SetMode(mode);
	node.SetUserID(geteuid());
	node.SetGroupID(parent != NULL ? parent->Node().GroupID() : getegid());
	node.SetNumLinks(inode->IsDirectory() ? 2 : 1);
	TRACE("Inode::Create(): Updating time\n");
	struct timespec timespec;
	_BigtimeToTimespec(real_time_clock_usecs(), &timespec);
	inode->SetAccessTime(&timespec);
	inode->SetCreationTime(&timespec);
	inode->SetModificationTime(&timespec);
	
	if (sizeof(ext2_inode) < volume->InodeSize())
		node.SetExtraInodeSize(sizeof(ext2_inode) - EXT2_INODE_NORMAL_SIZE);

	TRACE("Inode::Create(): Updating ID\n");
	inode->fID = id;

	if (inode->IsDirectory()) {
		TRACE("Inode::Create(): Initializing directory\n");
		status = inode->InitDirectory(transaction, parent);
		if (status != B_OK)
			return status;
	}

	// TODO: Maybe it can be better
	/*if (volume->HasExtendedAttributes()) {
		TRACE("Inode::Create(): Initializing extended attributes\n");
		uint32 blockGroup = 0;
		uint32 pos = 0;
		uint32 allocated;

		status = volume->AllocateBlocks(transaction, 1, 1, blockGroup, pos,
			allocated);
		if (status != B_OK)
			return status;

		// Clear the new block
		uint32 blockNum = volume->FirstDataBlock() + pos +
			volume->BlocksPerGroup() * blockGroup;
		CachedBlock cached(volume);
		cached.SetToWritable(transaction, blockNum, true);

		node.SetExtendedAttributesBlock(blockNum);
	}*/

	TRACE("Inode::Create(): Saving inode\n");
	status = inode->WriteBack(transaction);
	if (status != B_OK)
		return status;

	TRACE("Inode::Create(): Creating vnode\n");

	Vnode vnode;
	status = vnode.Publish(transaction, inode, vnodeOps, publishFlags);
	if (status != B_OK)
		return status;

	if (!inode->IsSymLink()) {
		// Vnode::Publish doesn't publish symlinks
		if (!inode->IsDirectory()) {
			status = inode->EnableFileCache();
			if (status != B_OK)
				return status;
		}

		inode->WriteLockInTransaction(transaction);
	}

	if (_created)
		*_created = true;
	if (_id != NULL)
		*_id = id;
	if (_inode != NULL)
		*_inode = inode;

	if (_id != NULL || _inode != NULL)
		vnode.Keep();

	TRACE("Inode::Create(): Deleting entries iterator\n");
	DirectoryIterator* iterator = entriesDeleter.Detach();
	TRACE("Inode::Create(): Entries iterator: %p\n", entries);
	delete iterator;
	TRACE("Inode::Create(): Done\n");

	return B_OK;
}


status_t
Inode::EnableFileCache()
{
	TRACE("Inode::EnableFileCache()\n");

	if (fCached)
		return B_OK;
	if (fCache != NULL) {
		fCached = true;
		return B_OK;
	}

	TRACE("Inode::EnableFileCache(): Creating the file cache: %d, %d, %d\n",
		(int)fVolume->ID(), (int)ID(), (int)Size());
	fCache = file_cache_create(fVolume->ID(), ID(), Size());
	fMap = file_map_create(fVolume->ID(), ID(), Size());

	if (fCache == NULL) {
		TRACE("Inode::EnableFileCache(): Failed to create file cache\n");
		fCached = false;
		return B_ERROR;
	}

	fCached = true;
	TRACE("Inode::EnableFileCache(): Done\n");

	return B_OK;
}


status_t
Inode::DisableFileCache()
{
	TRACE("Inode::DisableFileCache()\n");

	if (!fCached)
		return B_OK;

	file_cache_delete(FileCache());
	file_map_delete(Map());

	fCached = false;

	return B_OK;
}


status_t
Inode::Sync()
{
	if (!IsFileCacheDisabled())
		return file_cache_sync(fCache);

	return B_OK;
}


void
Inode::TransactionDone(bool success)
{
	if (!success) {
		// Revert any changes to the inode
		if (fInitStatus == B_OK && UpdateNodeFromDisk() != B_OK)
			panic("Failed to reload inode from disk!\n");
		else if (fInitStatus == B_NO_INIT) {
			// TODO: Unpublish vnode?
			panic("Failed to finish creating inode\n");
		}
	} else {
		if (fInitStatus == B_NO_INIT) {
			TRACE("Inode::TransactionDone(): Inode creation succeeded\n");
			fInitStatus = B_OK;
		}
	}
}


void
Inode::RemovedFromTransaction()
{
	TRACE("Inode::RemovedFromTransaction(): Unlocking\n");
	rw_lock_write_unlock(&fLock);

	put_vnode(fVolume->FSVolume(), ID());
}


status_t
Inode::_EnlargeDataStream(Transaction& transaction, off_t size)
{
	if (size < 0)
		return B_BAD_VALUE;

	TRACE("Inode::_EnlargeDataStream()\n");

	uint32 blockSize = fVolume->BlockSize();
	off_t oldSize = Size();
	off_t maxSize = oldSize;
	if (maxSize % blockSize != 0)
		maxSize += blockSize - maxSize % blockSize;

	if (size <= maxSize) {
		// No need to allocate more blocks
		TRACE("Inode::_EnlargeDataStream(): No need to allocate more blocks\n");
		TRACE("Inode::_EnlargeDataStream(): Setting size to %Ld\n", size);
		fNode.SetSize(size);
		return B_OK;
	}

	uint32 end = size == 0 ? 0 : (size - 1) / fVolume->BlockSize() + 1;
	DataStream stream(fVolume, &fNode.stream, oldSize);
	stream.Enlarge(transaction, end);

	TRACE("Inode::_EnlargeDataStream(): Setting size to %Ld\n", size);
	fNode.SetSize(size);
	TRACE("Inode::_EnlargeDataStream(): Setting allocated block count to %lu\n",
		end);
	return _SetNumBlocks(_NumBlocks() + end * (fVolume->BlockSize() / 512));
}


status_t
Inode::_ShrinkDataStream(Transaction& transaction, off_t size)
{
	TRACE("Inode::_ShrinkDataStream()\n");

	if (size < 0)
		return B_BAD_VALUE;

	uint32 blockSize = fVolume->BlockSize();
	off_t oldSize = Size();
	off_t lastByte = oldSize == 0 ? 0 : oldSize - 1;
	off_t minSize = (lastByte / blockSize + 1) * blockSize;
		// Minimum size that doesn't require freeing blocks

	if (size > minSize) {
		// No need to allocate more blocks
		TRACE("Inode::_ShrinkDataStream(): No need to allocate more blocks\n");
		TRACE("Inode::_ShrinkDataStream(): Setting size to %ld\n", (long)size);
		fNode.SetSize(size);
		return B_OK;
	}

	uint32 end = size == 0 ? 0 : (size - 1) / fVolume->BlockSize() + 1;
	DataStream stream(fVolume, &fNode.stream, oldSize);
	stream.Shrink(transaction, end);

	fNode.SetSize(size);
	return _SetNumBlocks(_NumBlocks() - end * (fVolume->BlockSize() / 512));
}


uint64
Inode::_NumBlocks()
{
	if (fVolume->HugeFiles()) {
		if (fNode.Flags() & EXT2_INODE_HUGE_FILE)
			return fNode.NumBlocks64() * (fVolume->BlockSize() / 512);
		else
			return fNode.NumBlocks64();
	} else
		return fNode.NumBlocks();
}


status_t
Inode::_SetNumBlocks(uint64 numBlocks)
{
	if (numBlocks <= 0xffffffff) {
		fNode.SetNumBlocks(numBlocks);
		fNode.ClearFlag(EXT2_INODE_HUGE_FILE);
		return B_OK;
	}
	if (!fVolume->HugeFiles())
		return E2BIG;

	if (numBlocks > 0xffffffffffffULL) {
		fNode.SetFlag(EXT2_INODE_HUGE_FILE);
		numBlocks /= (fVolume->BlockSize() / 512);
	} else 
		fNode.ClearFlag(EXT2_INODE_HUGE_FILE);
		
	fNode.SetNumBlocks64(numBlocks);
	return B_OK;
}

