/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef LEAF_ATTRIBUTE_H
#define LEAF_ATTRIBUTE_H


#include "Attribute.h"
#include "Inode.h"


#define	XFS_ATTR_LEAF_MAPSIZE	3
#define	XFS_ATTR_LEAF_MAGIC		0xfbee
#define	XFS_ATTR3_LEAF_MAGIC	0x3bee
#define	XFS_ATTR_LOCAL_BIT		0
#define XFS_ATTR_LOCAL			(1 << XFS_ATTR_LOCAL_BIT)
#define	XFS_ATTR3_RMT_MAGIC		0x5841524d


// Enum values to check which directory we are reading
enum AttrType {
	ATTR_LEAF,
	ATTR_NODE,
	ATTR_BTREE,
};


// xfs_attr_leaf_map
struct AttrLeafMap {
	uint16	base;
	uint16	size;
};


// This class will act as interface for V4 and V5 Attr leaf header
class AttrLeafHeader {
public:

			virtual						~AttrLeafHeader()			=	0;
			virtual uint16				Magic()						=	0;
			virtual uint64				Blockno()					=	0;
			virtual uint64				Owner()						=	0;
			virtual const uuid_t&		Uuid()						=	0;
			virtual	uint16				Count()						=	0;
			static	uint32				ExpectedMagic(int8 WhichDirectory,
										Inode* inode);
			static	uint32				CRCOffset();
			static	AttrLeafHeader*		Create(Inode* inode, const char* buffer);
			static	uint32				Size(Inode* inode);
};


//xfs_attr_leaf_hdr
class AttrLeafHeaderV4 : public AttrLeafHeader {
public:
			struct OnDiskData {
			public:
				uint16				count;
				uint16				usedbytes;
				uint16				firstused;
				uint8				holes;
				uint8				pad1;
				AttrLeafMap			freemap[3];
				BlockInfo			info;
			};

								AttrLeafHeaderV4(const char* buffer);
								~AttrLeafHeaderV4();
			void				SwapEndian();
			uint16				Magic();
			uint64				Blockno();
			uint64				Owner();
			const uuid_t&		Uuid();
			uint16				Count();

private:
			OnDiskData			fData;
};


//xfs_attr3_leaf_hdr
class AttrLeafHeaderV5 : public AttrLeafHeader {
public:
			struct OnDiskData {
			public:
				uint16				count;
				uint16				usedbytes;
				uint16				firstused;
				uint8				holes;
				uint8				pad1;
				AttrLeafMap			freemap[3];
				uint32				pad2;
				BlockInfoV5			info;
			};

								AttrLeafHeaderV5(const char* buffer);
								~AttrLeafHeaderV5();
			void				SwapEndian();
			uint16				Magic();
			uint64				Blockno();
			uint64				Owner();
			const uuid_t&		Uuid();
			uint16				Count();

private:
			OnDiskData			fData;
};


// xfs_attr_leaf_entry
struct AttrLeafEntry {
	uint32		hashval;
	uint16		nameidx;
	uint8		flags;
	uint8		pad2;
};


// xfs_attr_leaf_name_local
struct AttrLeafNameLocal {
	uint16		valuelen;
	uint8		namelen;
	uint8		nameval[];
};


// xfs_attr_leaf_name_remote
struct AttrLeafNameRemote {
	uint32		valueblk;
	uint32		valuelen;
	uint8		namelen;
	uint8		name[];
};


struct AttrRemoteHeader {
	uint32		rm_magic;
	uint32		rm_offset;
	uint32		rm_bytes;
	uint32		rm_crc;
	uuid_t		rm_uuid;
	uint64		rm_blkno;
	uint64		rm_lsn;
};

#define XFS_ATTR3_RMT_CRC_OFF	offsetof(struct AttrRemoteHeader, rm_crc)


class LeafAttribute : public Attribute {
public:
								LeafAttribute(Inode* inode);
								~LeafAttribute();

			status_t			Init();

			status_t			Stat(attr_cookie* cookie, struct stat& stat);

			status_t			Read(attr_cookie* cookie, off_t pos,
									uint8* buffer, size_t* length);

			status_t			Open(const char* name, int openMode, attr_cookie** _cookie);

			status_t			GetNext(char* name, size_t* nameLength);

			status_t			Lookup(const char* name, size_t* nameLength);
private:
			status_t			_FillMapEntry();

			status_t			_FillLeafBuffer();

			Inode*				fInode;
			const char*			fName;
			ExtentMapEntry*		fMap;
			char*				fLeafBuffer;
			uint16				fLastEntryOffset;
			AttrLeafNameLocal*	fLocalEntry;
			AttrLeafNameRemote*	fRemoteEntry;
};

#endif
