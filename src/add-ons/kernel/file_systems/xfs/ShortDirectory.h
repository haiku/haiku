/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef __SHORT_DIR_H__
#define __SHORT_DIR_H__


#include "Inode.h"


/*
 * offset into the literal area
 * xfs_dir2_sf_off_t
 */
struct ShortFormOffset {
			uint16				i;
} __attribute__((packed));

//xfs_dir2_inou_t
union ShortFormInodeUnion {
			uint64				i8;
			uint32				i4;
} __attribute__((packed));


// xfs_dir2_sf_hdr_t
struct ShortFormHeader {
			uint8				count;
				// number of entries
			uint8				i8count;
				// # of 64bit inode entries
			ShortFormInodeUnion	parent;
				// absolute inode # of parent
} __attribute__((packed));


//xfs_dir2_sf_entry_t
struct ShortFormEntry {
			uint8				namelen;
				// length of the name, in bytes
			ShortFormOffset		offset;
				// offset tag, for directory iteration
			uint8				name[];
				// name of directory entry
/*
 * Following will be a single byte file type variable
 * and inode number (64bits or 32 bits)
 */
} __attribute__((packed));


class ShortDirectory
{
public:
								ShortDirectory(Inode* inode);
								~ShortDirectory();
			size_t				HeaderSize();
			uint8				GetFileType(ShortFormEntry* entry);
			ShortFormEntry*		FirstEntry();
			xfs_ino_t			GetIno(ShortFormInodeUnion* inum);
			xfs_ino_t			GetEntryIno(ShortFormEntry* entry);
			size_t				EntrySize(int namelen);
			status_t			GetNext(char* name, size_t* length,
									xfs_ino_t* ino);
			status_t			Lookup(const char* name, size_t length,
									xfs_ino_t* id);
private:
			Inode*				fInode;
			ShortFormHeader*	fHeader;
			uint16				fLastEntryOffset;
			// offset into the literal area
			uint8				fTrack;
			// Takes the values 0, 1 or 2
};

#endif
