/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef XFS_SYMLINK_H
#define XFS_SYMLINK_H


#include "Inode.h"


#define SYMLINK_MAGIC 0x58534c4d


// Used only on Version 5
struct SymlinkHeader {
public:

			uint32				Magic()
								{ return B_BENDIAN_TO_HOST_INT32(sl_magic); }

			uint64				Blockno()
								{ return B_BENDIAN_TO_HOST_INT64(sl_blkno); }

			const uuid_t&		Uuid()
								{ return sl_uuid; }

			uint64				Owner()
								{ return B_BENDIAN_TO_HOST_INT64(sl_owner); }

	static	uint32				ExpectedMagic(int8 whichDirectory, Inode* inode)
								{ return SYMLINK_MAGIC; }

	static	uint32				CRCOffset()
								{ return offsetof(SymlinkHeader, sl_crc); }

private:
			uint32				sl_magic;
			uint32				sl_offset;
			uint32				sl_bytes;
			uint32				sl_crc;
			uuid_t				sl_uuid;
			uint64				sl_owner;
			uint64				sl_blkno;
			uint64				sl_lsn;
};


// This class will handle all formats of Symlinks in xfs
class Symlink {
public:
								Symlink(Inode* inode);
								~Symlink();
			status_t			ReadLink(off_t pos, char* buffer, size_t* _length);
private:
			status_t			_FillMapEntry();
			status_t			_FillBuffer();
			status_t			_ReadLocalLink(off_t pos, char* buffer, size_t* _length);
			status_t			_ReadExtentLink(off_t pos, char* buffer, size_t* _length);

			Inode*				fInode;
			ExtentMapEntry		fMap;
			char*				fSymlinkBuffer;
};

#endif
