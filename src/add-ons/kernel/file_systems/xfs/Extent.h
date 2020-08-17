/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _EXTENT_H_
#define _EXTENT_H_


#include "Inode.h"
#include "system_dependencies.h"


#define DIR2_BLOCK_HEADER_MAGIC 0x58443242
	// for v4 system
#define DIR2_FREE_TAG 0xffff
#define XFS_DIR2_DATA_FD_COUNT 3
#define EXTENT_REC_SIZE		128
#define EXTENT_SIZE 16
#define BLOCKNO_FROM_ADDRESS(n, volume) \
	((n) >> (volume->BlockLog() + volume->DirBlockLog()))
#define BLOCKOFFSET_FROM_ADDRESS(n, inode) ((n) & (inode->DirBlockSize() - 1))
#define LEAF_STARTOFFSET(n) 1UL << (35 - (n))


// xfs_exntst_t
enum ExtentState {
	XFS_EXT_NORM,
	XFS_EXT_UNWRITTEN,
	XFS_EXT_INVALID
};


// xfs_dir2_data_free_t
struct FreeRegion {
			uint16				offset;
			uint16				length;
};


// xfs_dir2_data_hdr_t
struct ExtentDataHeader {
			uint32				magic;
			FreeRegion			bestfree[XFS_DIR2_DATA_FD_COUNT];
};


// xfs_dir2_data_entry_t
struct ExtentDataEntry {
			xfs_ino_t			inumber;
			uint8				namelen;
			uint8				name[];

// Followed by a file type (8bit) if applicable and a 16bit tag
// tag is the offset from start of the block
};


// xfs_dir2_data_unused_t
struct ExtentUnusedEntry {
			uint16				freetag;
				// takes the value 0xffff
			uint16				length;
				// freetag+length overrides the inumber of an entry
			uint16				tag;
};


// xfs_dir2_leaf_entry_t
struct ExtentLeafEntry {
			uint32				hashval;
			uint32				address;
				// offset into block after >> 3
};


// xfs_dir2_block_tail_t
struct ExtentBlockTail {
			uint32				count;
				// # of entries in leaf
			uint32				stale;
				// # of free leaf entries
};


class Extent
{
public:
								Extent(Inode* inode);
								~Extent();
			status_t			Init();
			bool				IsBlockType();
			void				FillMapEntry(void* pointerToMap);
			status_t			FillBlockBuffer();
			ExtentBlockTail*	BlockTail();
			ExtentLeafEntry*	BlockFirstLeaf(ExtentBlockTail* tail);
			xfs_ino_t			GetIno();
			uint32				GetOffsetFromAddress(uint32 address);
			int					EntrySize(int len) const;
			status_t			GetNext(char* name, size_t* length,
									xfs_ino_t* ino);
			status_t			Lookup(const char* name, size_t length,
									xfs_ino_t* id);
private:
			Inode*				fInode;
			ExtentMapEntry*		fMap;
			uint32				fOffset;
			char*				fBlockBuffer;
				// This isn't inode data. It holds the directory block.
};


#endif
