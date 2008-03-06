/* Inode - inode access functions
 *
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de
 * This file may be used under the terms of the MIT License.
 */
#ifndef INODE_H
#define INODE_H


#include <KernelExport.h>
#ifdef USER
#	include <stdio.h>
#endif

#include "cache.h"
#include "fsproto.h"

#include <string.h>
#include <unistd.h>

#include "Volume.h"
#include "Journal.h"
#include "Lock.h"
#include "Chain.h"
#include "Debug.h"


class BPlusTree;
class TreeIterator;
class AttributeIterator;
class InodeAllocator;


enum inode_type {
	S_DIRECTORY		= S_IFDIR,
	S_FILE			= S_IFREG,
	S_SYMLINK		= S_IFLNK,

	S_INDEX_TYPES	= (S_STR_INDEX | S_INT_INDEX | S_UINT_INDEX | S_LONG_LONG_INDEX
						| S_ULONG_LONG_INDEX | S_FLOAT_INDEX | S_DOUBLE_INDEX)
};


// The CachedBlock class is completely implemented as inlines.
// It should be used when cache single blocks to make sure they
// will be properly released after use (and it's also very
// convenient to use them).

class CachedBlock {
	public:
		CachedBlock(Volume *volume);
		CachedBlock(Volume *volume, off_t block, bool empty = false);
		CachedBlock(Volume *volume, block_run run, bool empty = false);
		CachedBlock(CachedBlock *cached);
		~CachedBlock();

		inline void Keep();
		inline void Unset();
		inline uint8 *SetTo(off_t block, bool empty = false);
		inline uint8 *SetTo(block_run run, bool empty = false);
		inline status_t WriteBack(Transaction *transaction);

		uint8 *Block() const { return fBlock; }
		off_t BlockNumber() const { return fBlockNumber; }
		uint32 BlockSize() const { return fVolume->BlockSize(); }
		uint32 BlockShift() const { return fVolume->BlockShift(); }

	private:
		CachedBlock(const CachedBlock &);
		CachedBlock &operator=(const CachedBlock &);
			// no implementation

	protected:
		Volume	*fVolume;
		off_t	fBlockNumber;
		uint8	*fBlock;
};

//--------------------------------------

class Inode : public CachedBlock {
	public:
		Inode(Volume *volume, vnode_id id, bool empty = false, uint8 reenter = 0);
		Inode(CachedBlock *cached);
		~Inode();

		bfs_inode *Node() const { return (bfs_inode *)fBlock; }
		vnode_id ID() const { return fVolume->ToVnode(fBlockNumber); }

		ReadWriteLock &Lock() { return fLock; }
		SimpleLock &SmallDataLock() { return fSmallDataLock; }

		mode_t Mode() const { return Node()->Mode(); }
		uint32 Type() const { return Node()->Type(); }
		int32 Flags() const { return Node()->Flags(); }
		bool IsContainer() const { return Mode() & (S_DIRECTORY | S_INDEX_DIR | S_ATTR_DIR); }
			// note, that this test will also be true for S_IFBLK (not that it's used in the fs :)
		bool IsDirectory() const { return (Mode() & (S_DIRECTORY | S_INDEX_DIR | S_ATTR_DIR)) == S_DIRECTORY; }
		bool IsIndex() const { return (Mode() & (S_INDEX_DIR | 0777)) == S_INDEX_DIR; }
			// that's a stupid check, but AFAIK the only possible method...

		bool IsDeleted() const { return (Flags() & INODE_DELETED) != 0; }

		bool IsAttributeDirectory() const { return (Mode() & S_ATTR_DIR) != 0; }
		bool IsAttribute() const { return Mode() & S_ATTR; }
		bool IsFile() const { return (Mode() & (S_IFMT | S_ATTR)) == S_FILE; }
		bool IsRegularNode() const { return (Mode() & (S_ATTR_DIR | S_INDEX_DIR | S_ATTR)) == 0; }
			// a regular node in the standard namespace (i.e. not an index or attribute)
		bool IsSymLink() const { return S_ISLNK(Mode()); }
		bool HasUserAccessableStream() const { return IsFile(); }
			// currently only files can be accessed with bfs_read()/bfs_write()

