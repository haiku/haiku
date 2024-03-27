/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _INODE_H_
#define _INODE_H_


#include "system_dependencies.h"
#include "Volume.h"
#include "xfs_types.h"


#define INODE_MAGIC	0x494e
#define INODE_MINSIZE_LOG	8
#define INODE_MAXSIZE_LOG	11
#define INODE_MIN_SIZE	(1 << INODE_MINSIZE_LOG)
#define INODE_MAX_SIZE	(1 << INODE_MAXSIZE_LOG)
#define INODE_CRC_OFF offsetof(Inode::Dinode, di_crc)
#define MAXAEXTNUM	((xfs_aextnum_t) 0x7fff)
#define MAXEXTNUM	((xfs_extnum_t) 0x7fffffff)

// Does fs volume has v3 inodes?
#define HAS_V3INODES(volume)	(volume->IsVersion5() ? 1 : 0 )

// Inode size for given fs
#define DINODE_SIZE(volume) \
	(HAS_V3INODES(volume) ? sizeof(Inode::Dinode) : offsetof(Inode::Dinode, di_crc))
#define LITINO(volume) \
	((volume)->InodeSize() - DINODE_SIZE(volume))

// inode data and attribute fork sizes
#define DFORK_BOFF(ino)	((int)((ino)->di_forkoff << 3))

#define DFORK_DSIZE(ino, volume) \
	((ino)->di_forkoff ? DFORK_BOFF(ino) : LITINO(volume))
#define DFORK_ASIZE(ino, volume) \
	((ino)->di_forkoff ? LITINO(volume) - DFORK_BOFF(ino) : 0)
#define DFORK_SIZE(ino, volume, w) \
	((w) == XFS_DATA_FORK ? \
		DFORK_DSIZE(ino, volume) : DFORK_ASIZE(ino, volume))


#define INO_MASK(x)	((1ULL << (x)) - 1)
	// Gets 2^x - 1
#define INO_TO_AGNO(id, volume)	((xfs_agnumber_t)id >> (volume->AgInodeBits()))
	// Gets AG number from inode number
#define INO_TO_AGINO(id, bits)	((uint32) id & INO_MASK(bits))
	// Gets the AG relative inode number
#define INO_TO_AGBLOCK(id, volume) \
		(id >> (volume->InodesPerBlkLog())) \
		& (INO_MASK(volume->AgBlocksLog()))
	// Gets the AG relative block number that contains inode
#define INO_TO_BLOCKOFFSET(id, volume)	(id & INO_MASK(volume->InodesPerBlkLog()))
	// Gets the offset into the block from the inode number

// Data and attribute fork pointers
#define DIR_DFORK_PTR(dir_ino_ptr, DATA_FORK_OFFSET) (void*) \
		((char*) dir_ino_ptr + DATA_FORK_OFFSET)
#define DIR_AFORK_PTR(dir_ino_ptr, DATA_FORK_OFFSET, forkoff) \
					(void*)((char*)DIR_DFORK_PTR(dir_ino_ptr, DATA_FORK_OFFSET) + \
					(((uint32)forkoff)<<3))

#define MASK(n)	((1UL << n) - 1)
#define FSBLOCKS_TO_AGNO(n, volume)	((n) >> volume->AgBlocksLog())
#define FSBLOCKS_TO_AGBLOCKNO(n, volume)	((n) & MASK(volume->AgBlocksLog()))
#define BLOCKNO_FROM_POSITION(n, volume) \
	((n) >> (volume->BlockLog()))
#define BLOCKOFFSET_FROM_POSITION(n, inode)	((n) & (inode->BlockSize() - 1))


// Inode fork identifiers
#define XFS_DATA_FORK	0
#define XFS_ATTR_FORK	1

#define XFS_DFORK_FORMAT(ino, w) \
	((w) == XFS_DATA_FORK ? (ino)->di_format : (ino)->di_aformat)

#define XFS_DFORK_NEXTENTS(ino, w) \
	((w) == XFS_DATA_FORK ? (ino)->di_nextents : (ino)->di_naextents)

#define DFORK_MAXEXT(ino, volume, w) \
	(DFORK_SIZE(ino, volume, w) / (2 * sizeof(uint64)))


struct LongBlock;  // Forward declaration to remove cyclic dependency


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


//xfs_da_blkinfo_t
struct BlockInfo {
			uint32				forw;
			uint32				back;
			uint16				magic;
			uint16				pad;
};


// xfs_da3_blkinfo_t
struct BlockInfoV5 {
			uint32				forw;
			uint32				back;
			uint16				magic;
			uint16				pad;

			// version 5

			uint32				crc;
			uint64				blkno;
			uint64				lsn;
			uuid_t				uuid;
			uint64				owner;
};

#define XFS_BLOCK_CRC_OFF offsetof(struct BlockInfoV5, crc)


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
	Dirents in version 3 directories have a file type field. Additions to this
	list are an on-disk format change, requiring feature bits. Valid values
	are as follows:
