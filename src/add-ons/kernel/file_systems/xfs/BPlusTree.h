/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BPLUS_TREE_H_
#define _BPLUS_TREE_H_


#include "Extent.h"
#include "Inode.h"
#include "LeafDirectory.h"
#include "Node.h"
#include "system_dependencies.h"


#define XFS_BTREE_SBLOCK_SIZE	18
	// Header for Short Format btree
#define XFS_BTREE_LBLOCK_SIZE	24
	// Header for Long Format btree
#define XFS_KEY_SIZE sizeof(xfs_fileoff_t)
#define XFS_PTR_SIZE sizeof(xfs_fsblock_t)
#define XFS_BMAP_MAGIC 0x424d4150


typedef xfs_fileoff_t TreeKey;
typedef xfs_fsblock_t TreePointer;


/*
 * Headers(here, the LongBlock) are the "nodes" really and are called "blocks".
 * The records, keys and ptrs are calculated using helpers
 */
struct LongBlock {

			uint32				Magic()
								{ return B_BENDIAN_TO_HOST_INT32(bb_magic); }

			uint16				Level()
								{ return B_BENDIAN_TO_HOST_INT16(bb_level); }

			uint16				NumRecs()
								{ return B_BENDIAN_TO_HOST_INT16(bb_numrecs); }

			TreePointer			Left()
								{ return B_BENDIAN_TO_HOST_INT64(bb_leftsib); }

			TreePointer			Right()
								{ return B_BENDIAN_TO_HOST_INT64(bb_rightsib); }

			uint32				bb_magic;
			uint16				bb_level;
			uint16				bb_numrecs;
			uint64				bb_leftsib;
			uint64				bb_rightsib;
};


/* We have an array of extent records in
 * the leaf node along with above headers
 * The behaviour is very much like node directories.
 */


//xfs_bmdr_block
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


struct ExtentMapUnwrap {
			uint64				first;
			uint64				second;
};


/*
 * This class should handle B+Tree based directories
 */
class TreeDirectory {
public:
								TreeDirectory(Inode* inode);
								~TreeDirectory();
			status_t			InitCheck();
			status_t			GetNext(char* name, size_t* length,
									xfs_ino_t* ino);
			status_t			Lookup(const char* name, size_t length,
									xfs_ino_t* id);
			int					EntrySize(int len) const;
			int					BlockLen();
			size_t				PtrSize();
			size_t				KeySize();
			TreeKey				GetKey(int pos);
									// get the pos'th key
			TreePointer*		GetPtr(int pos, LongBlock* pointer);
									// get the pos'th pointer
			status_t			GetAllExtents();
			size_t				MaxRecordsPossible(size_t len);

private:
	inline	status_t			UnWrapExtents(ExtentMapUnwrap* extentsWrapped);

private:
			Inode*				fInode;
			status_t			fInitStatus;
			BlockInDataFork*	fRoot;
			ExtentMapEntry*		fExtents;
			uint32				fCountOfFilledExtents;
			char*				fSingleDirBlock;
};


#endif
