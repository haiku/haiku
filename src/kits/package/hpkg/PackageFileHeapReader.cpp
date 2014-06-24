/*
 * Copyright 2013-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageFileHeapReader.h>

#include <algorithm>
#include <new>

#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/HPKGDefs.h>

#include <AutoDeleter.h>
#include <package/hpkg/PoolBuffer.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


PackageFileHeapReader::PackageFileHeapReader(BErrorOutput* errorOutput, int fd,
	off_t heapOffset, off_t compressedHeapSize, uint64 uncompressedHeapSize)
	:
	PackageFileHeapAccessorBase(errorOutput, fd, heapOffset),
	fOffsets()
{
	fCompressedHeapSize = compressedHeapSize;
	fUncompressedHeapSize = uncompressedHeapSize;
}


PackageFileHeapReader::~PackageFileHeapReader()
{
}


status_t
PackageFileHeapReader::Init()
{
	if (fUncompressedHeapSize == 0)
		return B_OK;

	// Determine number of chunks and adjust the compressed heap size (subtract
	// the size of the chunk size array at the end). Note that the size of the
	// last chunk has not been saved, since it size is implied.
	ssize_t chunkCount = (fUncompressedHeapSize + kChunkSize - 1) / kChunkSize;
	if (chunkCount <= 0)
		return B_OK;

	fCompressedHeapSize -= (chunkCount - 1) * 2;

	// allocate a buffer
	uint16* buffer = (uint16*)malloc(kChunkSize);
	if (buffer == NULL)
		return B_NO_MEMORY;
	MemoryDeleter bufferDeleter(buffer);

	// read the chunk size array
	size_t remainingChunks = chunkCount - 1;
	size_t index = 0;
	uint64 offset = fCompressedHeapSize;
	while (remainingChunks > 0) {
		size_t toRead = std::min(remainingChunks, kChunkSize / 2);
		status_t error = ReadFileData(offset, buffer, toRead * 2);
		if (error != B_OK)
			return error;

		if (!fOffsets.InitChunksOffsets(chunkCount, index, buffer, toRead))
			return B_NO_MEMORY;

		remainingChunks -= toRead;
		index += toRead;
		offset += toRead * 2;
	}

	return B_OK;
}


PackageFileHeapReader*
PackageFileHeapReader::Clone() const
{
	PackageFileHeapReader* clone = new(std::nothrow) PackageFileHeapReader(
		fErrorOutput, fFD, fHeapOffset, fCompressedHeapSize,
		fUncompressedHeapSize);
	if (clone == NULL)
		return NULL;

	ssize_t chunkCount = (fUncompressedHeapSize + kChunkSize - 1) / kChunkSize;
	if (!clone->fOffsets.Init(chunkCount, fOffsets)) {
		delete clone;
		return NULL;
	}

	return clone;
}


status_t
PackageFileHeapReader::ReadAndDecompressChunk(size_t chunkIndex,
	void* compressedDataBuffer, void* uncompressedDataBuffer)
{
	uint64 offset = fOffsets[chunkIndex];
	bool isLastChunk
		= uint64(chunkIndex + 1) * kChunkSize >= fUncompressedHeapSize;
	size_t compressedSize = isLastChunk
		? fCompressedHeapSize - offset
		: fOffsets[chunkIndex + 1] - offset;
	size_t uncompressedSize = isLastChunk
		? fUncompressedHeapSize - (uint64)chunkIndex * kChunkSize
		: kChunkSize;

	return ReadAndDecompressChunkData(offset, compressedSize, uncompressedSize,
		compressedDataBuffer, uncompressedDataBuffer);
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
