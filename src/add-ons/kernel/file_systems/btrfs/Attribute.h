/*
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


class Attribute {
public:
								Attribute(Inode* inode);
								Attribute(Inode* inode, attr_cookie* cookie);
								~Attribute();

			status_t			CheckAccess(const char* name, int openMode);

			status_t			Create(const char* name, type_code type,
									int openMode, attr_cookie** _cookie);
			status_t			Open(const char* name, int openMode,
									attr_cookie** _cookie);

			status_t			Stat(struct stat& stat);

			status_t			Read(attr_cookie* cookie, off_t pos,
									uint8* buffer, size_t* _length);
private:
			status_t			_Lookup(const char* name, size_t nameLength,
									btrfs_dir_entry** entries = NULL,
									size_t* length = NULL);
			status_t			_FindEntry(btrfs_dir_entry* entries,
									size_t length, const char* name,
									size_t nameLength,
									btrfs_dir_entry** _entry);

			::Volume*			fVolume;
			Inode*				fInode;
			const char*			fName;
};

#endif	// ATTRIBUTE_H

