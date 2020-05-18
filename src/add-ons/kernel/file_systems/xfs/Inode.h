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
#define INODE_CORE_SIZE 96				// For v4 FS
#define INODE_CORE_UNLINKED_SIZE 100	//For v4 FS
#define DATA_FORK_OFFSET 0x64

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
#define INO_TO_OFFSET(id, volume) (id & INO_MASK(volume->InodesPerBlkLog()))
	// Gets the offset into the block from the inode number


typedef struct xfs_timestamp
{
		int32				t_sec;
		int32				t_nsec;
} xfs_timestamp_t;


typedef enum xfs_dinode_fmt
{
		XFS_DINODE_FMT_DEV,		// For devices
		XFS_DINODE_FMT_LOCAL,	// For Directories and links
		XFS_DINODE_FMT_EXTENTS,	// For large number of extents
		XFS_DINODE_FMT_BTREE,	// Extents and Directories
		XFS_DINODE_FMT_UUID,	// Not used
} xfs_dinode_fmt_t;


/*
	The xfs_ino_t is the same for all types of inodes, the data and attribute
	fork might be different and that is to be handled accordingly.
*/
typedef struct xfs_inode
{
		void				SwapEndian();
		void				DirIsV2();		//TODO
		mode_t				Mode();
		void				GetModificationTime();
		int8				Format();		// The format of the inode

		uint16				di_magic;
		uint16				di_mode;		// uses standard S_Ixxx
		int8				di_version;		// 1 or 2
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
		xfs_fsize_t			di_size;		// size in bytes or length if link
		xfs_rfsblock_t		di_nblocks;		// blocks used including metadata
											// extended attributes not included
		xfs_extlen_t		di_extsize;		// extent size
		xfs_extnum_t		di_nextents;	// number of data extents
		xfs_aextnum_t		di_anextents;	// number of EA extents
		uint8				di_forkoff;		// decides where di_a starts
		int8				di_aformat;		// similar to di_format
		uint32				di_dmevmask;
		uint16				di_dmstate;
		uint16				di_flags;
		uint32				di_gen;

		uint32				di_next_unlinked;
} xfs_inode_t;


class Inode
{
	public:
					Inode(Volume* volume, xfs_ino_t id);
					~Inode();
		bool		InitCheck();
		bool		IsDirectory() const
						{ return S_ISDIR(Mode()); }

		bool		IsFile() const
						{ return S_ISREG(Mode()); }

		bool		IsSymLink() const
						{ return S_ISLNK(Mode()); }

		mode_t		Mode() const { return fNode->Mode(); }

		Volume*		GetVolume() { return fVolume;}

		int8		Format() { return fNode->Format(); }

		bool		IsLocal() { return Format() == XFS_DINODE_FMT_LOCAL; }

	#if 0
		int32		Flags() const { return fNode->Flags(); }

		off_t		Size() const { return fNode->Size(); }

		void		GetChangeTime(xfs_timestamp_t *stamp) const
					{ fNode.GetChangeTime(stamp, fHasExtraAttributes); }

		void		GetModificationTime(xfs_timestamp_t *stamp) const
					{ fNode.GetModificationTime(stamp,
						fHasExtraAttributes); }

		void		GetCreationTime(xfs_timestamp_t *stamp) const
					{ fNode.GetCreationTime(stamp,
						fHasExtraAttributes); }

		void		GetAccessTime(xfs_timestamp_t *stamp) const
					{ fNode.GetAccessTime(stamp, fHasExtraAttributes); }
	#endif

	private:

		status_t			GetFromDisk();
		xfs_inode_t*		fNode;
		xfs_ino_t			fId;
		Volume*				fVolume;
		char*				fBuffer;	// Contains the disk inode in BE format
};

#endif
