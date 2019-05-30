/*
 * Copyright 2017, Chế Vũ Gia Hy, cvghy116@gmail.com.
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef DIRECTORYITERATOR_H
#define DIRECTORYITERATOR_H


#include "BTree.h"
#include "Inode.h"


//! Class used to iterate through entries in a directory
class DirectoryIterator {
public:
								DirectoryIterator(Inode* inode);
								~DirectoryIterator();

			status_t			InitCheck();

			/*! Get details of next entry
			 * \param name Location to copy name of next entry
			 * \param _nameLength Location to copy length of next entry's name
			 * \param _id Location to copy inode number of next entry
			 */
			status_t			GetNext(char* name, size_t* _nameLength, ino_t* _id);
			/*! Search for item in current directory
			 * \param name Name of entry to lookup
			 * \param nameLength Length of name being searched
			 * \param _id inode value of entry if found, ??? otherwise
			 */
			status_t			Lookup(const char* name, size_t nameLength, ino_t* _id);
			//! Reset iterator to beginning
			status_t			Rewind();
private:
			uint64				fOffset;
			Inode* 				fInode;
			TreeIterator*		fIterator;
};


#endif	// DIRECTORYITERATOR_H
