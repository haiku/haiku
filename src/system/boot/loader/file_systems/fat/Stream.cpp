/*
 * Copyright 2008, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */


#include "Stream.h"
#include "CachedBlock.h"
#include "Directory.h"
#include "File.h"

#include <util/kernel_cpp.h>

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define TRACE(x) dprintf x


using namespace FATFS;


Stream::Stream(Volume &volume, uint32 chain, off_t size, const char *name)
	:
	fVolume(volume),
	fFirstCluster(chain),
	//fClusters(NULL),
	fClusterMapCacheLast(0),
	fSize(size)
{
	TRACE(("FATFS::Stream::(, %d, %Ld, %s)\n", chain, size, name));
	fName[FATFS_NAME_LENGTH] = '\0';
	strlcpy(fName, name, FATFS_NAME_LENGTH+1);
	fClusterCount = (fSize + fVolume.ClusterSize() - 1) / fVolume.ClusterSize();
	if (size == UINT32_MAX)
		fClusterCount = 10; // ?
	for (int i = 0; i < CLUSTER_MAP_CACHE_SIZE; i++) {
		fClusterMapCache[i].block = -1;
		fClusterMapCache[i].cluster = fVolume.InvalidClusterID();
	}
}


Stream::~Stream()
{
	TRACE(("FATFS::Stream::~()\n"));
}


status_t
Stream::InitCheck()
{
	if (fSize && !fVolume.IsValidCluster(fFirstCluster))
		return B_BAD_VALUE;
	return B_OK;
}


status_t
Stream::GetName(char *nameBuffer, size_t bufferSize) const
{
	return strlcpy(nameBuffer, fName, bufferSize);
}


status_t
Stream::FindBlock(off_t pos, off_t &block, off_t &offset)
{
	//TRACE(("FATFS::Stream::%s(%Ld,,)\n", __FUNCTION__, pos));
	uint32 index = (uint32)(pos / fVolume.ClusterSize());
	uint32 cluster;
	if (pos >= fSize)
		return B_BAD_VALUE;
	if (index >= fClusterCount)
		return B_BAD_VALUE;

	bool found = false;
	int i;
	for (i = 0; i < CLUSTER_MAP_CACHE_SIZE; i++) {
		if (fClusterMapCache[i].block == index) {
			cluster = fClusterMapCache[i].cluster;
			found = true;
			break;
		}
	}
	if (!found) {
#if 1
		uint32 count = (fSize + fVolume.ClusterSize() - 1) / fVolume.ClusterSize();
		cluster = fFirstCluster;
		if (fSize == UINT32_MAX) // it's a directory, try a large enough value
			count = 10;
		for (i = 0; i < index && fVolume.IsValidCluster(cluster); i++) {
			if (fVolume.IsLastCluster(cluster))
				break;
			//TRACE(("FATFS::Stream::%s: [%d] = %d\n", __FUNCTION__, i, cluster));
			cluster = fVolume.NextCluster(cluster);
			//TRACE(("FATFS::Stream::%s: %04lx ?\n", __FUNCTION__, cluster));
		}
#endif
#if 0
		cluster = fVolume.NextCluster(cluster, index);
#endif
	}
	if (!fVolume.IsValidCluster(cluster))
		return ENOENT;
	fClusterMapCache[fClusterMapCacheLast].block = index;
	fClusterMapCache[fClusterMapCacheLast].cluster = cluster;
	fClusterMapCacheLast++;
	fClusterMapCacheLast %= CLUSTER_MAP_CACHE_SIZE;

	//cluster = fClusters[index];
	// convert to position
	offset = fVolume.ToOffset(cluster);
	offset += (pos %= fVolume.ClusterSize());
	// convert to block + offset
	block = fVolume.ToBlock(offset);
	offset %= fVolume.BlockSize();
	//TRACE(("FATFS::Stream::FindBlock: %Ld:%Ld\n", block, offset));
	return B_OK;
}


