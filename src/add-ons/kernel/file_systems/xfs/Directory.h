/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_


#include "Extent.h"
#include "Inode.h"
#include "LeafDirectory.h"
#include "ShortDirectory.h"


/*
 * This class should act as a layer between any kind of directory
 * and the kernel interface
 */
class DirectoryIterator {
public:
								DirectoryIterator(Inode* inode);
								~DirectoryIterator();
			status_t			Init();
			bool				IsLocalDir() { return fInode->IsLocal(); }
			status_t			GetNext(char* name, size_t* length,
									xfs_ino_t* ino);
			status_t			Lookup(const char* name, size_t length,
									xfs_ino_t* id);

private:
			Inode*				fInode;
			ShortDirectory*		fShortDir;
				// Short form Directory type
			Extent*				fExtentDir;
				// Extent form Directory type
				// TODO: Rename all to block type
			LeafDirectory*		fLeafDir;
				// Extent based leaf directory
};


#endif
