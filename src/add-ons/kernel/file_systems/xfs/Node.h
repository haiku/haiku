/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _NODE_H_
#define _NODE_H_


#include "Extent.h"
#include "LeafDirectory.h"


#define XFS_DIR2_LEAFN_MAGIC (0xd2ff)
#define XFS_DA_NODE_MAGIC (0xfebe)


//xfs_da_node_hdr
struct NodeHeader {
			BlockInfo			info;
			uint16				count;
			uint16				level;
};


//xfs_da_node_entry
struct NodeEntry {
			uint32				hashval;
			uint32				before;
};


class NodeDirectory {
public:
								NodeDirectory(Inode* inode);
								~NodeDirectory();
			status_t			Init();
			bool				IsNodeType();
			void				FillMapEntry(int num, ExtentMapEntry* map);
			status_t			FillBuffer(int type, char* buffer,
									int howManyBlocksFurther);
			void				SearchAndFillDataMap(int blockNo);
			uint32				FindHashInNode(uint32 hashVal);
			uint32				GetOffsetFromAddress(uint32 address);
			int					EntrySize(int len) const;
			status_t			GetNext(char* name, size_t* length,
									xfs_ino_t* ino);
			status_t			Lookup(const char* name, size_t length,
									xfs_ino_t* id);
private:
			Inode*				fInode;
			ExtentMapEntry*		fDataMap;
			ExtentMapEntry*		fLeafMap;
			uint32				fOffset;
			char*				fDataBuffer;
				// This isn't inode data. It holds the directory block.
			char*				fLeafBuffer;
			uint32				fCurBlockNumber;
			uint8				fCurLeafMapNumber;
			uint8				fCurLeafBufferNumber;
};

#endif