/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008-2014, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Inode.h"

#include <string.h>
#include <util/AutoLock.h>
#include <NodeMonitor.h>

#include "CachedBlock.h"
#include "CRCTable.h"
#include "DataStream.h"
#include "DirectoryIterator.h"
#include "ExtentStream.h"
#include "HTree.h"
#include "Utility.h"


#undef ASSERT
//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#	define ASSERT(x) { if (!(x)) kernel_debugger("ext2: assert failed: " #x "\n"); }
#else
#	define TRACE(x...) ;
#	define ASSERT(x) ;
#endif
#define ERROR(x...) dprintf("\33[34mext2:\33[0m " x)


#define EXT2_EA_CHECKSUM_SIZE (offsetof(ext2_inode, checksum_high) \
	+ sizeof(uint16) - EXT2_INODE_NORMAL_SIZE)


Inode::Inode(Volume* volume, ino_t id)
	:
	fVolume(volume),
	fID(id),
	fCache(NULL),
	fMap(NULL),
	fUnlinked(false),
	fHasExtraAttributes(false)
{
	rw_lock_init(&fLock, "ext2 inode");
	recursive_lock_init(&fSmallDataLock, "ext2 inode small data");
	memset(&fNode, 0, sizeof(fNode));

	TRACE("Inode::Inode(): ext2_inode: %lu, disk inode: %" B_PRIu32
		"\n", sizeof(ext2_inode), fVolume->InodeSize());
	fNodeSize = sizeof(ext2_inode) > fVolume->InodeSize()
		? fVolume->InodeSize() : sizeof(ext2_inode);

	fInitStatus = UpdateNodeFromDisk();
	if (fInitStatus == B_OK) {
		fHasExtraAttributes = (fNodeSize == sizeof(ext2_inode)
			&& fNode.ExtraInodeSize() + EXT2_INODE_NORMAL_SIZE
				== sizeof(ext2_inode));

		if (IsDirectory() || (IsSymLink() && Size() < 60)) {
			TRACE("Inode::Inode(): Not creating the file cache\n");
			fInitStatus = B_OK;
		} else
			fInitStatus = CreateFileCache();
	} else
		TRACE("Inode: Failed initialization\n");
}


Inode::Inode(Volume* volume)
	:
	fVolume(volume),
	fID(0),
	fCache(NULL),
	fMap(NULL),
	fUnlinked(false),
	fInitStatus(B_NO_INIT)
{
	rw_lock_init(&fLock, "ext2 inode");
	recursive_lock_init(&fSmallDataLock, "ext2 inode small data");
	memset(&fNode, 0, sizeof(fNode));

	TRACE("Inode::Inode(): ext2_inode: %lu, disk inode: %" B_PRIu32 "\n",
		sizeof(ext2_inode), fVolume->InodeSize());
	fNodeSize = sizeof(ext2_inode) > fVolume->InodeSize()
		? fVolume->InodeSize() : sizeof(ext2_inode);
}


Inode::~Inode()
{
	TRACE("Inode destructor\n");

	DeleteFileCache();

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
	off_t blockNum;

	status_t status = fVolume->GetInodeBlock(fID, blockNum);
	if (status != B_OK)
		return status;

 	if (Node().Size() > 0x7fffffffLL) {
		status = fVolume->ActivateLargeFiles(transaction);
		 if (status != B_OK)
			  return status;
	}

	CachedBlock cached(fVolume);
	uint8* inodeBlockData = cached.SetToWritable(transaction, blockNum);
	if (inodeBlockData == NULL)
		return B_IO_ERROR;

	TRACE("Inode::WriteBack(): Inode ID: %" B_PRIdINO ", inode block: %"
		B_PRIdOFF ", data: %p, index: %" B_PRIu32 ", inode size: %" B_PRIu32
		", node size: %" B_PRIu32 ", this: %p, node: %p\n",
		fID, blockNum, inodeBlockData, fVolume->InodeBlockIndex(fID),
		fVolume->InodeSize(), fNodeSize, this, &fNode);
	ext2_inode* inode = (ext2_inode*)(inodeBlockData +
			fVolume->InodeBlockIndex(fID) * fVolume->InodeSize());
	memcpy(inode, (uint8*)&fNode, fNodeSize);

	if (fVolume->HasMetaGroupChecksumFeature()) {
		uint32 checksum = _InodeChecksum(inode);
		inode->checksum = checksum & 0xffff;
		if (fNodeSize > EXT2_INODE_NORMAL_SIZE
			&& fNode.ExtraInodeSize() >= EXT2_EA_CHECKSUM_SIZE) {
			inode->checksum_high = checksum >> 16;
		}
	}
	TRACE("Inode::WriteBack() finished %" B_PRId32 "\n", Node().stream.direct[0]);

	return B_OK;
}


