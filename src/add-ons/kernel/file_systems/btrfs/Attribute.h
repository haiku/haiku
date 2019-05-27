/*
 * Copyright 2019, Bharathi Ramana Joshi, joshibharathiramana@gmail.com.
 * Copyright 2017, Chế Vũ Gia Hy, cvghy116@gmail.com.
 * Copyright 2010-2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H


#include "CachedBlock.h"
#include "Inode.h"


struct attr_cookie {
	char	name[B_ATTR_NAME_LENGTH];
	uint32	type;
	int		open_mode;
	bool	create;
};


/** Attributes in btrfs use the same on-disk format as directories, in the
 * XATTR_ITEM of the inode. Each attribute appears as a "file" there.
 *
 * Additionally, this class provides access to the non-extended attributes
 * through the Stat method. These are stored dorectly in the base inode of
 * the file.
 */
class Attribute {
public:
			//! Constructs an Attribute object for the file inode
								Attribute(Inode* inode);
								Attribute(Inode* inode, attr_cookie* cookie);
								~Attribute();

			//! Tests if operations specified by openMode can be performed on file *name
			status_t			CheckAccess(const char* name, int openMode);

			status_t			Create(const char* name, type_code type,
									int openMode, attr_cookie** _cookie);
			//! Loads file attributes into cookie
			status_t			Open(const char* name, int openMode,
									attr_cookie** _cookie);

			//! Get the stat from the attribute
			status_t			Stat(struct stat& stat);

			//! Loads extended attributes of file into buffer
			status_t			Read(attr_cookie* cookie, off_t pos,
									uint8* buffer, size_t* _length);
private:
			//! Searches through the filesystem tree for the entry named *name
			status_t			_Lookup(const char* name, size_t nameLength,
									btrfs_dir_entry** entries = NULL,
									uint32* length = NULL);
			//! Searches through the entry array for the entry named *name
			status_t			_FindEntry(btrfs_dir_entry* entries,
									size_t length, const char* name,
									size_t nameLength,
									btrfs_dir_entry** _entry);

			::Volume*			fVolume;
			Inode*				fInode;
			const char*			fName;
};

#endif	// ATTRIBUTE_H

