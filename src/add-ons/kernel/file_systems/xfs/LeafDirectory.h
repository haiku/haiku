/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _LEAFDIRECTORY_H_
#define _LEAFDIRECTORY_H_


#include "Directory.h"
#include "Extent.h"
#include "Inode.h"
#include "system_dependencies.h"


#define V4_DATA_HEADER_MAGIC 0x58443244
#define V5_DATA_HEADER_MAGIC 0x58444433

#define V4_LEAF_HEADER_MAGIC 0xd2f1
#define V5_LEAF_HEADER_MAGIC 0x3df1


enum ContentType { DATA, LEAF };


// This class will act as interface for V4 and V5 leaf header
class ExtentLeafHeader {
public:

			virtual						~ExtentLeafHeader()		=	0;
			virtual	uint16				Magic()					=	0;
			virtual	uint64				Blockno()				=	0;
			virtual	uint64				Lsn()					=	0;
			virtual	uint64				Owner()					=	0;
			virtual	uuid_t*				Uuid()					=	0;
			virtual	uint16				Count()					=	0;
			virtual	uint32				Forw()					=	0;
			static	uint32				ExpectedMagic(int8 WhichDirectory,
										Inode* inode);
			static	uint32				CRCOffset();
			static	ExtentLeafHeader*	Create(Inode* inode, const char* buffer);
			static	uint32				Size(Inode* inode);

};


//xfs_dir_leaf_hdr_t
class ExtentLeafHeaderV4 : public ExtentLeafHeader {
public:

								ExtentLeafHeaderV4(const char* buffer);
								~ExtentLeafHeaderV4();
			void				SwapEndian();
			uint16				Magic();
			uint64				Blockno();
			uint64				Lsn();
			uint64				Owner();
			uuid_t*				Uuid();
			uint16				Count();
			uint32				Forw();

			BlockInfo			info;
private:
			uint16				count;
			uint16				stale;
};


// xfs_dir3_leaf_hdr_t
class ExtentLeafHeaderV5 : public ExtentLeafHeader {
public:

								ExtentLeafHeaderV5(const char* buffer);
								~ExtentLeafHeaderV5();
			void				SwapEndian();
			uint16				Magic();
			uint64				Blockno();
			uint64				Lsn();
			uint64				Owner();
			uuid_t*				Uuid();
			uint16				Count();
			uint32				Forw();

			BlockInfoV5			info;
private:
			uint16				count;
			uint16				stale;
			uint32				pad;
};

#define XFS_LEAF_CRC_OFF offsetof(ExtentLeafHeaderV5, info.crc)
#define XFS_LEAF_V5_VPTR_OFF offsetof(ExtentLeafHeaderV5, info.forw)
#define XFS_LEAF_V4_VPTR_OFF offsetof(ExtentLeafHeaderV4, info.forw)


// xfs_dir2_leaf_tail_t
struct ExtentLeafTail {
			uint32				bestcount;
				// # of best free entries
};


class LeafDirectory : public DirectoryIterator {
public:
								LeafDirectory(Inode* inode);
								~LeafDirectory();
			status_t			Init();
			bool				IsLeafType();
			void				FillMapEntry(int num, ExtentMapEntry* map);
			status_t			FillBuffer(int type, char* buffer,
									int howManyBlocksFurthur);
			void				SearchAndFillDataMap(uint64 blockNo);
			ExtentLeafEntry*	FirstLeaf();
			xfs_ino_t			GetIno();
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
};

#endif
