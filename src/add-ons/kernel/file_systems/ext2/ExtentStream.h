/*
 * Copyright 2001-2011, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef EXTENTSTREAM_H
#define EXTENTSTREAM_H


#include "ext2.h"
#include "Transaction.h"


class Inode;
class Volume;


class ExtentStream
{
public:
					ExtentStream(Volume* volume, Inode* inode,
						ext2_extent_stream* stream, off_t size);
					~ExtentStream();

	status_t		FindBlock(off_t offset, fsblock_t& block,
						uint32 *_count = NULL);
	status_t		Enlarge(Transaction& transaction, off_t& numBlocks);
	status_t		Shrink(Transaction& transaction, off_t& numBlocks);
	void			Init();
	
	bool			Check();

private:
	status_t		_Check(ext2_extent_stream *stream, fileblock_t &block);
	status_t		_CheckBlock(ext2_extent_stream *stream, fsblock_t block);

	Volume*			fVolume;
	Inode*			fInode;
	ext2_extent_stream* fStream;
	fsblock_t		fFirstBlock;

	fsblock_t		fAllocatedPos;

	off_t			fNumBlocks;
	off_t			fSize;
};

#endif	// EXTENTSTREAM_H

