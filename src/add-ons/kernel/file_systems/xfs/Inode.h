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
#define INODE_CRC_OFF	offsetof(struct xfs_inode_t, di_crc)
#define MAXAEXTNUM	((xfs_aextnum_t) 0x7fff)
#define MAXEXTNUM	((xfs_extnum_t) 0x7fffffff)

// Does fs volume has v3 inodes?
#define HAS_V3INODES(volume)	(volume->IsVersion5() ? 1 : 0 )

// Inode size for given fs
#define DINODE_SIZE(volume) \
	(HAS_V3INODES(volume) ? \
		sizeof(struct xfs_inode_t) : offsetof(struct xfs_inode_t, di_crc))
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

#define DIR_AFORK_EXIST(dir_ino_ptr)	(dir_ino_ptr->di_forkoff != 0)

#define MASK(n)	((1UL << n) - 1)
#define FSBLOCKS_TO_AGNO(n, volume)	((n) >> volume->AgBlocksLog())
#define FSBLOCKS_TO_AGBLOCKNO(n, volume)	((n) & MASK(volume->AgBlocksLog()))
#define BLOCKNO_FROM_POSITION(n, volume) \
	((n) >> (volume->BlockLog()))
#define BLOCKOFFSET_FROM_POSITION(n, inode)	((n) & (inode->BlockSize() - 1))

#define MAXINUMBER	((xfs_ino_t)((1ULL << 56) - 1ULL))

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

#define XFS_BLOCK_CRC_OFF offsetof(struct BlockInfo, crc)


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


#define XFS_INODE_FORMAT_STR \
	{ XFS_DINODE_FMT_DEV,		"dev" }, \
	{ XFS_DINODE_FMT_LOCAL,		"local" }, \
	{ XFS_DINODE_FMT_EXTENTS,	"extent" }, \
	{ XFS_DINODE_FMT_BTREE,		"btree" }, \
	{ XFS_DINODE_FMT_UUID,		"uuid" }


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
			void				GetCreationTime(struct timespec& timestamp);

			int8				Format() const;
				// The format of the inode
			int8				AttrFormat() const;
			xfs_fsize_t			Size() const;
			xfs_rfsblock_t		BlockCount() const;
			uint32				NLink() const;
			uint16				Flags() const;
			uint32				UserId() const;
			uint32				GroupId() const;
			xfs_extnum_t		DataExtentsCount() const;
			xfs_extnum_t		AttrExtentsCount() const;
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


// Values for di_flags
#define XFS_DIFLAG_REALTIME_BIT  0	// file's blocks come from rt area
#define XFS_DIFLAG_PREALLOC_BIT  1	// file space has been preallocated
#define XFS_DIFLAG_NEWRTBM_BIT   2	// for rtbitmap inode, new format
#define XFS_DIFLAG_IMMUTABLE_BIT 3	// inode is immutable
#define XFS_DIFLAG_APPEND_BIT    4	// inode is append-only
#define XFS_DIFLAG_SYNC_BIT      5	// inode is written synchronously
#define XFS_DIFLAG_NOATIME_BIT   6	// do not update atime
#define XFS_DIFLAG_NODUMP_BIT    7	// do not dump
#define XFS_DIFLAG_RTINHERIT_BIT 8	// create with realtime bit set
#define XFS_DIFLAG_PROJINHERIT_BIT   9	// create with parents projid
#define XFS_DIFLAG_NOSYMLINKS_BIT   10	// disallow symlink creation
#define XFS_DIFLAG_EXTSIZE_BIT      11	// inode extent size allocator hint
#define XFS_DIFLAG_EXTSZINHERIT_BIT 12	// inherit inode extent size
#define XFS_DIFLAG_NODEFRAG_BIT     13	// do not reorganize/defragment
#define XFS_DIFLAG_FILESTREAM_BIT   14  // use filestream allocator

