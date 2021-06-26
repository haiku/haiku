/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef INODE_H
#define INODE_H


#include <fs_cache.h>
#include <lock.h>
#include <string.h>

#include "ext2.h"
#include "Volume.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACEI(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACEI(x...) ;
#endif


class Inode : public TransactionListener {
public:
						Inode(Volume* volume, ino_t id);
						~Inode();

			status_t	InitCheck();

			ino_t		ID() const { return fID; }

			rw_lock*	Lock() { return &fLock; }
			void		WriteLockInTransaction(Transaction& transaction);

			status_t	UpdateNodeFromDisk();
			status_t	WriteBack(Transaction& transaction);

			recursive_lock&	SmallDataLock() { return fSmallDataLock; }

			bool		IsDirectory() const
							{ return S_ISDIR(Mode()); }
			bool		IsFile() const
							{ return S_ISREG(Mode()); }
			bool		IsSymLink() const
							{ return S_ISLNK(Mode()); }
			status_t	CheckPermissions(int accessMode) const;

			bool		IsDeleted() const { return fUnlinked; }
			bool		HasExtraAttributes() const
							{ return fHasExtraAttributes; }
			bool		IsIndexed() const 
						{ return fVolume->IndexedDirectories()
							&& (Flags() & EXT2_INODE_INDEXED) != 0; }

			mode_t		Mode() const { return fNode.Mode(); }
			int32		Flags() const { return fNode.Flags(); }

			off_t		Size() const { return fNode.Size(); }
			void		GetChangeTime(struct timespec *timespec) const
						{ fNode.GetChangeTime(timespec, fHasExtraAttributes); }
			void		GetModificationTime(struct timespec *timespec) const
						{ fNode.GetModificationTime(timespec, 
							fHasExtraAttributes); }
			void		GetCreationTime(struct timespec *timespec) const
						{ fNode.GetCreationTime(timespec,
							fHasExtraAttributes); }
			void		GetAccessTime(struct timespec *timespec) const
						{ fNode.GetAccessTime(timespec, fHasExtraAttributes); }
			void		SetChangeTime(const struct timespec *timespec)
						{ fNode.SetChangeTime(timespec, fHasExtraAttributes); }
			void		SetModificationTime(const struct timespec *timespec)
						{ fNode.SetModificationTime(timespec,
							fHasExtraAttributes); }
			void		SetCreationTime(const struct timespec *timespec) 
						{ fNode.SetCreationTime(timespec,
							fHasExtraAttributes); }
			void		SetAccessTime(const struct timespec *timespec)
						{ fNode.SetAccessTime(timespec, fHasExtraAttributes); }
			void		IncrementNumLinks(Transaction& transaction);

			//::Volume* _Volume() const { return fVolume; }
			Volume*		GetVolume() const { return fVolume; }

			status_t	FindBlock(off_t offset, fsblock_t& block,
							uint32 *_count = NULL);
			status_t	ReadAt(off_t pos, uint8 *buffer, size_t *length);
			status_t	WriteAt(Transaction& transaction, off_t pos,
							const uint8* buffer, size_t* length);
			status_t	FillGapWithZeros(off_t start, off_t end);

			status_t	Resize(Transaction& transaction, off_t size);

			ext2_inode&	Node() { return fNode; }

			status_t	InitDirectory(Transaction& transaction, Inode* parent);

			status_t	Unlink(Transaction& transaction);

	static	status_t	Create(Transaction& transaction, Inode* parent,
							const char* name, int32 mode, int openMode,
							uint8 type, bool* _created = NULL,
							ino_t* _id = NULL, Inode** _inode = NULL,
							fs_vnode_ops* vnodeOps = NULL,
							uint32 publishFlags = 0);
	static 	void		_BigtimeToTimespec(bigtime_t time,
							struct timespec *timespec)
						{ timespec->tv_sec = time / 1000000LL; 
							timespec->tv_nsec = (time % 1000000LL) * 1000; }
	
			void*		FileCache() const { return fCache; }
			void*		Map() const { return fMap; }
			status_t	CreateFileCache();
			void		DeleteFileCache();
			bool		HasFileCache() { return fCache != NULL; }
			status_t	EnableFileCache();
			status_t	DisableFileCache();

			status_t	Sync();

			void		SetDirEntryChecksum(uint8* block, uint32 id, uint32 gen);
			void		SetDirEntryChecksum(uint8* block);

