/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BPLUS_TREE_H_
#define _BPLUS_TREE_H_

#include "system_dependencies.h"


/* Allocation B+ Tree Format */
#define XFS_ABTB_MAGICNUM 0x41425442
	// For block offset B+Tree
#define XFS_ABTC_MAGICNUM 0x41425443
	// For block count B+ Tree
#define XFS_BTREE_SBLOCK_SIZE	18
	// Header for Short Format btree
#define XFS_BTREE_LBLOCK_SIZE	24
	// Header for Long Format btree


/*
 * Headers are the "nodes" really and are called "blocks". The records, keys
 * and ptrs are calculated using helpers
 */
struct bplustree_short_block {
			void				SwapEndian();

			uint32				Magic()
								{ return bb_magic; }

			uint16				Level()
								{ return bb_level; }

			uint16				NumRecs()
								{ return bb_numrecs; }

			xfs_alloc_ptr_t		Left()
								{ return bb_leftsib; }

			xfs_alloc_ptr_t		Right()
								{ return bb_rightsib;}

			uint32				bb_magic;
			uint16				bb_level;
			uint16				bb_numrecs;
			uint32				bb_leftsib;
			uint32				bb_rightsib;
}


struct bplustree_long_block {
			void				SwapEndian();

			uint32				Magic()
								{ return bb_magic; }

			uint16				Level()
								{ return bb_level; }

			uint16				NumRecs()
								{ return bb_numrecs; }

			xfs_alloc_ptr_t		Left()
								{ return bb_leftsib; }

			xfs_alloc_ptr_t		Right()
								{ return bb_rightsib;}

			uint32				bb_magic;
			uint16				bb_level;
			uint16				bb_numrecs;
			uint64				bb_leftsib;
			uint64				bb_rightsib;
}


/* Array of these records in the leaf node along with above headers */
#define XFS_ALLOC_REC_SIZE	8
typedef struct xfs_alloc_rec {
			void				SwapEndian();

			uint32				StartBlock()
								{ return ar_startblock; }

			uint32				BlockCount()
								{ return ar_blockcount; }

			uint32				ar_startblock;
			uint32				ar_blockcount;

} xfs_alloc_rec_t, xfs_alloc_key_t;


typedef uint32 xfs_alloc_ptr_t;
	//  Node pointers, AG relative block pointer

#define ALLOC_FLAG 0x1
#define LONG_BLOCK_FLAG 0x1
#define SHORT_BLOCK_FLAG 0x2

union btree_ptr {
	bplustree_long_block fLongBlock;
	bplustree_short_block fShortBlock;
}


union btree_key {
	xfs_alloc_key_t fAlloc;
}


union btree_rec {
	xfs_alloc_rec_t fAlloc;
}


class BPlusTree {
public:
			uint32				BlockSize();
			int					RecordSize();
			int					MaxRecords(bool leaf);
			int					KeyLen();
			int					BlockLen();
			int					PtrLen();
			int					RecordOffset(int pos); // get the pos'th record
			int					KeyOffset(int pos); // get the pos'th key
			int					PtrOffset(int pos); // get the pos'th ptr

private:
			Volume*				fVolume;
			xfs_agnumber_t		fAgnumber;
			btree_ptr*			fRoot;
			int					fRecType;
			int					fKeyType;
			int					fPtrType;
}


#endif
