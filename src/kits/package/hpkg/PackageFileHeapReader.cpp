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


PackageFileHeapReader::PackageFileHeapReader(BErrorOutput* errorOutput,
	BPositionIO* file, off_t heapOffset, off_t compressedHeapSize,
	uint64 uncompressedHeapSize,
	DecompressionAlgorithmOwner* decompressionAlgorithm)
	:
	PackageFileHeapAccessorBase(errorOutput, file, heapOffset,
		decompressionAlgorithm),
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
	if (fUncompressedHeapSize == 0) {
		if (fCompressedHeapSize != 0) {
			fErrorOutput->PrintError(
				"Invalid total compressed heap size (!= 0, empty heap)\n");
			return B_BAD_DATA;
		}
		return B_OK;
	}

	// Determine number of chunks and adjust the compressed heap size (subtract
	// the size of the chunk size array at the end). Note that the size of the
	// last chunk has not been saved, since its size is implied.
	ssize_t chunkCount = (fUncompressedHeapSize + kChunkSize - 1) / kChunkSize;
	if (chunkCount == 0)
		return B_OK;

	// If no compression is used at all, the chunk size table is omitted. Handle
	// this case.
	if (fDecompressionAlgorithm == NULL) {
		if (fUncompressedHeapSize != fCompressedHeapSize) {
			fErrorOutput->PrintError(
				"Compressed and uncompressed heap sizes (%" B_PRIu64 " vs. "
				"%" B_PRIu64 ") don't match for uncompressed heap.\n",
				fCompressedHeapSize, fUncompressedHeapSize);
			return B_BAD_DATA;
		}

		if (!fOffsets.InitUncompressedChunksOffsets(chunkCount))
			return B_NO_MEMORY;

		return B_OK;
	}

	size_t chunkSizeTableSize = (chunkCount - 1) * 2; 
	if (fCompressedHeapSize <= chunkSizeTableSize) {
		fErrorOutput->PrintError(
			"Invalid total compressed heap size (%" B_PRIu64 ", "
			"uncompressed %" B_PRIu64 ")\n", fCompressedHeapSize,
			fUncompressedHeapSize);
		return B_BAD_DATA;
	}

	fCompressedHeapSize -= chunkSizeTableSize;

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

	// Sanity check: The sum of the chunk sizes must match the compressed heap
	// size. The information aren't stored redundantly, so we check, if things
	// look at least plausible.
	uint64 lastChunkOffset = fOffsets[chunkCount - 1];
	if (lastChunkOffset >= fCompressedHeapSize
			|| fCompressedHeapSize - lastChunkOffset > kChunkSize
			|| fCompressedHeapSize - lastChunkOffset
				> fUncompressedHeapSize - (chunkCount - 1) * kChunkSize) {
		fErrorOutput->PrintError(
			"Invalid total compressed heap size (%" B_PRIu64 ", uncompressed: "
			"%" B_PRIu64 ", last chunk offset: %" B_PRIu64 ")\n",
			fCompressedHeapSize, fUncompressedHeapSize, lastChunkOffset);
		return B_BAD_DATA;
	}

	return B_OK;
}


PackageFileHeapReader*
PackageFileHeapReader::Clone() const
{
	PackageFileHeapReader* clone = new(std::nothrow) PackageFileHeapReader(
		fErrorOutput, fFile, fHeapOffset, fCompressedHeapSize,
		fUncompressedHeapSize, fDecompressionAlgorithm);
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
		= ((uint64)chunkIndex + 1) * kChunkSize >= fUncompressedHeapSize;
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
