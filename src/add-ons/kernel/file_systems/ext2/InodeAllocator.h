/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef INODEALLOCATOR_H
#define INODEALLOCATOR_H

#include <lock.h>

#include "Transaction.h"


class Inode;
class Volume;


class InodeAllocator {
public:
						InodeAllocator(Volume* volume);
	virtual				~InodeAllocator();

	virtual	status_t	New(Transaction& transaction, Inode* parent,
							int32 mode, ino_t& id);
	virtual	status_t	Free(Transaction& transaction, ino_t id,
							bool isDirectory);

private:
			status_t	_Allocate(Transaction& transaction,
							uint32 prefferedBlockGroup, bool isDirectory,
							ino_t& id);
			status_t	_MarkInBitmap(Transaction& transaction,
							uint32 bitmapBlock, uint32 blockGroup,
							uint32 numInodes, ino_t& id);
			status_t	_UnmarkInBitmap(Transaction& transaction,
							uint32 bitmapBlock, uint32 numInodes, ino_t id);


			Volume*		fVolume;
			mutex		fLock;
};

#endif	// INODEALLOCATOR_H
