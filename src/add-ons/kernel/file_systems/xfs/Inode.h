/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _INODE_H_
#define _INODE_H_


#include "system_dependencies.h"
#include "Volume.h"
#include "xfs_types.h"


#define INODE_MAGIC 0x494e
#define INODE_MINSIZE_LOG 8
#define INODE_MAXSIZE_LOG 11
#define INODE_CORE_SIZE 96
#define INODE_CORE_UNLINKED_SIZE 100
	// Inode core but with unlinked pointer
#define DATA_FORK_OFFSET 0x64
	// For v4 FS
#define INO_MASK(x)		((1ULL << (x)) - 1)
	// Gets 2^x - 1
#define INO_TO_AGNO(id, volume)	(xfs_agnumber_t)id >> (volume->AgInodeBits())
	// Gets AG number from inode number
#define INO_TO_AGINO(id, bits)	(uint32) id & INO_MASK(bits);
	// Gets the AG relative inode number
#define INO_TO_AGBLOCK(id, volume) \
		(id >> (volume->InodesPerBlkLog())) \
		& (INO_MASK(volume->AgBlocksLog()))
	// Gets the AG relative block number that contains inode
#define INO_TO_BLOCKOFFSET(id, volume) (id & INO_MASK(volume->InodesPerBlkLog()))
	// Gets the offset into the block from the inode number
#define DIR_DFORK_PTR(dir_ino_ptr) (void*) \
		((char*) dir_ino_ptr + DATA_FORK_OFFSET)
#define DIR_AFORK_PTR(dir_ino_ptr, forkoff) \
					(void*)((char*)DIR_DFORK_PTR(dir_ino_ptr) + \
					(((uint32)forkoff)<<3))
#define DIR_AFORK_EXIST(dir_ino_ptr) dir_ino_ptr->di_forkoff!=0
#define MASK(n) ((1UL << n) - 1)
#define FSBLOCKS_TO_AGNO(n, volume) ((n) >> volume->AgBlocksLog())
#define FSBLOCKS_TO_AGBLOCKNO(n, volume) ((n) & MASK(volume->AgBlocksLog()))
#define BLOCKNO_FROM_POSITION(n, volume) \
	((n) >> (volume->BlockLog()))
#define BLOCKOFFSET_FROM_POSITION(n, inode) ((n) & (inode->BlockSize() - 1))


// xfs_bmdr_block
struct BlockInDataFork {
			uint16				Levels()
									{ return
										B_BENDIAN_TO_HOST_INT16(bb_level); }
			uint16				NumRecords()
									{ return
										B_BENDIAN_TO_HOST_INT16(bb_numrecs); }
			uint16				bb_level;
			uint16				bb_numrecs;
};


// xfs_da_blkinfo_t
struct BlockInfo {
			uint32				forw;
			uint32				back;
			uint16				magic;
			uint16				pad;
};


struct ExtentMapEntry {
			xfs_fileoff_t		br_startoff;
				// logical file block offset
			xfs_fsblock_t		br_startblock;
				// absolute block number
			xfs_filblks_t		br_blockcount;
				// # of blocks
			uint8				br_state;
				// state of the extent
};


uint32
hashfunction(const char* name, int length);


struct xfs_timestamp_t {
		int32				t_sec;
		int32				t_nsec;
};


enum xfs_dinode_fmt_t {
		XFS_DINODE_FMT_DEV,
			// For devices
		XFS_DINODE_FMT_LOCAL,
			// For Directories and links
		XFS_DINODE_FMT_EXTENTS,
			// For large number of extents
		XFS_DINODE_FMT_BTREE,
			// Extents and Directories
		XFS_DINODE_FMT_UUID,
			// Not used
};

/*
 * The xfs_ino_t is the same for all types of inodes, the data and attribute
 * fork might be different and that is to be handled accordingly.
 */
