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
	kClusterSize(volume->ClusterSize()),
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
	cluster_t clusterIndex = pos / kClusterSize;
	uint32 offset = pos % kClusterSize;

	cluster_t cluster = fInode->StartCluster();
	for (uint32 i = 0; i < clusterIndex; i++)
		cluster = fInode->NextCluster(cluster);
	fsblock_t block;
	if (fVolume->ClusterToBlock(cluster, block) != B_OK)
		return B_BAD_DATA;
	physical = block * kBlockSize + offset;
	for (uint32 i = 0; i < 64; i++) {
		cluster_t extentEnd = fInode->NextCluster(cluster);
		if (extentEnd == EXFAT_CLUSTER_END || extentEnd == cluster + 1)
			break;
		cluster = extentEnd;
	}
	*_length = min_c((cluster - clusterIndex + 1) * kClusterSize - offset,
		fSize - pos);
	TRACE("inode %" B_PRIdINO ": cluster %" B_PRIu32 ", pos %" B_PRIdOFF ", %"
		B_PRIdOFF "\n", fInode->ID(), clusterIndex, pos, physical);
	return B_OK;
}