		off_t Size() const { return Node()->data.Size(); }
		off_t LastModified() const { return Node()->last_modified_time; }

		block_run &BlockRun() const { return Node()->inode_num; }
		block_run &Parent() const { return Node()->parent; }
		block_run &Attributes() const { return Node()->attributes; }
		Volume *GetVolume() const { return fVolume; }

		status_t InitCheck(bool checkNode = true);

		status_t CheckPermissions(int accessMode) const;

		// small_data access methods
		status_t MakeSpaceForSmallData(Transaction *transaction, const char *name, int32 length);
		status_t RemoveSmallData(Transaction *transaction, const char *name);
		status_t AddSmallData(Transaction *transaction, const char *name, uint32 type,
					const uint8 *data, size_t length, bool force = false);
		status_t GetNextSmallData(small_data **_smallData) const;
		small_data *FindSmallData(const char *name) const;
		const char *Name() const;
		status_t GetName(char *buffer) const;
		status_t SetName(Transaction *transaction, const char *name);

		// high-level attribute methods
		status_t ReadAttribute(const char *name, int32 type, off_t pos, uint8 *buffer, size_t *_length);
		status_t WriteAttribute(Transaction *transaction, const char *name, int32 type, off_t pos, const uint8 *buffer, size_t *_length);
		status_t RemoveAttribute(Transaction *transaction, const char *name);

		// attribute methods
		status_t GetAttribute(const char *name, Inode **attribute);
		void ReleaseAttribute(Inode *attribute);
		status_t CreateAttribute(Transaction *transaction, const char *name, uint32 type, Inode **attribute);

		// for directories only:
		status_t GetTree(BPlusTree **);
		bool IsEmpty();

		// manipulating the data stream
		status_t FindBlockRun(off_t pos, block_run &run, off_t &offset);

		status_t ReadAt(off_t pos, uint8 *buffer, size_t *length);
		status_t WriteAt(Transaction *transaction, off_t pos, const uint8 *buffer, size_t *length);
		status_t FillGapWithZeros(off_t oldSize, off_t newSize);

		status_t SetFileSize(Transaction *transaction, off_t size);
		status_t Append(Transaction *transaction, off_t bytes);
		status_t Trim(Transaction *transaction);

		status_t Free(Transaction *transaction);
		status_t Sync();

		// create/remove inodes
		status_t Remove(Transaction *transaction, const char *name, off_t *_id = NULL,
					bool isDirectory = false);
		static status_t Create(Transaction *transaction, Inode *parent, const char *name,
					int32 mode, int omode, uint32 type, off_t *_id = NULL, Inode **_inode = NULL);

		// index maintaining helper
		void UpdateOldSize() { fOldSize = Size(); }
		void UpdateOldLastModified() { fOldLastModified = Node()->LastModifiedTime(); }
		off_t OldSize() { return fOldSize; }
		off_t OldLastModified() { return fOldLastModified; }

		// file cache
		void *FileCache() const { return fCache; }
		void SetFileCache(void *cache) { fCache = cache; }

	private:
		Inode(const Inode &);
		Inode &operator=(const Inode &);
			// no implementation

		friend void dump_inode(Inode &inode);
		friend class AttributeIterator;
		friend class InodeAllocator;

		void Initialize();

		status_t RemoveSmallData(small_data *item, int32 index);

		void AddIterator(AttributeIterator *iterator);
		void RemoveIterator(AttributeIterator *iterator);

		status_t FreeStaticStreamArray(Transaction *transaction, int32 level, block_run run,
					off_t size, off_t offset, off_t &max);
		status_t FreeStreamArray(Transaction *transaction, block_run *array, uint32 arrayLength,
					off_t size, off_t &offset, off_t &max);
		status_t AllocateBlockArray(Transaction *transaction, block_run &run);
		status_t GrowStream(Transaction *transaction, off_t size);
		status_t ShrinkStream(Transaction *transaction, off_t size);

