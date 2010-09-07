/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef DIRECTORY_ITERATOR_H
#define DIRECTORY_ITERATOR_H


#include <SupportDefs.h>

#include "Transaction.h"


class HTreeEntryIterator;
class Inode;

class DirectoryIterator {
public:
						DirectoryIterator(Inode* inode, off_t start = 0,
							HTreeEntryIterator* parent = NULL);
						~DirectoryIterator();

			status_t	InitCheck();


			status_t	Next();
			status_t	Get(char* name, size_t* _nameLength, ino_t* id);

			status_t	Rewind();
			void		Restart();

			status_t	AddEntry(Transaction& transaction, const char* name,
							size_t nameLength, ino_t id, uint8 type);
			status_t	FindEntry(const char* name, ino_t* id = NULL);
			status_t	RemoveEntry(Transaction& transaction);

			status_t	ChangeEntry(Transaction& transaction, ino_t id,
							uint8 fileType);

private:
						DirectoryIterator(const DirectoryIterator&);
						DirectoryIterator &operator=(const DirectoryIterator&);
							// no implementation


protected:
			status_t	_AllocateBestEntryInBlock(uint8 nameLength, uint16& pos,
							uint16& newLength);
			status_t	_AddEntry(Transaction& transaction, const char* name,
							uint8 nameLength, ino_t id, uint8 fileType,
							uint16 newLength, uint16 pos,
							bool hasPrevious = true);
			status_t	_SplitIndexedBlock(Transaction& transaction,
							const char* name, uint8 nameLength, ino_t id,
							uint8 type, uint32 newBlocksPos,
							bool firstSplit = false);

			status_t	_NextBlock();


	Inode*				fDirectory;
	Volume*				fVolume;
	uint32				fBlockSize;
	HTreeEntryIterator*	fParent;
	bool				fIndexing;

	uint32				fNumBlocks;
	uint32				fLogicalBlock;
	uint32				fPhysicalBlock;
	uint32				fDisplacement;
	uint32				fPreviousDisplacement;

	uint32				fStartPhysicalBlock;
	uint32				fStartLogicalBlock;
	uint32				fStartDisplacement;

	status_t			fInitStatus;
};

#endif	// DIRECTORY_ITERATOR_H

