/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef HTREE_ENTRY_ITERATOR_H
#define HTREE_ENTRY_ITERATOR_H


#include <AutoDeleter.h>

#include "DirectoryIterator.h"


class HTreeEntryIterator {
public:
								HTreeEntryIterator(off_t offset,
									Inode* directory);
								~HTreeEntryIterator();

			status_t			Init();
			
			status_t			Lookup(uint32 hash, int indirections,
									DirectoryIterator**	iterator);
			bool				HasCollision() { return fHasCollision; }
						
			status_t			GetNext(off_t& offset);
private:
								HTreeEntryIterator(uint32 block,
									uint32 blockSize, Inode* directory,
									HTreeEntryIterator* parent,
									bool hasCollision);

private:
			bool				fHasCollision;
			uint16				fLimit, fCount;

			uint32				fBlockSize;
			Inode*				fDirectory;
			off_t				fOffset;
			off_t				fMaxOffset;

			HTreeEntryIterator*	fParent;
			HTreeEntryIterator*	fChild;
			ObjectDeleter<HTreeEntryIterator> fChildDeleter;
};

#endif	// HTREE_ENTRY_ITERATOR_H
