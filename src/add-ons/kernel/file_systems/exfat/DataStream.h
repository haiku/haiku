/*
 * Copyright 2011, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef DATASTREAM_H
#define DATASTREAM_H


#include "exfat.h"
#include "Inode.h"


class Volume;


class DataStream
{
public:
								DataStream(Volume* volume, Inode* inode,
									off_t size);
								~DataStream();

			status_t			FindBlock(off_t pos, off_t& physical,
									off_t *_length = NULL);
private:
			const uint32		kBlockSize;
			const uint32		kClusterSize;
			Volume*				fVolume;
			Inode*				fInode;
			off_t				fNumBlocks;
			off_t				fSize;
};

#endif	// DATASTREAM_H

