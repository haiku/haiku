/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef INDEXED_DIRECTORY_ITERATOR_H
#define INDEXED_DIRECTORY_ITERATOR_H


#include "DirectoryIterator.h"


class HTreeEntryIterator;

class IndexedDirectoryIterator : public DirectoryIterator {
public:
								IndexedDirectoryIterator(off_t start, 
									uint32 blockSize, Inode* directory,
									HTreeEntryIterator* parent);
	virtual						~IndexedDirectoryIterator();
	
			status_t			GetNext(char* name, size_t* nameLength, 
									ino_t* id);
			
			status_t			Rewind();
private:
			bool				fIndexing;
			HTreeEntryIterator*	fParent;
			off_t				fMaxOffset;
			uint32				fBlockSize;
			uint32				fMaxAttempts;
};

#endif	// INDEXED_DIRECTORY_ITERATOR_H