#define XFS_DIFLAG_REALTIME      (1 << XFS_DIFLAG_REALTIME_BIT)
#define XFS_DIFLAG_PREALLOC      (1 << XFS_DIFLAG_PREALLOC_BIT)
#define XFS_DIFLAG_NEWRTBM       (1 << XFS_DIFLAG_NEWRTBM_BIT)
#define XFS_DIFLAG_IMMUTABLE     (1 << XFS_DIFLAG_IMMUTABLE_BIT)
#define XFS_DIFLAG_APPEND        (1 << XFS_DIFLAG_APPEND_BIT)
#define XFS_DIFLAG_SYNC          (1 << XFS_DIFLAG_SYNC_BIT)
#define XFS_DIFLAG_NOATIME       (1 << XFS_DIFLAG_NOATIME_BIT)
#define XFS_DIFLAG_NODUMP        (1 << XFS_DIFLAG_NODUMP_BIT)
#define XFS_DIFLAG_RTINHERIT     (1 << XFS_DIFLAG_RTINHERIT_BIT)
#define XFS_DIFLAG_PROJINHERIT   (1 << XFS_DIFLAG_PROJINHERIT_BIT)
#define XFS_DIFLAG_NOSYMLINKS    (1 << XFS_DIFLAG_NOSYMLINKS_BIT)
#define XFS_DIFLAG_EXTSIZE       (1 << XFS_DIFLAG_EXTSIZE_BIT)
#define XFS_DIFLAG_EXTSZINHERIT  (1 << XFS_DIFLAG_EXTSZINHERIT_BIT)
#define XFS_DIFLAG_NODEFRAG      (1 << XFS_DIFLAG_NODEFRAG_BIT)
#define XFS_DIFLAG_FILESTREAM    (1 << XFS_DIFLAG_FILESTREAM_BIT)

#define XFS_DIFLAG_ANY \
	(XFS_DIFLAG_REALTIME | XFS_DIFLAG_PREALLOC | XFS_DIFLAG_NEWRTBM | \
	 XFS_DIFLAG_IMMUTABLE | XFS_DIFLAG_APPEND | XFS_DIFLAG_SYNC | \
	 XFS_DIFLAG_NOATIME | XFS_DIFLAG_NODUMP | XFS_DIFLAG_RTINHERIT | \
	 XFS_DIFLAG_PROJINHERIT | XFS_DIFLAG_NOSYMLINKS | XFS_DIFLAG_EXTSIZE | \
	 XFS_DIFLAG_EXTSZINHERIT | XFS_DIFLAG_NODEFRAG | XFS_DIFLAG_FILESTREAM)

/*
	Values for di_flags2 These start by being exposed to userspace in the upper
	16 bits of the XFS_XFLAG_S range.
*/
#define XFS_DIFLAG2_DAX_BIT	0	// use DAX for this inode
#define XFS_DIFLAG2_REFLINK_BIT	1	// file's blocks may be shared
#define XFS_DIFLAG2_COWEXTSIZE_BIT 2  // copy on write extent size hint
#define XFS_DIFLAG2_BIGTIME_BIT	3	// big timestamps

#define XFS_DIFLAG2_DAX		(1 << XFS_DIFLAG2_DAX_BIT)
#define XFS_DIFLAG2_REFLINK     (1 << XFS_DIFLAG2_REFLINK_BIT)
#define XFS_DIFLAG2_COWEXTSIZE  (1 << XFS_DIFLAG2_COWEXTSIZE_BIT)
#define XFS_DIFLAG2_BIGTIME	(1 << XFS_DIFLAG2_BIGTIME_BIT)

#define XFS_DIFLAG2_ANY \
	(XFS_DIFLAG2_DAX | XFS_DIFLAG2_REFLINK | XFS_DIFLAG2_COWEXTSIZE | \
	 XFS_DIFLAG2_BIGTIME)

class Inode {
public:
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

			mode_t				Mode() const { return fNode->Mode(); }

			Volume*				GetVolume() { return fVolume;}

			int8				Format() const { return fNode->Format(); }

			int8				AttrFormat() const { return fNode->AttrFormat(); }

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

			uint32				CoreInodeSize() const;

			void				GetChangeTime(struct timespec& timestamp) const
								{ fNode->GetChangeTime(timestamp); }

			void				GetModificationTime(struct timespec& timestamp)
									const
								{ fNode->GetModificationTime(timestamp); }

			void				GetAccessTime(struct timespec& timestamp) const
								{ fNode->GetAccessTime(timestamp); }

			void				GetCreationTime(struct timespec& timestamp) const
								{ fNode->GetCreationTime(timestamp); }

			unsigned char		XfsModeToFtype() const;
			status_t			CheckPermissions(int accessMode) const;
			uint32				UserId() const { return fNode->UserId(); }
			uint32				GroupId() const { return fNode->GroupId(); }
			bool				HasFileTypeField() const;
			xfs_extnum_t		DataExtentsCount() const
									{ return fNode->DataExtentsCount(); }
			xfs_extnum_t		AttrExtentsCount() const
									{ return fNode->AttrExtentsCount(); }
			uint64				FileSystemBlockToAddr(uint64 block);
			uint8				ForkOffset() const
									{ return fNode->ForkOffset(); }
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
			status_t			GetFromDisk();
			xfs_inode_t*		fNode;
			xfs_ino_t			fId;
			Volume*				fVolume;
			char*				fBuffer;
				// Contains the disk inode in BE format
			ExtentMapEntry*		fExtents;
};

#endif
