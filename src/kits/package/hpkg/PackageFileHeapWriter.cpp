/*
 * Copyright 2013-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageFileHeapWriter.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>
#include <List.h>
#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/HPKGDefs.h>

#include <AutoDeleter.h>
#include <package/hpkg/DataReader.h>
#include <package/hpkg/PackageFileHeapReader.h>
#include <RangeArray.h>
#include <CompressionAlgorithm.h>


// minimum length of data we require before trying to compress them
static const size_t kCompressionSizeThreshold = 64;


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


struct PackageFileHeapWriter::Chunk {
	uint64	offset;
	uint32	compressedSize;
	uint32	uncompressedSize;
	void*	buffer;
};


struct PackageFileHeapWriter::ChunkSegment {
	ssize_t	chunkIndex;
	uint32	toKeepOffset;
	uint32	toKeepSize;
};


struct PackageFileHeapWriter::ChunkBuffer {
	ChunkBuffer(PackageFileHeapWriter* writer, size_t bufferSize)
		:
		fWriter(writer),
		fChunks(),
		fCurrentChunkIndex(0),
		fNextReadIndex(0),
		fSegments(),
		fCurrentSegmentIndex(0),
		fBuffers(),
		fUnusedBuffers(),
		fBufferSize(bufferSize)
	{
	}

	~ChunkBuffer()
	{
		for (int32 i = 0; void* buffer = fBuffers.ItemAt(i); i++)
			free(buffer);
	}

	bool PushChunkSegment(uint64 chunkOffset, uint32 compressedSize,
		uint32 uncompressedSize, uint32 toKeepOffset, uint32 toKeepSize)
	{
		ChunkSegment segment;
		segment.toKeepOffset = toKeepOffset;
		segment.toKeepSize = toKeepSize;

		// might refer to the last chunk
		segment.chunkIndex = fChunks.Count() - 1;

		if (segment.chunkIndex < 0
			|| fChunks.ElementAt(segment.chunkIndex).offset != chunkOffset) {
			// no, need to push a new chunk
			segment.chunkIndex++;

			Chunk chunk;
			chunk.offset = chunkOffset;
			chunk.compressedSize = compressedSize;
			chunk.uncompressedSize = uncompressedSize;
			chunk.buffer = NULL;
			if (!fChunks.Add(chunk))
				return false;
		}

		return fSegments.Add(segment);
	}

	bool IsEmpty() const
	{
		return fSegments.IsEmpty();
	}

	bool HasMoreSegments() const
	{
		return fCurrentSegmentIndex < fSegments.Count();
	}

	const ChunkSegment& CurrentSegment() const
	{
		return fSegments[fCurrentSegmentIndex];
	}

	const Chunk& ChunkAt(ssize_t index) const
	{
		return fChunks[index];
	}

	bool HasMoreChunksToRead() const
	{
		return fNextReadIndex < fChunks.Count();
	}

	bool HasBufferedChunk() const
	{
		return fCurrentChunkIndex < fNextReadIndex;
	}

	uint64 NextReadOffset() const
	{
		return fChunks[fNextReadIndex].offset;
	}

	void ReadNextChunk()
	{
		if (!HasMoreChunksToRead())
			throw status_t(B_BAD_VALUE);

		Chunk& chunk = fChunks[fNextReadIndex++];
		chunk.buffer = _GetBuffer();

		status_t error = fWriter->ReadFileData(chunk.offset, chunk.buffer,
			chunk.compressedSize);
		if (error != B_OK)
			throw error;
	}

	void CurrentSegmentDone()
	{
		// Unless the next segment refers to the same chunk, advance to the next
		// chunk.
		const ChunkSegment& segment = fSegments[fCurrentSegmentIndex++];
		if (!HasMoreSegments()
			|| segment.chunkIndex != CurrentSegment().chunkIndex) {
			_PutBuffer(fChunks[fCurrentChunkIndex++].buffer);
		}
	}

private:
	void* _GetBuffer()
	{
		if (!fUnusedBuffers.IsEmpty())
			return fUnusedBuffers.RemoveItem(fUnusedBuffers.CountItems() - 1);

		void* buffer = malloc(fBufferSize);
		if (buffer == NULL || !fBuffers.AddItem(buffer)) {
			free(buffer);
			throw std::bad_alloc();
		}

		return buffer;
	}

	void _PutBuffer(void* buffer)
	{
		if (buffer != NULL && !fUnusedBuffers.AddItem(buffer)) {
			fBuffers.RemoveItem(buffer);
			free(buffer);
		}
	}

private:
	PackageFileHeapWriter*	fWriter;

	Array<Chunk>			fChunks;
	ssize_t					fCurrentChunkIndex;
	ssize_t					fNextReadIndex;

	Array<ChunkSegment>		fSegments;
	ssize_t					fCurrentSegmentIndex;

	BList					fBuffers;
	BList					fUnusedBuffers;
	size_t					fBufferSize;
};


PackageFileHeapWriter::PackageFileHeapWriter(BErrorOutput* errorOutput,
	BPositionIO* file, off_t heapOffset,
	CompressionAlgorithmOwner* compressionAlgorithm,
	DecompressionAlgorithmOwner* decompressionAlgorithm)
	:
	PackageFileHeapAccessorBase(errorOutput, file, heapOffset,
		decompressionAlgorithm),
	fPendingDataBuffer(NULL),
	fCompressedDataBuffer(NULL),
	fPendingDataSize(0),
	fOffsets(),
	fCompressionAlgorithm(compressionAlgorithm)
{
	if (fCompressionAlgorithm != NULL)
		fCompressionAlgorithm->AcquireReference();
}


PackageFileHeapWriter::~PackageFileHeapWriter()
{
	_Uninit();

	if (fCompressionAlgorithm != NULL)
		fCompressionAlgorithm->ReleaseReference();
}


void
PackageFileHeapWriter::Init()
{
	// allocate data buffers
	fPendingDataBuffer = malloc(kChunkSize);
	fCompressedDataBuffer = malloc(kChunkSize);
	if (fPendingDataBuffer == NULL || fCompressedDataBuffer == NULL)
		throw std::bad_alloc();
}


void
PackageFileHeapWriter::Reinit(PackageFileHeapReader* heapReader)
{
	fHeapOffset = heapReader->HeapOffset();
	fCompressedHeapSize = heapReader->CompressedHeapSize();
	fUncompressedHeapSize = heapReader->UncompressedHeapSize();
	fPendingDataSize = 0;

	// copy the offsets array
	size_t chunkCount = (fUncompressedHeapSize + kChunkSize - 1) / kChunkSize;
	if (chunkCount > 0) {
		if (!fOffsets.AddUninitialized(chunkCount))
			throw std::bad_alloc();

		for (size_t i = 0; i < chunkCount; i++)
			fOffsets[i] = heapReader->Offsets()[i];
	}

	_UnwriteLastPartialChunk();
}


status_t
PackageFileHeapWriter::AddData(BDataReader& dataReader, off_t size,
	uint64& _offset)
{
	_offset = fUncompressedHeapSize;

	// copy the data to the heap
	off_t readOffset = 0;
	off_t remainingSize = size;
	while (remainingSize > 0) {
		// read data into pending data buffer
		size_t toCopy = std::min(remainingSize,
			off_t(kChunkSize - fPendingDataSize));
		status_t error = dataReader.ReadData(readOffset,
			(uint8*)fPendingDataBuffer + fPendingDataSize, toCopy);
		if (error != B_OK) {
			fErrorOutput->PrintError("Failed to read data: %s\n",
				strerror(error));
			return error;
		}

		fPendingDataSize += toCopy;
		fUncompressedHeapSize += toCopy;
		remainingSize -= toCopy;
		readOffset += toCopy;

		if (fPendingDataSize == kChunkSize) {
			error = _FlushPendingData();
			if (error != B_OK)
				return error;
		}
	}

	return B_OK;
}


void
PackageFileHeapWriter::AddDataThrows(const void* buffer, size_t size)
{
	BBufferDataReader reader(buffer, size);
	uint64 dummyOffset;
	status_t error = AddData(reader, size, dummyOffset);
	if (error != B_OK)
		throw status_t(error);
}


void
PackageFileHeapWriter::RemoveDataRanges(
	const ::BPrivate::RangeArray<uint64>& ranges)
{
	ssize_t rangeCount = ranges.CountRanges();
	if (rangeCount == 0)
		return;

	if (fUncompressedHeapSize == 0) {
		fErrorOutput->PrintError("Can't remove ranges from empty heap\n");
		throw status_t(B_BAD_VALUE);
	}

	// Before we begin flush any pending data, so we don't need any special
	// handling and also can use the pending data buffer.
	status_t status = _FlushPendingData();
	if (status != B_OK)
		throw status_t(status);

	// We potentially have to recompress all data from the first affected chunk
	// to the end (minus the removed ranges, of course). As a basic algorithm we
	// can use our usual data writing strategy, i.e. read a chunk, decompress it
	// to a temporary buffer, and write the data to keep via AddData(). There
	// are a few complications/optimizations, though:
	// * As data moves to other chunks, it may actually compress worse than
	//   before. While unlikely, we still have to take care of this case by
	//   making sure our reading end is at least a complete uncompressed chunk
	//   ahead of the writing end.
	// * When we run into the situation that we have to move complete aligned
	//   chunks, we want to avoid uncompressing and recompressing them
	//   needlessly.

	// Build a list of (possibly partial) chunks we want to keep.

	// the first partial chunk (if any) and all chunks between ranges
	ChunkBuffer chunkBuffer(this, kChunkSize);
	uint64 writeOffset = ranges[0].offset - ranges[0].offset % kChunkSize;
	uint64 readOffset = writeOffset;
	for (ssize_t i = 0; i < rangeCount; i++) {
		const Range<uint64>& range = ranges[i];
		if (range.size > 0) {
			_PushChunks(chunkBuffer, readOffset, range.offset);
			readOffset = range.offset + range.size;
		}
	}

	if (readOffset == writeOffset) {
		fErrorOutput->PrintError("Only empty ranges to remove from heap\n");
		throw status_t(B_BAD_VALUE);
	}

	// all chunks after the last range
	_PushChunks(chunkBuffer, readOffset, fUncompressedHeapSize);

	// Reset our state to look like all chunks from the first affected one have
	// been removed and re-add all data we want to keep.

	// truncate the offsets array and reset the heap sizes
	ssize_t firstChunkIndex = ssize_t(writeOffset / kChunkSize);
	fCompressedHeapSize = fOffsets[firstChunkIndex];
	fUncompressedHeapSize = (uint64)firstChunkIndex * kChunkSize;
	fOffsets.Remove(firstChunkIndex, fOffsets.Count() - firstChunkIndex);

	// we need a decompression buffer
	void* decompressionBuffer = malloc(kChunkSize);
	if (decompressionBuffer == NULL)
		throw std::bad_alloc();
	MemoryDeleter decompressionBufferDeleter(decompressionBuffer);

	const Chunk* decompressedChunk = NULL;

	while (chunkBuffer.HasMoreSegments()) {
		const ChunkSegment& segment = chunkBuffer.CurrentSegment();

		// If we have an aligned, complete chunk, copy its compressed data.
		bool copyCompressed = fPendingDataSize == 0 && segment.toKeepOffset == 0
			&& segment.toKeepSize == kChunkSize;

		// Read more chunks. We need at least one buffered one to do anything
		// and we want to buffer as many as necessary to ensure we don't
		// overwrite one we haven't buffered yet.
		while (chunkBuffer.HasMoreChunksToRead()
			&& (!chunkBuffer.HasBufferedChunk()
				|| (!copyCompressed
					&& chunkBuffer.NextReadOffset()
						< fCompressedHeapSize + kChunkSize))) {
			// read chunk
			chunkBuffer.ReadNextChunk();
		}

		// copy compressed chunk data, if possible
		const Chunk& chunk = chunkBuffer.ChunkAt(segment.chunkIndex);
		if (copyCompressed) {
			status_t error = _WriteChunk(chunk.buffer, chunk.compressedSize,
				false);
			if (error != B_OK)
				throw error;
			continue;
		}

		// decompress chunk, if compressed
		void* uncompressedData;
		if (chunk.uncompressedSize == chunk.compressedSize) {
			uncompressedData = chunk.buffer;
		} else if (decompressedChunk == &chunk) {
			uncompressedData = decompressionBuffer;
		} else {
			status_t error = DecompressChunkData(chunk.buffer,
				chunk.compressedSize, decompressionBuffer,
				chunk.uncompressedSize);
			if (error != B_OK)
				throw error;

			decompressedChunk = &chunk;
			uncompressedData = decompressionBuffer;
		}

		// add chunk data
		AddDataThrows((uint8*)uncompressedData + segment.toKeepOffset,
			segment.toKeepSize);

		chunkBuffer.CurrentSegmentDone();
	}

	// Make sure a last partial chunk ends up in the pending data buffer. This
	// is only necessary when we didn't have to move any chunk segments, since
	// the loop would otherwise have read it in and left it in the pending data
	// buffer.
	if (chunkBuffer.IsEmpty())
		_UnwriteLastPartialChunk();
}


status_t
PackageFileHeapWriter::Finish()
{
	// flush pending data, if any
	status_t error = _FlushPendingData();
	if (error != B_OK)
		return error;

	// write chunk sizes table

	// We don't need to do that, if we don't use any compression.
	if (fCompressionAlgorithm == NULL)
		return B_OK;

	// We don't need to write the last chunk size, since it is implied by the
	// total size minus the sum of all other chunk sizes.
	ssize_t offsetCount = fOffsets.Count();
	if (offsetCount < 2)
		return B_OK;

	// Convert the offsets to 16 bit sizes and write them. We use the (no longer
	// used) pending data buffer for the conversion.
	uint16* buffer = (uint16*)fPendingDataBuffer;
	for (ssize_t offsetIndex = 1; offsetIndex < offsetCount;) {
		ssize_t toWrite = std::min(offsetCount - offsetIndex,
			ssize_t(kChunkSize / 2));

		for (ssize_t i = 0; i < toWrite; i++, offsetIndex++) {
			// store chunkSize - 1, so it fits 16 bit (chunks cannot be empty)
			buffer[i] = B_HOST_TO_BENDIAN_INT16(
				uint16(fOffsets[offsetIndex] - fOffsets[offsetIndex - 1] - 1));
		}

		error = _WriteDataUncompressed(buffer, toWrite * 2);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


status_t
PackageFileHeapWriter::ReadAndDecompressChunk(size_t chunkIndex,
	void* compressedDataBuffer, void* uncompressedDataBuffer)
{
	if (uint64(chunkIndex + 1) * kChunkSize > fUncompressedHeapSize) {
		// The chunk has not been written to disk yet. Its data are still in the
		// pending data buffer.
		memcpy(uncompressedDataBuffer, fPendingDataBuffer, fPendingDataSize);
		// TODO: This can be optimized. Since we write to a BDataIO anyway,
		// there's no need to copy the data.
		return B_OK;
	}

	uint64 offset = fOffsets[chunkIndex];
	size_t compressedSize = chunkIndex + 1 == (size_t)fOffsets.Count()
		? fCompressedHeapSize - offset
		: fOffsets[chunkIndex + 1] - offset;

	return ReadAndDecompressChunkData(offset, compressedSize, kChunkSize,
		compressedDataBuffer, uncompressedDataBuffer);
}


void
PackageFileHeapWriter::_Uninit()
{
	free(fPendingDataBuffer);
	free(fCompressedDataBuffer);
	fPendingDataBuffer = NULL;
	fCompressedDataBuffer = NULL;
}


status_t
PackageFileHeapWriter::_FlushPendingData()
{
	if (fPendingDataSize == 0)
		return B_OK;

	status_t error = _WriteChunk(fPendingDataBuffer, fPendingDataSize, true);
	if (error == B_OK)
		fPendingDataSize = 0;

	return error;
}


status_t
PackageFileHeapWriter::_WriteChunk(const void* data, size_t size,
	bool mayCompress)
{
	// add offset
	if (!fOffsets.Add(fCompressedHeapSize)) {
		fErrorOutput->PrintError("Out of memory!\n");
		return B_NO_MEMORY;
	}

	// Try to use compression only for data large enough.
	bool compress = mayCompress && size >= (off_t)kCompressionSizeThreshold;
	if (compress) {
		status_t error = _WriteDataCompressed(data, size);
		if (error != B_OK) {
			if (error != B_BUFFER_OVERFLOW)
				return error;
			compress = false;
		}
	}

	// Write uncompressed, if necessary.
	if (!compress) {
		status_t error = _WriteDataUncompressed(data, size);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


status_t
PackageFileHeapWriter::_WriteDataCompressed(const void* data, size_t size)
{
	if (fCompressionAlgorithm == NULL)
		return B_BUFFER_OVERFLOW;

	size_t compressedSize;
	status_t error = fCompressionAlgorithm->algorithm->CompressBuffer(data,
		size, fCompressedDataBuffer, size, compressedSize,
		fCompressionAlgorithm->parameters);
	if (error != B_OK) {
		if (error != B_BUFFER_OVERFLOW) {
			fErrorOutput->PrintError("Failed to compress chunk data: %s\n",
				strerror(error));
		}
		return error;
	}

	// only use compressed data when we've actually saved space
	if (compressedSize == size)
		return B_BUFFER_OVERFLOW;

	return _WriteDataUncompressed(fCompressedDataBuffer, compressedSize);
}


status_t
PackageFileHeapWriter::_WriteDataUncompressed(const void* data, size_t size)
{
	status_t error = fFile->WriteAtExactly(
		fHeapOffset + (off_t)fCompressedHeapSize, data, size);
	if (error != B_OK) {
		fErrorOutput->PrintError("Failed to write data: %s\n", strerror(error));
		return error;
	}

	fCompressedHeapSize += size;

	return B_OK;
}


void
PackageFileHeapWriter::_PushChunks(ChunkBuffer& chunkBuffer, uint64 startOffset,
	uint64 endOffset)
{
	if (endOffset > fUncompressedHeapSize) {
		fErrorOutput->PrintError("Invalid range to remove from heap\n");
		throw status_t(B_BAD_VALUE);
	}

	ssize_t chunkIndex = startOffset / kChunkSize;
	uint64 uncompressedChunkOffset = (uint64)chunkIndex * kChunkSize;

	while (startOffset < endOffset) {
		bool isLastChunk = fUncompressedHeapSize - uncompressedChunkOffset
			<= kChunkSize;
		uint32 inChunkOffset = uint32(startOffset - uncompressedChunkOffset);
		uint32 uncompressedChunkSize = isLastChunk
			? fUncompressedHeapSize - uncompressedChunkOffset
			: kChunkSize;
		uint64 compressedChunkOffset = fOffsets[chunkIndex];
		uint32 compressedChunkSize = isLastChunk
			? fCompressedHeapSize - compressedChunkOffset
			: fOffsets[chunkIndex + 1] - compressedChunkOffset;
		uint32 toKeepSize = uint32(std::min(
			(uint64)uncompressedChunkSize - inChunkOffset,
			endOffset - startOffset));

		if (!chunkBuffer.PushChunkSegment(compressedChunkOffset,
				compressedChunkSize, uncompressedChunkSize, inChunkOffset,
				toKeepSize)) {
			throw std::bad_alloc();
		}

		startOffset += toKeepSize;
		chunkIndex++;
		uncompressedChunkOffset += uncompressedChunkSize;
	}
}


void
PackageFileHeapWriter::_UnwriteLastPartialChunk()
{
	// If the last chunk is partial, read it in and remove it from the offsets.
	size_t lastChunkSize = fUncompressedHeapSize % kChunkSize;
	if (lastChunkSize != 0) {
		uint64 lastChunkOffset = fOffsets[fOffsets.Count() - 1];
		size_t compressedSize = fCompressedHeapSize - lastChunkOffset;

		status_t error = ReadAndDecompressChunkData(lastChunkOffset,
			compressedSize, lastChunkSize, fCompressedDataBuffer,
			fPendingDataBuffer);
		if (error != B_OK)
			throw error;

		fPendingDataSize = lastChunkSize;
		fCompressedHeapSize = lastChunkOffset;
		fOffsets.Remove(fOffsets.Count() - 1);
	}
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