status_t
Inode::UpdateNodeFromDisk()
{
	off_t blockNum;

	status_t status = fVolume->GetInodeBlock(fID, blockNum);
	if (status != B_OK)
		return status;

	TRACE("inode %" B_PRIdINO " at block %" B_PRIdOFF "\n", fID, blockNum);

	CachedBlock cached(fVolume);
	const uint8* inodeBlock = cached.SetTo(blockNum);

	if (inodeBlock == NULL)
		return B_IO_ERROR;

	TRACE("Inode size: %" B_PRIu32 ", inode index: %" B_PRIu32 "\n",
		fVolume->InodeSize(), fVolume->InodeBlockIndex(fID));
	ext2_inode* inode = (ext2_inode*)(inodeBlock
		+ fVolume->InodeBlockIndex(fID) * fVolume->InodeSize());

	TRACE("Attempting to copy inode data from %p to %p, ext2_inode "
		"size: %" B_PRIu32 "\n", inode, &fNode, fNodeSize);

	memcpy(&fNode, inode, fNodeSize);

	if (fVolume->HasMetaGroupChecksumFeature()) {
		uint32 checksum = _InodeChecksum(inode);
		uint32 provided = fNode.checksum;
		if (fNodeSize > EXT2_INODE_NORMAL_SIZE
			&& fNode.ExtraInodeSize() >= EXT2_EA_CHECKSUM_SIZE) {
			provided |= ((uint32)fNode.checksum_high << 16);
		} else
			checksum &= 0xffff;
		if (provided != checksum) {
			ERROR("Inode::UpdateNodeFromDisk(%" B_PRIdOFF "): "
			"verification failed\n", blockNum);
			return B_BAD_DATA;
		}
	}

	uint32 numLinks = fNode.NumLinks();
	fUnlinked = numLinks == 0 || (IsDirectory() && numLinks == 1);

	return B_OK;
}