			void		SetExtentChecksum(ext2_extent_stream* stream);
			bool		VerifyExtentChecksum(ext2_extent_stream* stream);

protected:
	virtual	void		TransactionDone(bool success);
	virtual	void		RemovedFromTransaction();


private:
						Inode(Volume* volume);
						Inode(const Inode&);
						Inode &operator=(const Inode&);
							// no implementation

			status_t	_EnlargeDataStream(Transaction& transaction,
							off_t size);
			status_t	_ShrinkDataStream(Transaction& transaction,
							off_t size);
	
			uint64		_NumBlocks();
			status_t	_SetNumBlocks(uint64 numBlocks);

			uint32		_InodeChecksum(ext2_inode* inode);

			ext2_dir_entry_tail*	_DirEntryTail(uint8* block) const;
			uint32		_DirEntryChecksum(uint8* block, uint32 id,
							uint32 gen) const;

			uint32		_ExtentLength(ext2_extent_stream* stream) const;
			uint32		_ExtentChecksum(ext2_extent_stream* stream) const;

			rw_lock		fLock;
			::Volume*	fVolume;
			ino_t		fID;
			void*		fCache;
			void*		fMap;
			bool		fUnlinked;
			bool		fHasExtraAttributes;
			ext2_inode	fNode;
			uint32		fNodeSize;
				// Inodes have a variable size, but the important
				// information is always the same size (except in ext4)
			status_t	fInitStatus;

			mutable recursive_lock fSmallDataLock;
};


// The Vnode class provides a convenience layer upon get_vnode(), so that
// you don't have to call put_vnode() anymore, which may make code more
// readable in some cases

class Vnode {
public:
	Vnode(Volume* volume, ino_t id)
		:
		fInode(NULL)
	{
		SetTo(volume, id);
	}

	Vnode()
		:
		fStatus(B_NO_INIT),
		fInode(NULL)
	{
	}

	~Vnode()
	{
		Unset();
	}

	status_t InitCheck()
	{
		return fStatus;
	}

	void Unset()
	{
		if (fInode != NULL) {
			put_vnode(fInode->GetVolume()->FSVolume(), fInode->ID());
			fInode = NULL;
			fStatus = B_NO_INIT;
		}
	}

	status_t SetTo(Volume* volume, ino_t id)
	{
		Unset();

		return fStatus = get_vnode(volume->FSVolume(), id, (void**)&fInode);
	}

	status_t Get(Inode** _inode)
	{
		*_inode = fInode;
		return fStatus;
	}

	void Keep()
	{
		TRACEI("Vnode::Keep()\n");
		fInode = NULL;
	}

	status_t Publish(Transaction& transaction, Inode* inode,
		fs_vnode_ops* vnodeOps, uint32 publishFlags)
	{
		TRACEI("Vnode::Publish()\n");
		Volume* volume = transaction.GetVolume();

		status_t status = B_OK;

		if (!inode->IsSymLink() && volume->ID() >= 0) {
			TRACEI("Vnode::Publish(): Publishing volume: %p, %" B_PRIdINO
				", %p, %p, %" B_PRIu16 ", %" B_PRIx32 "\n", volume->FSVolume(),
				inode->ID(), inode, vnodeOps != NULL ? vnodeOps : &gExt2VnodeOps,
				inode->Mode(), publishFlags);
			status = publish_vnode(volume->FSVolume(), inode->ID(), inode,
				vnodeOps != NULL ? vnodeOps : &gExt2VnodeOps, inode->Mode(),
				publishFlags);
			TRACEI("Vnode::Publish(): Result: %s\n", strerror(status));
		}

		if (status == B_OK) {
			TRACEI("Vnode::Publish(): Preparing internal data\n");
			fInode = inode;
			fStatus = B_OK;

			cache_add_transaction_listener(volume->BlockCache(),
				transaction.ID(), TRANSACTION_ABORTED, &_TransactionListener,
				inode);
		}

		return status;
	}

private:
	status_t	fStatus;
	Inode*		fInode;

	// TODO: How to apply coding style here?
	static void _TransactionListener(int32 id, int32 event, void* _inode)
	{
		Inode* inode = (Inode*)_inode;

		if (event == TRANSACTION_ABORTED) {
			// TODO: Unpublish?
			panic("Transaction %d aborted, inode %p still exists!\n", (int)id,
				inode);
		}
	}
};

#endif	// INODE_H