		BPlusTree		*fTree;
		Inode			*fAttributes;
		ReadWriteLock	fLock;
		off_t			fOldSize;			// we need those values to ensure we will remove
		off_t			fOldLastModified;	// the correct keys from the indices
		void			*fCache;

		mutable SimpleLock	fSmallDataLock;
		Chain<AttributeIterator> fIterators;
};


// The Vnode class provides a convenience layer upon get_vnode(), so that
// you don't have to call put_vnode() anymore, which may make code more
// readable in some cases

class Vnode {
	public:
		Vnode(Volume *volume, vnode_id id)
			:
			fVolume(volume),
			fID(id)
		{
		}

		Vnode(Volume *volume, block_run run)
			:
			fVolume(volume),
			fID(volume->ToVnode(run))
		{
		}

		~Vnode()
		{
			Put();
		}

		status_t Get(Inode **inode)
		{
			// should we check inode against NULL here? it should not be necessary
#ifdef UNSAFE_GET_VNODE
			RecursiveLocker locker(fVolume->Lock());
#endif
			return get_vnode(fVolume->ID(), fID, (void **)inode);
		}

		void Put()
		{
			if (fVolume)
				put_vnode(fVolume->ID(), fID);
			fVolume = NULL;
		}

		void Keep()
		{
			fVolume = NULL;
		}

	private:
		Volume		*fVolume;
		vnode_id	fID;
};


class AttributeIterator {
	public:
		AttributeIterator(Inode *inode);
		~AttributeIterator();
		
		status_t Rewind();
		status_t GetNext(char *name, size_t *length, uint32 *type, vnode_id *id);

	private:
		int32		fCurrentSmallData;
		Inode		*fInode, *fAttributes;
		TreeIterator *fIterator;
		void		*fBuffer;

	private:
		friend class Chain<AttributeIterator>;
		friend class Inode;

		void Update(uint16 index, int8 change);
		AttributeIterator *fNext;
};


//--------------------------------------
// inlines


inline
CachedBlock::CachedBlock(Volume *volume)
	:
	fVolume(volume),
	fBlockNumber(-1),
	fBlock(NULL)
{
}


inline
CachedBlock::CachedBlock(Volume *volume, off_t block, bool empty)
	:
	fVolume(volume),
	fBlock(NULL)
{
	SetTo(block, empty);
}


inline
CachedBlock::CachedBlock(Volume *volume, block_run run, bool empty)
	:
	fVolume(volume),
	fBlock(NULL)
{
	SetTo(volume->ToBlock(run), empty);
}


inline 
CachedBlock::CachedBlock(CachedBlock *cached)
	:
	fVolume(cached->fVolume),
	fBlockNumber(cached->BlockNumber()),
	fBlock(cached->fBlock)
{
	cached->Keep();
}


inline
CachedBlock::~CachedBlock()
{
	Unset();
}


inline void
CachedBlock::Keep()
{
	fBlock = NULL;
}


inline void
CachedBlock::Unset()
{
	if (fBlock != NULL)
		release_block(fVolume->Device(), fBlockNumber);
}


inline uint8 *
CachedBlock::SetTo(off_t block, bool empty)
{
	Unset();
	fBlockNumber = block;
	return fBlock = empty ? (uint8 *)get_empty_block(fVolume->Device(), block, BlockSize())
						  : (uint8 *)get_block(fVolume->Device(), block, BlockSize());
}


inline uint8 *
CachedBlock::SetTo(block_run run, bool empty)
{
	return SetTo(fVolume->ToBlock(run), empty);
}


inline status_t
CachedBlock::WriteBack(Transaction *transaction)
{
	if (transaction == NULL || fBlock == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	return transaction->WriteBlocks(fBlockNumber, fBlock);
}


/**	Converts the "omode", the open flags given to bfs_open(), into
 *	access modes, e.g. since O_RDONLY requires read access to the
 *	file, it will be converted to R_OK.
 */

inline int
oModeToAccess(int omode)
{
	omode &= O_RWMASK;
	if (omode == O_RDONLY)
		return R_OK;
	else if (omode == O_WRONLY)
		return W_OK;
	
	return R_OK | W_OK;
}

#endif	/* INODE_H */
