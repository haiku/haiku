/*
 * Copyright 2013-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageFileHeapAccessorBase.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>
#include <DataIO.h>
#include <package/hpkg/ErrorOutput.h>

#include <AutoDeleter.h>
#include <CompressionAlgorithm.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


// #pragma mark - OffsetArray


PackageFileHeapAccessorBase::OffsetArray::OffsetArray()
	:
	fOffsets(NULL)
{
}


PackageFileHeapAccessorBase::OffsetArray::~OffsetArray()
{
	delete[] fOffsets;
}


bool
PackageFileHeapAccessorBase::OffsetArray::InitChunksOffsets(
	size_t totalChunkCount, size_t baseIndex, const uint16* chunkSizes,
	size_t chunkCount)
{
	if (totalChunkCount <= 1)
		return true;

	if (fOffsets == NULL) {
		fOffsets = new(std::nothrow) uint32[totalChunkCount];
		if (fOffsets == NULL)
			return false;
		fOffsets[0] = 0;
			// Value that serves as a marker that all offsets are 32 bit. We'll
			// replace it, when necessary.
	}

	uint64 offset = (*this)[baseIndex];
	for (size_t i = 0; i < chunkCount; i++) {
		offset += (uint64)B_BENDIAN_TO_HOST_INT16(chunkSizes[i]) + 1;
			// the stored value is chunkSize - 1
		size_t index = baseIndex + i + 1;
			// (baseIndex + i) is the index of the chunk whose size is stored in
			// chunkSizes[i]. We compute the offset of the following element
			// which we store at index (baseIndex + i + 1).

		if (offset <= ~(uint32)0) {
			fOffsets[index] = (uint32)offset;
		} else {
			if (fOffsets[0] == 0) {
				// Not scaled to allow for 64 bit offsets yet. Do that.
				fOffsets[0] = index;
				uint32* newOffsets = new(std::nothrow) uint32[
					2 * totalChunkCount - fOffsets[0]];
				if (newOffsets == NULL)
					return false;

				memcpy(newOffsets, fOffsets,
					sizeof(newOffsets[0]) * fOffsets[0]);

				delete[] fOffsets;
				fOffsets = newOffsets;
			}

			index += index - fOffsets[0];
			fOffsets[index] = (uint32)offset;
			fOffsets[index + 1] = uint32(offset >> 32);
		}
	}

	return true;
}


bool
PackageFileHeapAccessorBase::OffsetArray::Init(size_t totalChunkCount,
	const OffsetArray& other)
{
	if (other.fOffsets == NULL)
		return true;

	size_t elementCount = other.fOffsets[0] == 0
		? totalChunkCount
		: 2 * totalChunkCount - other.fOffsets[0];

	fOffsets = new(std::nothrow) uint32[elementCount];
	if (fOffsets == NULL)
		return false;

	memcpy(fOffsets, other.fOffsets, elementCount * sizeof(fOffsets[0]));
	return true;
}


// #pragma mark - PackageFileHeapAccessorBase


PackageFileHeapAccessorBase::PackageFileHeapAccessorBase(
	BErrorOutput* errorOutput, int fd, off_t heapOffset,
	DecompressionAlgorithmOwner* decompressionAlgorithm)
	:
	fErrorOutput(errorOutput),
	fFD(fd),
	fHeapOffset(heapOffset),
	fCompressedHeapSize(0),
	fUncompressedHeapSize(0),
	fDecompressionAlgorithm(decompressionAlgorithm)
{
	if (fDecompressionAlgorithm != NULL)
		fDecompressionAlgorithm->AcquireReference();
}


PackageFileHeapAccessorBase::~PackageFileHeapAccessorBase()
{
	if (fDecompressionAlgorithm != NULL)
		fDecompressionAlgorithm->ReleaseReference();
}


uint64
PackageFileHeapAccessorBase::HeapOverhead(uint64 uncompressedSize) const
{
	// Determine number of chunks and the size of the chunk size table. Note
	// that the size of the last chunk is not saved, since its size is implied.
	size_t chunkCount = (uncompressedSize + kChunkSize - 1) / kChunkSize;
	return chunkCount > 1 ? (chunkCount - 1) * 2 : 0;
}


status_t
PackageFileHeapAccessorBase::ReadDataToOutput(off_t offset, size_t size,
	BDataIO* output)
{
	if (size == 0)
		return B_OK;

	if (offset < 0 || (uint64)offset > fUncompressedHeapSize
		|| size > fUncompressedHeapSize - offset) {
		return B_BAD_VALUE;
	}

	// allocate buffers for compressed and uncompressed data
	uint16* compressedDataBuffer = (uint16*)malloc(kChunkSize);
	uint16* uncompressedDataBuffer = (uint16*)malloc(kChunkSize);
	MemoryDeleter compressedDataBufferDeleter(compressedDataBuffer);
	MemoryDeleter uncompressedDataBufferDeleter(uncompressedDataBuffer);
	if (compressedDataBuffer == NULL || uncompressedDataBuffer == NULL)
		return B_NO_MEMORY;

	// read the data
	size_t chunkIndex = size_t(offset / kChunkSize);
	size_t inChunkOffset = (uint64)offset - (uint64)chunkIndex * kChunkSize;
	size_t remainingBytes = size;

	while (remainingBytes > 0) {
		status_t error = ReadAndDecompressChunk(chunkIndex,
			compressedDataBuffer, uncompressedDataBuffer);
		if (error != B_OK)
			return error;

		size_t toWrite = std::min((size_t)kChunkSize - inChunkOffset,
			remainingBytes);
			// The last chunk may be shorter than kChunkSize, but since
			// size (and thus remainingSize) had been clamped, that doesn't
			// harm.
		error = output->WriteExactly(
			(char*)uncompressedDataBuffer + inChunkOffset, toWrite);
		if (error != B_OK)
			return error;

		remainingBytes -= toWrite;
		chunkIndex++;
		inChunkOffset = 0;
	}

	return B_OK;
}


status_t
PackageFileHeapAccessorBase::ReadAndDecompressChunkData(uint64 offset,
	size_t compressedSize, size_t uncompressedSize, void* compressedDataBuffer,
	void* uncompressedDataBuffer)
{
	// if uncompressed, read directly into the uncompressed data buffer
	if (compressedSize == uncompressedSize)
		return ReadFileData(offset, uncompressedDataBuffer, compressedSize);

	// otherwise read into the other buffer and decompress
	status_t error = ReadFileData(offset, compressedDataBuffer, compressedSize);
	if (error != B_OK)
		return error;

	return DecompressChunkData(compressedDataBuffer, compressedSize,
		uncompressedDataBuffer, uncompressedSize);
}


status_t
PackageFileHeapAccessorBase::DecompressChunkData(void* compressedDataBuffer,
	size_t compressedSize, void* uncompressedDataBuffer,
	size_t uncompressedSize)
{
	size_t actualSize;
	status_t error = fDecompressionAlgorithm->algorithm->DecompressBuffer(
		compressedDataBuffer, compressedSize, uncompressedDataBuffer,
		uncompressedSize, actualSize, fDecompressionAlgorithm->parameters);
	if (error != B_OK) {
		fErrorOutput->PrintError("Failed to decompress chunk data: %s\n",
			strerror(error));
		return error;
	}

	if (actualSize != uncompressedSize) {
		fErrorOutput->PrintError("Failed to decompress chunk data: size "
			"mismatch\n");
		return B_ERROR;
	}

	return B_OK;
}


status_t
PackageFileHeapAccessorBase::ReadFileData(uint64 offset, void* buffer,
	size_t size)
{
	ssize_t bytesRead = pread(fFD, buffer, size, fHeapOffset + (off_t)offset);
	if (bytesRead < 0) {
		fErrorOutput->PrintError("ReadFileData(%" B_PRIu64 ", %p, %zu) failed "
			"to read data: %s\n", offset, buffer, size, strerror(errno));
		return errno;
	}
	if ((size_t)bytesRead != size) {
		fErrorOutput->PrintError("ReadFileData(%" B_PRIu64 ", %p, %zu) could "
			"read only %zd bytes\n", offset, buffer, size, bytesRead);
		return B_ERROR;
	}

	return B_OK;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
