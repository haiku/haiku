/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef DIRECTORYITERATOR_H
#define DIRECTORYITERATOR_H


#include "BTree.h"
#include "Inode.h"


class DirectoryIterator {
public:
								DirectoryIterator(Inode* inode);
								~DirectoryIterator();

			status_t			InitCheck();

			status_t			GetNext(char* name, size_t* _nameLength, ino_t* _id);
			status_t			Lookup(const char* name, size_t nameLength, ino_t* _id);
			status_t			Rewind();
private:
			uint64				fOffset;
			Inode* 				fInode;
			TreeIterator*		fIterator;
};


#endif	// DIRECTORYITERATOR_H
