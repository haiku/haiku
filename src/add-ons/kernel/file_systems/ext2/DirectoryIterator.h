/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef DIRECTORY_ITERATOR_H
#define DIRECTORY_ITERATOR_H


#include "ext2.h"

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
			status_t	GetNext(char* name, size_t* _nameLength, ino_t* id);

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
			off_t		_Offset() { return fLogicalBlock * fBlockSize
							+ fDisplacement; }

			bool		_CheckDirEntry(const ext2_dir_entry* dirEntry,
							const uint8* buffer);
			status_t	_CheckBlock(const uint8* buffer);
			uint32		_MaxSize();

#if 0
			ext2_dir_entry_tail* _DirEntryTail(uint8* block) const;
			uint32		_Checksum(uint8* block) const;
			void		_SetDirEntryChecksum(uint8* block);
#endif
			ext2_htree_tail* _HTreeEntryTail(uint8* block, uint16 offset) const;
			uint32		_HTreeRootChecksum(uint8* block, uint16 offset, uint16 count) const;
			void		_SetHTreeEntryChecksum(uint8* block, uint16 offset, uint16 count);


	Inode*				fDirectory;
	Volume*				fVolume;
	uint32				fBlockSize;
	HTreeEntryIterator*	fParent;
	bool				fIndexing;

	uint32				fNumBlocks;
	uint32				fLogicalBlock;
	fsblock_t			fPhysicalBlock;
	uint32				fDisplacement;
	uint32				fPreviousDisplacement;

	off_t				fStartPhysicalBlock;
	uint32				fStartLogicalBlock;
	uint32				fStartDisplacement;

	status_t			fInitStatus;
};

#endif	// DIRECTORY_ITERATOR_H

