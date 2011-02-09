/*
 * Copyright 2011, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */


#include "DataStream.h"

#include "Volume.h"


//#define TRACE_EXFAT
#ifdef TRACE_EXFAT
#	define TRACE(x...) dprintf("\33[34mexfat:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)	dprintf("\33[34mexfat:\33[0m " x)


DataStream::DataStream(Volume* volume, Inode* inode, off_t size)
	:
	kBlockSize(volume->BlockSize()),
	fVolume(volume),
	fInode(inode),
	fSize(size)
{
	fNumBlocks = size == 0 ? 0 : ((size - 1) / kBlockSize) + 1;
}


DataStream::~DataStream()
{
}


status_t
DataStream::FindBlock(off_t pos, off_t& physical, off_t *_length)
{
	if (pos >= fSize) {
		TRACE("FindBlock: offset larger than size\n");
		return B_ENTRY_NOT_FOUND;
	}
	cluster_t clusterIndex = pos / fVolume->ClusterSize();
	uint32 offset = pos % fVolume->ClusterSize();

	cluster_t cluster = fInode->StartCluster();
	for (uint32 i = 0; i < clusterIndex; i++)
		cluster = fInode->NextCluster(cluster);
	fsblock_t block;
	fVolume->ClusterToBlock(cluster, block);
	physical = block * kBlockSize + offset;
	*_length = min_c(kBlockSize, fSize - pos);
	TRACE("inode %" B_PRIdINO ": cluster %ld, pos %lld, %lld\n",
		fInode->ID(), fInode->StartCluster(), pos, physical);
	return B_OK;
}