*/
#define XFS_DIR3_FT_UNKNOWN		0
#define XFS_DIR3_FT_REG_FILE	1
#define XFS_DIR3_FT_DIR			2
#define XFS_DIR3_FT_CHRDEV		3
#define XFS_DIR3_FT_BLKDEV		4
#define XFS_DIR3_FT_FIFO		5
#define XFS_DIR3_FT_SOCK		6
#define XFS_DIR3_FT_SYMLINK		7
#define XFS_DIR3_FT_WHT			8

#define XFS_DIR3_FT_MAX			9


/*
 * The dinode is the same for all types of inodes, the data and attribute
 * fork might be different and that is to be handled accordingly.
 */
class Inode {
public:
	typedef struct Dinode{
	public:
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
		xfs_aextnum_t		di_naextents;
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

		// XFS Version 5

		uint32				di_crc;
		uint64				di_changecount;
		uint64				di_lsn;
		uint64				di_flags2;
		uint32				di_cowextsize;
		uint8				di_pad2[12];

		// fields only written to during inode creation

		xfs_timestamp_t		di_crtime;
		uint64				di_ino;
		uuid_t				di_uuid;
	};

								Inode(Volume* volume, xfs_ino_t id);
								~Inode();

			status_t			Init();

			xfs_ino_t			ID() const { return fId; }

			bool				VerifyInode() const;

			bool				VerifyForkoff() const;

			bool				VerifyFork(int WhichFork) const;

			bool				IsDirectory() const
									{ return S_ISDIR(Mode()); }

			bool				IsFile() const
									{ return S_ISREG(Mode()); }

			bool				IsSymLink() const
									{ return S_ISLNK(Mode()); }

			mode_t				Mode() const { return fNode->di_mode; }

			Volume*				GetVolume() { return fVolume;}

			int8				Format() const { return fNode->di_format; }

			int8				AttrFormat() const { return fNode->di_aformat; }

			bool				IsLocal() const
									{ return Format() == XFS_DINODE_FMT_LOCAL; }

			uint32				NLink() const { return fNode->di_nlink; }

			int8				Version() const { return fNode->di_version; }

			xfs_rfsblock_t		BlockCount() const
									{ return fNode->di_nblocks; }

			char*				Buffer() { return fBuffer; }

			int16				Flags() const { return fNode->di_flags; }

			xfs_fsize_t			Size() const { return fNode->di_size; }

			uint32				DirBlockSize() const
									{ return fVolume->DirBlockSize(); }

			uint32				BlockSize() const
									{ return fVolume->BlockSize(); }

			uint32				CoreInodeSize() const;

			void				GetChangeTime(struct timespec& timestamp) const;

			void				GetModificationTime(struct timespec& timestamp) const;

			void				GetAccessTime(struct timespec& timestamp) const;

			void				GetCreationTime(struct timespec& timestamp) const;

			unsigned char		XfsModeToFtype() const;
			status_t			CheckPermissions(int accessMode) const;
			uint32				UserId() const { return fNode->di_uid; }
			uint32				GroupId() const { return fNode->di_gid; }
			bool				HasFileTypeField() const;
			xfs_extnum_t		DataExtentsCount() const
									{ return fNode->di_nextents; }
			xfs_extnum_t		AttrExtentsCount() const
									{ return fNode->di_naextents; }
			uint64				FileSystemBlockToAddr(uint64 block);
			uint8				ForkOffset() const
									{ return fNode->di_forkoff; }
			status_t			ReadExtents();
			status_t			ReadAt(off_t pos, uint8* buffer, size_t* length);
			status_t			GetNodefromTree(uint16& levelsInTree,
									Volume* volume, ssize_t& len,
									size_t DirBlockSize, char* block);
			int					SearchMapInAllExtent(uint64 blockNo);
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
			uint32				SizeOfLongBlock();
private:
			void				SwapEndian();
private:
			status_t			GetFromDisk();
			Dinode*				fNode;
			xfs_ino_t			fId;
			Volume*				fVolume;
			char*				fBuffer;
				// Contains the disk inode in BE format
			ExtentMapEntry*		fExtents;
};


uint32 hashfunction(const char* name, int length);


// A common function to return given hash lowerbound
template<class T> void
hashLowerBound(T* entry, int& left, int& right, uint32 hashValueOfRequest)
{
	int mid;

	/*
	* Trying to find the lowerbound of hashValueOfRequest
	* This is slightly different from bsearch(), as we want the first
	* instance of hashValueOfRequest and not any instance.
	*/
	while (left < right) {
		mid = (left + right) / 2;
		uint32 hashval = B_BENDIAN_TO_HOST_INT32(entry[mid].hashval);
		if (hashval >= hashValueOfRequest) {
			right = mid;
			continue;
		}
		if (hashval < hashValueOfRequest)
			left = mid+1;
	}
	TRACE("left:(%" B_PRId32 "), right:(%" B_PRId32 ")\n", left, right);
}

#endif
