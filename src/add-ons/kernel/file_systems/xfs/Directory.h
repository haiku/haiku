/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_


#include "Inode.h"


/*
 * This class should act as a layer between any kind of directory
 * and the kernel interface
 */
class DirectoryIterator {
public:
			virtual						~DirectoryIterator()							=	0;

			virtual	status_t			GetNext(char* name, size_t* length,
											xfs_ino_t* ino)								=	0;

			virtual	status_t			Lookup(const char* name, size_t length,
											xfs_ino_t* id)								=	0;

			static DirectoryIterator*	Init(Inode* inode);
};


#endif