status_t
Stream::ReadAt(off_t pos, uint8 *buffer, size_t *_length)
{
	TRACE(("FATFS::Stream::%s(%Ld, )\n", __FUNCTION__, pos));
	status_t err;
	// set/check boundaries for pos/length

	if (pos < 0)
		return B_BAD_VALUE;
	if (pos >= fSize) {
		*_length = 0;
		return B_NO_ERROR;
	}

#if 0
	// lazily build the cluster list
	if (!fClusters) {
		err = BuildClusterList();
		if (err < B_OK)
			return err;
	}
#endif

	size_t length = *_length;

	if (pos + length > fSize)
		length = fSize - pos;

	off_t num; // block number
	off_t offset;
	if (FindBlock(pos, num, offset) < B_OK) {
		*_length = 0;
		return B_BAD_VALUE;
	}

	uint32 bytesRead = 0;
	uint32 blockSize = fVolume.BlockSize();
	uint32 blockShift = fVolume.BlockShift();
	uint8 *block;

	// the first block_run we read could not be aligned to the block_size boundary
	// (read partial block at the beginning)

	// pos % block_size == (pos - offset) % block_size, offset % block_size == 0
	if (pos % blockSize != 0) {
		CachedBlock cached(fVolume, num);
		if ((block = cached.Block()) == NULL) {
			*_length = 0;
			return B_BAD_VALUE;
		}

		bytesRead = blockSize - (pos % blockSize);
		if (length < bytesRead)
			bytesRead = length;

		memcpy(buffer, block + (pos % blockSize), bytesRead);
		pos += bytesRead;

		length -= bytesRead;
		if (length == 0) {
			*_length = bytesRead;
			return B_OK;
		}

		if (FindBlock(pos, num, offset) < B_OK) {
			*_length = bytesRead;
			return B_BAD_VALUE;
		}
	}

	// the first block_run is already filled in at this point
	// read the following complete blocks using cached_read(),
	// the last partial block is read using the generic Cache class

	bool partial = false;

	while (length > 0) {
		// offset is the offset to the current pos in the block_run

		if (length < blockSize) {
			CachedBlock cached(fVolume, num);
			if ((block = cached.Block()) == NULL) {
				*_length = bytesRead;
				return B_BAD_VALUE;
			}
			memcpy(buffer + bytesRead, block, length);
			bytesRead += length;
			partial = true;
			break;
		}

		if (read_pos(fVolume.Device(), fVolume.ToOffset(num), 
			buffer + bytesRead, fVolume.BlockSize()) < B_OK) {
			*_length = bytesRead;
			return B_BAD_VALUE;
		}

		int32 bytes = fVolume.BlockSize();
		length -= bytes;
		bytesRead += bytes;
		if (length == 0)
			break;

		pos += bytes;

		if (FindBlock(pos, num, offset) < B_OK) {
			*_length = bytesRead;
			return B_BAD_VALUE;
		}
	}

	*_length = bytesRead;
	return B_NO_ERROR;
}

status_t
Stream::BuildClusterList()
{
#if 0
	TRACE(("FATFS::Stream::%s()\n", __FUNCTION__));
	uint32 count = (fSize + fVolume.ClusterSize() - 1) / fVolume.ClusterSize();
	uint32 c = fFirstCluster;
	int i;

	if (fSize == UINT32_MAX) // it's a directory, try a large enough value
		count = 10;
	//fClusters = (uint32 *)malloc(count * sizeof(uint32));
	for (i = 0; i < count && fVolume.IsValidCluster(c); i++) {
		if (fVolume.IsLastCluster(c))
			break;
		TRACE(("FATFS::Stream::%s: [%d] = %d\n", __FUNCTION__, i, c));
		fClusters[i] = c;
		c = fVolume.NextCluster(c);
		TRACE(("FATFS::Stream::%s: %04lx ?\n", __FUNCTION__, c));
		// XXX: try to realloc() for dirs maybe ?
	}
	fClusterCount = i;
	TRACE(("FATFS::Stream::%s: %d clusters in chain\n", __FUNCTION__, i));
#endif
	return B_OK;
}
