#ifndef INODE_H
#define INODE_H
/* Inode - inode access functions
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>
#ifdef USER
#	include "myfs.h"
#	include <stdio.h>
#endif

#ifndef _IMPEXP_KERNEL
#	define _IMPEXP_KERNEL
#endif

extern "C" {
	#include <lock.h>
	#include <cache.h>
}

#include <string.h>

#include "Volume.h"
#include "Journal.h"
#include "Lock.h"
#include "Chain.h"
#include "Debug.h"


class BPlusTree;
class TreeIterator;
class AttributeIterator;


enum inode_type {
	S_DIRECTORY	= S_IFDIR,
	S_FILE		= S_IFREG,
	S_SYMLINK	= S_IFLNK
};


// The CachedBlock class is completely implemented as inlines.
// It should be used when cache single blocks to make sure they
// will be properly released after use (and it's also very
// convenient to use them).

class CachedBlock {
	public:
		CachedBlock(Volume *volume)
			:
			fVolume(volume),
			fBlock(NULL)
		{
		}

		CachedBlock(Volume *volume,off_t block,bool empty = false)
			:
			fVolume(volume),
			fBlock(NULL)
		{
			SetTo(block,empty);
		}

		CachedBlock(Volume *volume,block_run run,bool empty = false)
			:
			fVolume(volume),
			fBlock(NULL)
		{
			SetTo(volume->ToBlock(run),empty);
		}

		~CachedBlock()
		{
			Unset();
		}

		void Unset()
		{
			if (fBlock != NULL)
				release_block(fVolume->Device(),fBlockNumber);
		}

		uint8 *SetTo(off_t block,bool empty = false)
		{
			Unset();
			fBlockNumber = block;
			return fBlock = empty ? (uint8 *)get_empty_block(fVolume->Device(),block,fVolume->BlockSize())
								  : (uint8 *)get_block(fVolume->Device(),block,fVolume->BlockSize());
		}

		uint8 *SetTo(block_run run,bool empty = false)
		{
			return SetTo(fVolume->ToBlock(run),empty);
		}

		status_t WriteBack(Transaction *transaction)
		{
			if (transaction == NULL || fBlock == NULL)
				RETURN_ERROR(B_BAD_VALUE);

			return transaction->WriteBlocks(fBlockNumber,fBlock);
		}

		uint8 *Block() const { return fBlock; }
		off_t BlockNumber() const { return fBlockNumber; }

	protected:
		Volume	*fVolume;
		off_t	fBlockNumber;
		uint8	*fBlock;
};


class Inode : public CachedBlock {
	public:
		Inode(Volume *volume,vnode_id id,bool empty = false,uint8 reenter = 0);
		~Inode();

		bfs_inode *Node() const { return (bfs_inode *)fBlock; }
		vnode_id ID() const { return fVolume->ToVnode(fBlockNumber); }

		ReadWriteLock &Lock() { return fLock; }
		SimpleLock &SmallDataLock() { return fSmallDataLock; }

		mode_t Mode() const { return Node()->mode; }
		int32 Flags() const { return Node()->flags; }
		bool IsDirectory() const { return Mode() & (S_DIRECTORY | S_INDEX_DIR | S_ATTR_DIR); }
			// note, that this test will also be true for S_IFBLK (not that it's used in the fs :)
		bool IsIndex() const { return (Mode() & (S_INDEX_DIR | 0777)) == S_INDEX_DIR; }
			// that's a stupid check, but AFAIK the only possible method...

		bool IsSymLink() const { return S_ISLNK(Mode()); }
		bool HasUserAccessableStream() const { return S_ISREG(Mode()); }
			// currently only files can be accessed with bfs_read()/bfs_write()

		off_t Size() const { return Node()->data.size; }

		block_run &BlockRun() const { return Node()->inode_num; }
		block_run &Parent() const { return Node()->parent; }
		block_run &Attributes() const { return Node()->attributes; }
		Volume *GetVolume() const { return fVolume; }

		status_t InitCheck();

		status_t CheckPermissions(int accessMode) const;

		// small_data access methods
		status_t MakeSpaceForSmallData(Transaction *transaction,const char *name, int32 length);
		status_t RemoveSmallData(Transaction *transaction,const char *name);
		status_t AddSmallData(Transaction *transaction,const char *name,uint32 type,const uint8 *data,size_t length,bool force = false);
		status_t GetNextSmallData(small_data **smallData) const;
		small_data *FindSmallData(const char *name) const;
		const char *Name() const;
		status_t SetName(Transaction *transaction,const char *name);

		// high-level attribute methods
		status_t ReadAttribute(const char *name, int32 type, off_t pos, uint8 *buffer, size_t *_length);
		status_t WriteAttribute(Transaction *transaction, const char *name, int32 type, off_t pos, const uint8 *buffer, size_t *_length);
		status_t RemoveAttribute(Transaction *transaction, const char *name);

		// attribute methods
		status_t GetAttribute(const char *name,Inode **attribute);
		void ReleaseAttribute(Inode *attribute);
		status_t CreateAttribute(Transaction *transaction,const char *name,uint32 type,Inode **attribute);

		// for directories only:
		status_t GetTree(BPlusTree **);
		bool IsEmpty();

		// manipulating the data stream
		status_t FindBlockRun(off_t pos,block_run &run,off_t &offset);

		status_t ReadAt(off_t pos,uint8 *buffer,size_t *length);
		status_t WriteAt(Transaction *transaction,off_t pos,const uint8 *buffer,size_t *length);
		status_t FillGapWithZeros(off_t oldSize,off_t newSize);

		status_t SetFileSize(Transaction *transaction,off_t size);
		status_t Append(Transaction *transaction,off_t bytes);
		status_t Trim(Transaction *transaction);

		status_t Sync();

		// create/remove inodes
		status_t Remove(Transaction *transaction,const char *name,off_t *_id = NULL,bool isDirectory = false);
		static status_t Create(Transaction *transaction,Inode *parent,const char *name,int32 mode,int omode,uint32 type,off_t *_id = NULL,Inode **_inode = NULL);

		// index maintaining helper
		void UpdateOldSize() { fOldSize = Size(); }
		void UpdateOldLastModified() { fOldLastModified = Node()->last_modified_time; }
		off_t OldSize() { return fOldSize; }
		off_t OldLastModified() { return fOldLastModified; }

	private:
		friend AttributeIterator;

		status_t RemoveSmallData(small_data *item,int32 index);

		void AddIterator(AttributeIterator *iterator);
		void RemoveIterator(AttributeIterator *iterator);

		status_t FreeStaticStreamArray(Transaction *transaction,int32 level,block_run run,off_t size,off_t offset,off_t &max);
		status_t FreeStreamArray(Transaction *transaction, block_run *array, uint32 arrayLength, off_t size, off_t &offset, off_t &max);
		status_t GrowStream(Transaction *transaction,off_t size);
		status_t ShrinkStream(Transaction *transaction,off_t size);

		BPlusTree		*fTree;
		Inode			*fAttributes;
		ReadWriteLock	fLock;
		off_t			fOldSize;			// we need those values to ensure we will remove
		off_t			fOldLastModified;	// the correct keys from the indices

		mutable SimpleLock	fSmallDataLock;
		Chain<AttributeIterator> fIterators;
};


// The Vnode class provides a convenience layer upon get_vnode(), so that
// you don't have to call put_vnode() anymore, which may make code more
// readable in some cases

class Vnode {
	public:
		Vnode(Volume *volume,vnode_id id)
			:
			fVolume(volume),
			fID(id)
		{
		}

		Vnode(Volume *volume,block_run run)
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
			return get_vnode(fVolume->ID(),fID,(void **)inode);
		}

		void Put()
		{
			if (fVolume)
				put_vnode(fVolume->ID(),fID);
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
		status_t GetNext(char *name,size_t *length,uint32 *type,vnode_id *id);

	private:
		int32		fCurrentSmallData;
		Inode		*fInode, *fAttributes;
		TreeIterator *fIterator;
		void		*fBuffer;

	private:
		friend Chain<AttributeIterator>;
		friend Inode;

		void Update(uint16 index,int8 change);
		AttributeIterator *fNext;
};


/**	Converts the "omode", the open flags given to bfs_open(), into
 *	access modes, e.g. since O_RDONLY requires read access to the
 *	file, it will be converted to R_OK.
 */

inline int oModeToAccess(int omode)
{
	omode &= O_RWMASK;
	if (omode == O_RDONLY)
		return R_OK;
	else if (omode == O_WRONLY)
		return W_OK;
	
	return R_OK | W_OK;
}

#endif	/* INODE_H */
