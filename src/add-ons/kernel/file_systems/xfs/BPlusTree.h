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


struct ExtentMapUnwrap {
			uint64				first;
			uint64				second;
};


/*
 * Using the structure to prevent re-reading of already read blocks during
 * a traversal of tree.
 *
 * type:
 * 0, if its an unused node, 1 if blockData size is a single block,
 * 2 if blockData size is directory block size.
 */
struct PathNode {
			int					type;
			char*				blockData;
			uint32				blockNumber;
				// This is the file system block number
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
			TreeKey*			GetKeyFromNode(int pos, void* buffer);
			TreePointer*		GetPtrFromNode(int pos, void* buffer);
			TreeKey*			GetKeyFromRoot(int pos);
			TreePointer*		GetPtrFromRoot(int pos);
			status_t			SearchMapInAllExtent(int blockNo,
									uint32& mapIndex);
			status_t			GetAllExtents();
			size_t				MaxRecordsPossibleRoot();
			size_t				MaxRecordsPossibleNode();
			void				FillMapEntry(int num, ExtentMapEntry** map,
									int type, int pathIndex);
			status_t			FillBuffer(char* blockBuffer,
									int howManyBlocksFurther,
									ExtentMapEntry* targetMap = NULL);
			size_t				GetPtrOffsetIntoNode(int pos);
			size_t				GetPtrOffsetIntoRoot(int pos);
			status_t			SearchAndFillPath(uint32 offset, int type);
			status_t			SearchOffsetInTreeNode (uint32 offset,
									TreePointer** pointer, int pathIndex);
			void				SearchForMapInDirectoryBlock (int blockNo,
									int entries, ExtentMapEntry** map,
									int type, int pathIndex);
			uint32				SearchForHashInNodeBlock(uint32 hashVal);
private:
	inline	status_t			UnWrapExtents(ExtentMapUnwrap* extentsWrapped);

private:
			Inode*				fInode;
			status_t			fInitStatus;
			BlockInDataFork*	fRoot;
			ExtentMapEntry*		fExtents;
			uint32				fCountOfFilledExtents;
			char*				fSingleDirBlock;
			uint32				fOffsetOfSingleDirBlock;
			uint32				fCurMapIndex;
			uint64				fOffset;
			uint32				fCurBlockNumber;
			PathNode			fPathForLeaves[MAX_TREE_DEPTH];
			PathNode			fPathForData[MAX_TREE_DEPTH];
};


#endif
