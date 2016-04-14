/*
 * Copyright 2008, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */


#include "Stream.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>

#include <boot/FileMapDisk.h>

#include "CachedBlock.h"
#include "Directory.h"
#include "File.h"


//#define TRACE(x) dprintf x
#define TRACE(x) do {} while (0)


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
Stream::GetFileMap(struct file_map_run *runs, int32 *count)
{
	int32 i;
	uint32 cluster = fFirstCluster;
	uint32 next = fVolume.InvalidClusterID();
	off_t offset = 0LL;

	for (i = 0; i < *count; i++) {
		runs[i].offset = offset;
		runs[i].block = fVolume.ToBlock(cluster);
		runs[i].len = fVolume.ClusterSize();
		do {
			next = fVolume.NextCluster(cluster);
			if (next != cluster + 1)
				break;
			runs[i].len += fVolume.ClusterSize();
		} while (true);
		if (!fVolume.IsValidCluster(next))
			break;
		cluster = next;
		offset += runs[i].len;
	}

	// too big
	if (i == *count && fVolume.IsValidCluster(next))
		return B_ERROR;

	*count = i;
	return B_OK;
}


status_t
Stream::_FindCluster(off_t pos, uint32& _cluster)
{
	//TRACE(("FATFS::Stream::%s(%Ld,,)\n", __FUNCTION__, pos));
	uint32 index = (uint32)(pos / fVolume.ClusterSize());
	if (pos > fSize || index >= fClusterCount)
		return B_BAD_VALUE;

	uint32 cluster = 0;
	bool found = false;
	uint32 i;
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
		return B_ENTRY_NOT_FOUND;

	fClusterMapCache[fClusterMapCacheLast].block = index;
	fClusterMapCache[fClusterMapCacheLast].cluster = cluster;
	fClusterMapCacheLast++;
	fClusterMapCacheLast %= CLUSTER_MAP_CACHE_SIZE;

	_cluster = cluster;
	return B_OK;
}


status_t
Stream::_FindOrCreateCluster(off_t pos, uint32& _cluster, bool& _added)
{
	status_t error = _FindCluster(pos, _cluster);
	if (error == B_OK) {
		_added = false;
		return B_OK;
	}

	// iterate through the cluster list
	uint32 index = (uint32)(pos / fVolume.ClusterSize());
	uint32 cluster = fFirstCluster;
	uint32 clusterCount = 0;
	if (cluster != 0) {
		uint32 nextCluster = cluster;
		while (clusterCount <= index) {
			if (!fVolume.IsValidCluster(nextCluster)
					|| fVolume.IsLastCluster(nextCluster)) {
				break;
			}

			cluster = nextCluster;
			clusterCount++;
			nextCluster = fVolume.NextCluster(nextCluster);
		}
	}

	if (clusterCount > index) {
		// the cluster existed after all
		_cluster = cluster;
		_added = false;
		return B_OK;
	}

	while (clusterCount <= index) {
		uint32 newCluster;
		error = fVolume.AllocateCluster(cluster, newCluster);
		if (error != B_OK)
			return error;

		if (clusterCount == 0)
			fFirstCluster = newCluster;

		// TODO: We should support to zero out the new cluster. Maybe make this
		// and optional parameter of WriteAt().

		cluster = newCluster;
		clusterCount++;
	}

	_cluster = cluster;
	_added = true;
	return B_OK;
}


status_t
Stream::FindBlock(off_t pos, off_t &block, off_t &offset)
{
	uint32 cluster;
	status_t error = _FindCluster(pos, cluster);
	if (error != B_OK)
		return error;

	// convert to position
	offset = fVolume.ClusterToOffset(cluster);
	offset += (pos %= fVolume.ClusterSize());

	// convert to block + offset
	block = fVolume.ToBlock(offset);
	offset %= fVolume.BlockSize();

	return B_OK;
}


status_t
Stream::ReadAt(off_t pos, void *_buffer, size_t *_length, off_t *diskOffset)
{
	TRACE(("FATFS::Stream::%s(%Ld, )\n", __FUNCTION__, pos));

	uint8* buffer = (uint8*)_buffer;

	// set/check boundaries for pos/length
	if (pos < 0)
		return B_BAD_VALUE;
	if (pos >= fSize) {
		*_length = 0;
		return B_OK;
	}

#if 0
	// lazily build the cluster list
	if (!fClusters) {
		status_t status = BuildClusterList();
		if (status != B_OK)
			return status;
	}
#endif

	size_t length = *_length;

	if (pos + (off_t)length > fSize)
		length = fSize - pos;

	off_t num; // block number
	off_t offset;
	if (FindBlock(pos, num, offset) < B_OK) {
		*_length = 0;
		return B_BAD_VALUE;
	}

	if (diskOffset != NULL)
		*diskOffset = fVolume.BlockToOffset(num) + offset;

	uint32 bytesRead = 0;
	uint32 blockSize = fVolume.BlockSize();
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

		if (read_pos(fVolume.Device(), fVolume.BlockToOffset(num),
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
	return B_OK;
}


status_t
Stream::WriteAt(off_t pos, const void* _buffer, size_t* _length,
	off_t* diskOffset)
{
	if (pos < 0)
		return B_BAD_VALUE;

	const uint8* buffer = (const uint8*)_buffer;
	size_t length = *_length;
	size_t totalWritten = 0;
	status_t error = B_OK;

	CachedBlock cachedBlock(fVolume);

	while (length > 0) {
		// get the cluster
		uint32 cluster;
		bool added;
		error = _FindOrCreateCluster(pos, cluster, added);
		if (error != B_OK)
			break;

		// convert to position
		off_t inClusterOffset = pos % fVolume.ClusterSize();
		off_t offset = fVolume.ClusterToOffset(cluster) + inClusterOffset;

		if (diskOffset != NULL) {
			*diskOffset = offset;
			diskOffset = NULL;
		}

		// convert to block + offset
		off_t block = fVolume.ToBlock(offset);
		size_t inBlockOffset = offset % fVolume.BlockSize();

		// write
		size_t toWrite = std::min((size_t)fVolume.BlockSize() - inBlockOffset,
			length);
		if (toWrite == (size_t)fVolume.BlockSize()) {
			// write the whole block
			ssize_t written = write_pos(fVolume.Device(),
				fVolume.BlockToOffset(block), buffer, fVolume.BlockSize());
			if (written < 0) {
				error = written;
				break;
			}
			if (written != fVolume.BlockSize()) {
				error = B_ERROR;
				break;
			}
		} else {
			// write a partial block -- need to read it from disk first
			error = cachedBlock.SetTo(block, CachedBlock::READ);
			if (error != B_OK)
				break;

			memcpy(cachedBlock.Block() + inBlockOffset, buffer, toWrite);

			error = cachedBlock.Flush();
			if (error != B_OK)
				break;
		}

		totalWritten += toWrite;
		pos += toWrite;
		buffer += toWrite;
		length -= toWrite;

		if (pos > fSize)
			fSize = pos;
	}

	*_length = totalWritten;
	return totalWritten > 0 ? B_OK : error;
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