status_t
Inode::CheckPermissions(int accessMode) const
{
	// you never have write access to a read-only volume
	if ((accessMode & W_OK) != 0 && fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	return check_access_permissions(accessMode, Mode(), (gid_t)fNode.GroupID(),
		(uid_t)fNode.UserID());
}


status_t
Inode::FindBlock(off_t offset, fsblock_t& block, uint32 *_count)
{
	if (Flags() & EXT2_INODE_EXTENTS) {
		ExtentStream stream(fVolume, this, &fNode.extent_stream, Size());
		return stream.FindBlock(offset, block, _count);
	}
	DataStream stream(fVolume, &fNode.stream, Size());
	return stream.FindBlock(offset, block, _count);
}


status_t
Inode::ReadAt(off_t pos, uint8* buffer, size_t* _length)
{
	return file_cache_read(FileCache(), NULL, pos, buffer, _length);
}


status_t
Inode::WriteAt(Transaction& transaction, off_t pos, const uint8* buffer,
	size_t* _length)
{
	TRACE("Inode::WriteAt(%" B_PRIdOFF ", %p, *(%p) = %" B_PRIuSIZE ")\n", pos,
		buffer, _length, *_length);
	ReadLocker readLocker(fLock);

	if (!HasFileCache())
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
#ifdef TRACE_EXT2
	off_t oldEnd = pos + length;
	TRACE("Inode::WriteAt(): Old calc for end? %x:%x\n",
		(int)(oldEnd >> 32), (int)(oldEnd & 0xFFFFFFFF));
#endif

	off_t end = pos + (off_t)length;
	off_t oldSize = Size();

	TRACE("Inode::WriteAt(): Old size: %" B_PRIdOFF ":%" B_PRIdOFF
		", new size: %" B_PRIdOFF ":%" B_PRIdOFF "\n",
		oldSize >> 32, oldSize & 0xFFFFFFFF,
		end >> 32, end & 0xFFFFFFFF);

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

	TRACE("Inode::WriteAt(): Performing write: %p, %" B_PRIdOFF ", %p, %"
		B_PRIuSIZE "\n", FileCache(), pos, buffer, *_length);
	status_t status = file_cache_write(FileCache(), NULL, pos, buffer,
		_length);

	WriteLockInTransaction(transaction);

	TRACE("Inode::WriteAt(): Done\n");

	return status;
}


status_t
Inode::FillGapWithZeros(off_t start, off_t end)
{
	TRACE("Inode::FileGapWithZeros(%" B_PRIdOFF " - %" B_PRIdOFF ")\n", start,
		end);

	while (start < end) {
		size_t size;

		if (end > start + 1024 * 1024 * 1024)
			size = 1024 * 1024 * 1024;
		else
			size = end - start;

		TRACE("Inode::FillGapWithZeros(): Calling file_cache_write(%p, NULL, "
			"%" B_PRIdOFF ", NULL, &(%" B_PRIuSIZE ") = %p)\n", fCache, start,
			size, &size);
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
	TRACE("Inode::Resize() ID:%" B_PRIdINO " size: %" B_PRIdOFF "\n", ID(),
		size);
	if (size < 0)
		return B_BAD_VALUE;

	off_t oldSize = Size();

	if (size == oldSize)
		return B_OK;

	TRACE("Inode::Resize(): old size: %" B_PRIdOFF ", new size: %" B_PRIdOFF
		"\n", oldSize, size);

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

	TRACE("Inode::Resize(): Writing back inode changes. Size: %" B_PRIdOFF
		"\n", Size());

	return WriteBack(transaction);
}


status_t
Inode::InitDirectory(Transaction& transaction, Inode* parent)
{
	TRACE("Inode::InitDirectory()\n");
	uint32 blockSize = fVolume->BlockSize();

	status_t status = Resize(transaction, blockSize);
	if (status != B_OK)
		return status;

	fsblock_t blockNum;
	if (Flags() & EXT2_INODE_EXTENTS) {
		ExtentStream stream(fVolume, this, &fNode.extent_stream, Size());
		status = stream.FindBlock(0, blockNum);
	} else {
		DataStream stream(fVolume, &fNode.stream, Size());
		status = stream.FindBlock(0, blockNum);
	}
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
	uint32 dotdotlength = blockSize - 12;
	if (fVolume->HasMetaGroupChecksumFeature())
		dotdotlength -= sizeof(ext2_dir_entry_tail);
	root->dotdot.entry_length = dotdotlength;
	root->dotdot.name_length = 2;
	root->dotdot.file_type = EXT2_TYPE_DIRECTORY;
	root->dotdot_entry_name[0] = '.';
	root->dotdot_entry_name[1] = '.';

	parent->IncrementNumLinks(transaction);

	SetDirEntryChecksum((uint8*)root);

	return parent->WriteBack(transaction);
}


status_t
Inode::Unlink(Transaction& transaction)
{
	uint32 numLinks = fNode.NumLinks();
	TRACE("Inode::Unlink(): Current links: %" B_PRIu32 "\n", numLinks);

	if (numLinks == 0) {
		ERROR("Inode::Unlink(): no links\n");
		return B_BAD_VALUE;
	}

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
	} else if (!IsDirectory() || numLinks > 2)
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

		// don't create anything in removed directories
		bool removed;
		if (get_vnode_removed(volume->FSVolume(), parent->ID(), &removed)
				== B_OK && removed) {
			return B_ENTRY_NOT_FOUND;
		}

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
	if (status != B_OK) {
		ERROR("Inode::Create(): AllocateInode() failed\n");
		return status;
	}

	if (entries != NULL) {
		size_t nameLength = strlen(name);
		status = entries->AddEntry(transaction, name, nameLength, id, type);
		if (status != B_OK) {
			ERROR("Inode::Create(): AddEntry() failed\n");
			return status;
		}
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
	if (parent != NULL)
		node.SetFlags(parent->Flags() & EXT2_INODE_INHERITED);
	if (volume->HasExtentsFeature()
		&& (inode->IsDirectory() || inode->IsFile())) {
		node.SetFlag(EXT2_INODE_EXTENTS);
		ExtentStream stream(volume, inode, &node.extent_stream, 0);
		stream.Init();
		ASSERT(stream.Check());
	}

	if (sizeof(ext2_inode) < volume->InodeSize())
		node.SetExtraInodeSize(sizeof(ext2_inode) - EXT2_INODE_NORMAL_SIZE);

	TRACE("Inode::Create(): Updating ID\n");
	inode->fID = id;

	if (inode->IsDirectory()) {
		TRACE("Inode::Create(): Initializing directory\n");
		status = inode->InitDirectory(transaction, parent);
		if (status != B_OK) {
			ERROR("Inode::Create(): InitDirectory() failed\n");
			delete inode;
			return status;
		}
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
	if (status != B_OK) {
		delete inode;
		return status;
	}

	TRACE("Inode::Create(): Creating vnode\n");

	Vnode vnode;
	status = vnode.Publish(transaction, inode, vnodeOps, publishFlags);
	if (status != B_OK)
		return status;

	if (!inode->IsSymLink()) {
		// Vnode::Publish doesn't publish symlinks
		if (!inode->IsDirectory()) {
			status = inode->CreateFileCache();
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
Inode::CreateFileCache()
{
	TRACE("Inode::CreateFileCache()\n");

	if (fCache != NULL)
		return B_OK;

	TRACE("Inode::CreateFileCache(): Creating file cache: %" B_PRIu32 ", %"
		B_PRIdINO ", %" B_PRIdOFF "\n", fVolume->ID(), ID(), Size());

	fCache = file_cache_create(fVolume->ID(), ID(), Size());
	if (fCache == NULL) {
		ERROR("Inode::CreateFileCache(): Failed to create file cache\n");
		return B_ERROR;
	}

	fMap = file_map_create(fVolume->ID(), ID(), Size());
	if (fMap == NULL) {
		ERROR("Inode::CreateFileCache(): Failed to create file map\n");
		file_cache_delete(fCache);
		fCache = NULL;
		return B_ERROR;
	}

	TRACE("Inode::CreateFileCache(): Done\n");

	return B_OK;
}


void
Inode::DeleteFileCache()
{
	TRACE("Inode::DeleteFileCache()\n");

	if (fCache == NULL)
		return;

	file_cache_delete(fCache);
	file_map_delete(fMap);

	fCache = NULL;
	fMap = NULL;
}


status_t
Inode::EnableFileCache()
{
	if (fCache == NULL)
		return B_BAD_VALUE;

	file_cache_enable(fCache);
	return B_OK;
}


status_t
Inode::DisableFileCache()
{
	status_t error = file_cache_disable(fCache);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
Inode::Sync()
{
	if (HasFileCache())
		return file_cache_sync(fCache);

	return B_OK;
}


void
Inode::TransactionDone(bool success)
{
	if (!success) {
		// Revert any changes to the inode
		if (UpdateNodeFromDisk() != B_OK)
			panic("Failed to reload inode from disk!\n");
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
		TRACE("Inode::_EnlargeDataStream(): Setting size to %" B_PRIdOFF "\n",
			size);
		fNode.SetSize(size);
		return B_OK;
	}

	off_t end = size == 0 ? 0 : (size - 1) / fVolume->BlockSize() + 1;
	if (Flags() & EXT2_INODE_EXTENTS) {
		ExtentStream stream(fVolume, this, &fNode.extent_stream, Size());
		stream.Enlarge(transaction, end);
		ASSERT(stream.Check());
	} else {
		DataStream stream(fVolume, &fNode.stream, oldSize);
		stream.Enlarge(transaction, end);
	}
	TRACE("Inode::_EnlargeDataStream(): Setting size to %" B_PRIdOFF "\n",
		size);
	fNode.SetSize(size);
	TRACE("Inode::_EnlargeDataStream(): Setting allocated block count to %"
		B_PRIdOFF "\n", end);
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
		TRACE("Inode::_ShrinkDataStream(): Setting size to %" B_PRIdOFF "\n",
			size);
		fNode.SetSize(size);
		return B_OK;
	}

	off_t end = size == 0 ? 0 : (size - 1) / fVolume->BlockSize() + 1;
	if (Flags() & EXT2_INODE_EXTENTS) {
		ExtentStream stream(fVolume, this, &fNode.extent_stream, Size());
		stream.Shrink(transaction, end);
		ASSERT(stream.Check());
	} else {
		DataStream stream(fVolume, &fNode.stream, oldSize);
		stream.Shrink(transaction, end);
	}

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


void
Inode::IncrementNumLinks(Transaction& transaction)
{
	fNode.SetNumLinks(fNode.NumLinks() + 1);
	if (IsIndexed() && (fNode.NumLinks() >= EXT2_INODE_MAX_LINKS
		|| fNode.NumLinks() == 2)) {
		fNode.SetNumLinks(1);
		fVolume->ActivateDirNLink(transaction);
	}
}


uint32
Inode::_InodeChecksum(ext2_inode* inode)
{
	size_t offset = offsetof(ext2_inode, checksum);
	uint32 number = fID;
	uint32 checksum = calculate_crc32c(fVolume->ChecksumSeed(),
		(uint8*)&number, sizeof(number));
	uint32 gen = fNode.generation;
	checksum = calculate_crc32c(checksum, (uint8*)&gen, sizeof(gen));
	checksum = calculate_crc32c(checksum, (uint8*)inode, offset);
	uint16 dummy = 0;
	checksum = calculate_crc32c(checksum, (uint8*)&dummy, sizeof(dummy));
	offset += sizeof(dummy);
	checksum = calculate_crc32c(checksum, (uint8*)inode + offset,
		EXT2_INODE_NORMAL_SIZE - offset);
	if (fNodeSize > EXT2_INODE_NORMAL_SIZE) {
		offset = offsetof(ext2_inode, checksum_high);
		checksum = calculate_crc32c(checksum, (uint8*)inode
			+ EXT2_INODE_NORMAL_SIZE, offset - EXT2_INODE_NORMAL_SIZE);
		if (fNode.ExtraInodeSize() >= EXT2_EA_CHECKSUM_SIZE) {
			checksum = calculate_crc32c(checksum, (uint8*)&dummy,
				sizeof(dummy));
			offset += sizeof(dummy);
		}
		checksum = calculate_crc32c(checksum, (uint8*)inode + offset,
			fVolume->InodeSize() - offset);
	}
	return checksum;
}


ext2_dir_entry_tail*
Inode::_DirEntryTail(uint8* block) const
{
	return (ext2_dir_entry_tail*)(block + fVolume->BlockSize()
		- sizeof(ext2_dir_entry_tail));
}


uint32
Inode::_DirEntryChecksum(uint8* block, uint32 id, uint32 gen) const
{
	uint32 number = id;
	uint32 checksum = calculate_crc32c(fVolume->ChecksumSeed(),
		(uint8*)&number, sizeof(number));
	checksum = calculate_crc32c(checksum, (uint8*)&gen, sizeof(gen));
	checksum = calculate_crc32c(checksum, block,
		fVolume->BlockSize() - sizeof(ext2_dir_entry_tail));
	return checksum;
}


void
Inode::SetDirEntryChecksum(uint8* block, uint32 id, uint32 gen)
{
	if (fVolume->HasMetaGroupChecksumFeature()) {
		ext2_dir_entry_tail* tail = _DirEntryTail(block);
		tail->twelve = 12;
		tail->hexade = 0xde;
		tail->checksum = _DirEntryChecksum(block, id, gen);
	}
}


void
Inode::SetDirEntryChecksum(uint8* block)
{
	SetDirEntryChecksum(block, fID, fNode.generation);
}


uint32
Inode::_ExtentLength(ext2_extent_stream* stream) const
{
	return sizeof(struct ext2_extent_header)
		+ stream->extent_header.MaxEntries()
			* sizeof(struct ext2_extent_entry);
}


uint32
Inode::_ExtentChecksum(ext2_extent_stream* stream) const
{
	uint32 number = fID;
	uint32 checksum = calculate_crc32c(fVolume->ChecksumSeed(),
		(uint8*)&number, sizeof(number));
	checksum = calculate_crc32c(checksum, (uint8*)&fNode.generation,
		sizeof(fNode.generation));
	checksum = calculate_crc32c(checksum, (uint8*)stream,
		_ExtentLength(stream));
	return checksum;
}


void
Inode::SetExtentChecksum(ext2_extent_stream* stream)
{
	if (fVolume->HasMetaGroupChecksumFeature()) {
		uint32 checksum = _ExtentChecksum(stream);
		struct ext2_extent_tail *tail = (struct ext2_extent_tail *)
			((uint8*)stream + _ExtentLength(stream));
		tail->checksum = checksum;
	}
}


bool
Inode::VerifyExtentChecksum(ext2_extent_stream* stream)
{
	if (fVolume->HasMetaGroupChecksumFeature()) {
		uint32 checksum = _ExtentChecksum(stream);
		struct ext2_extent_tail *tail = (struct ext2_extent_tail *)
			((uint8*)stream + _ExtentLength(stream));
		return tail->checksum == checksum;
	}
	return true;
}