struct xfs_inode_t {
			void				SwapEndian();
			int8				Version() const;
			mode_t				Mode() const;
			void				GetModificationTime(struct timespec&
									timestamp);
			void				GetChangeTime(struct timespec& timestamp);
			void				GetAccessTime(struct timespec& timestamp);
			int8				Format() const;
				// The format of the inode
			xfs_fsize_t			Size() const;
			xfs_rfsblock_t		BlockCount() const;
			uint32				NLink() const;
			uint16				Flags() const;
			uint32				UserId() const;
			uint32				GroupId() const;
			xfs_extnum_t		DataExtentsCount() const;
			uint8				ForkOffset() const;
			uint16				di_magic;
			uint16				di_mode;
				// uses standard S_Ixxx
			int8				di_version;
				//This either would be 1 or 2
			int8				di_format;
			uint16				di_onlink;
			uint32				di_uid;
			uint32				di_gid;
			uint32				di_nlink;
			uint16				di_projid;
			uint8				di_pad[8];
			uint16				di_flushiter;
			xfs_timestamp_t		di_atime;
			xfs_timestamp_t		di_mtime;
			xfs_timestamp_t		di_ctime;
			xfs_fsize_t			di_size;
				// size in bytes or length if link
			xfs_rfsblock_t		di_nblocks;
				// blocks used including metadata
				// extended attributes not included
			xfs_extlen_t		di_extsize;
				// extent size
			xfs_extnum_t		di_nextents;
				// number of data extents
			xfs_aextnum_t		di_anextents;
				// number of EA extents
			uint8				di_forkoff;
				// decides where di_a starts
			int8				di_aformat;
				// similar to di_format
			uint32				di_dmevmask;
			uint16				di_dmstate;
			uint16				di_flags;
			uint32				di_gen;
			uint32				di_next_unlinked;
};


class Inode {
public:
								Inode(Volume* volume, xfs_ino_t id);
								~Inode();
			status_t			Init();

			xfs_ino_t			ID() const { return fId; }

			bool				IsDirectory() const
									{ return S_ISDIR(Mode()); }

			bool				IsFile() const
									{ return S_ISREG(Mode()); }

			bool				IsSymLink() const
									{ return S_ISLNK(Mode()); }

			mode_t				Mode() const { return fNode->Mode(); }

			Volume*				GetVolume() { return fVolume;}

			int8				Format() const { return fNode->Format(); }

			bool				IsLocal() const
									{ return
										Format() == XFS_DINODE_FMT_LOCAL; }

			uint32				NLink() const { return fNode->NLink(); }

			int8				Version() const { return fNode->Version(); }

			xfs_rfsblock_t		BlockCount() const
									{ return fNode->BlockCount(); }

			char*				Buffer() { return fBuffer; }

			int16				Flags() const { return fNode->Flags(); }

			xfs_fsize_t			Size() const { return fNode->Size(); }

			uint32				DirBlockSize() const
									{ return fVolume->DirBlockSize(); }

			uint32				BlockSize() const
									{ return fVolume->BlockSize(); }

			void				GetChangeTime(struct timespec& timestamp) const
								{ fNode->GetChangeTime(timestamp); }

			void				GetModificationTime(struct timespec& timestamp)
									const
								{ fNode->GetModificationTime(timestamp); }

			void				GetAccessTime(struct timespec& timestamp) const
								{ fNode->GetAccessTime(timestamp); }

			status_t			CheckPermissions(int accessMode) const;
			uint32				UserId() const { return fNode->UserId(); }
			uint32				GroupId() const { return fNode->GroupId(); }
			bool				HasFileTypeField() const;
			xfs_extnum_t		DataExtentsCount() const
									{ return fNode->DataExtentsCount(); }
			uint64				FileSystemBlockToAddr(uint64 block);
			uint8				ForkOffset() const
									{ return fNode->ForkOffset(); }
			status_t			ReadExtents();
			status_t			ReadAt(off_t pos, uint8* buffer, size_t* length);
			status_t			GetNodefromTree(uint16& levelsInTree,
									Volume* volume, size_t& len,
									size_t DirBlockSize, char* block);
			int					SearchMapInAllExtent(int blockNo);
			void				UnWrapExtentFromWrappedEntry(
									uint64 wrappedExtent[2],
									ExtentMapEntry* entry);
			status_t			ReadExtentsFromExtentBasedInode();
			status_t			ReadExtentsFromTreeInode();
			size_t				MaxRecordsPossibleInTreeRoot();
			size_t				MaxRecordsPossibleNode();
			TreePointer*		GetPtrFromRoot(int pos);
			TreePointer*		GetPtrFromNode(int pos, void* buffer);
			size_t				GetPtrOffsetIntoRoot(int pos);
			size_t				GetPtrOffsetIntoNode(int pos);
private:
			status_t			GetFromDisk();
			xfs_inode_t*		fNode;
			xfs_ino_t			fId;
			Volume*				fVolume;
			char*				fBuffer;
				// Contains the disk inode in BE format
			ExtentMapEntry*		fExtents;
};

#endif
