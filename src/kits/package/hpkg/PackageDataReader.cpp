/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageDataReader.h>

#include <string.h>

#include <algorithm>
#include <new>

#include <package/hpkg/HPKGDefsPrivate.h>
#include <package/hpkg/BufferCache.h>
#include <package/hpkg/CachedBuffer.h>
#include <package/hpkg/DataOutput.h>
#include <package/hpkg/PackageData.h>
#include <package/hpkg/ZlibDecompressor.h>


namespace BPackageKit {

namespace BHPKG {


using namespace BPrivate;


// minimum/maximum zlib chunk size we consider sane
static const size_t kMinSaneZlibChunkSize = 1024;
static const size_t kMaxSaneZlibChunkSize = 10 * 1024 * 1024;

// maximum number of entries in the zlib offset table buffer
static const uint32 kMaxZlibOffsetTableBufferSize = 512;

static const size_t kUncompressedReaderBufferSize
	= B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB;


// #pragma mark - BPackageDataReader


BPackageDataReader::BPackageDataReader(BDataReader* dataReader)
	:
	fDataReader(dataReader)
{
}


BPackageDataReader::~BPackageDataReader()
{
}


status_t
BPackageDataReader::ReadData(off_t offset, void* buffer, size_t size)
{
	BBufferDataOutput output(buffer, size);
	return ReadDataToOutput(offset, size, &output);
}


// #pragma mark - UncompressedPackageDataReader


class UncompressedPackageDataReader : public BPackageDataReader {
public:
	UncompressedPackageDataReader(BDataReader* dataReader,
		BBufferCache* bufferCache)
		:
		BPackageDataReader(dataReader),
		fBufferCache(bufferCache)
	{
	}

	status_t Init(const BPackageData& data)
	{
		fOffset = data.Offset();
		fSize = data.UncompressedSize();
		return B_OK;
	}

	virtual uint64 Size() const
	{
		return fSize;
	}

	virtual size_t BlockSize() const
	{
		// TODO: Some other value?
		return 64 * 1024;
	}

	virtual status_t ReadData(off_t offset, void* buffer, size_t size)
	{
		if (size == 0)
			return B_OK;

		if (offset < 0)
			return B_BAD_VALUE;

		if ((uint64)offset > fSize || size > fSize - offset)
			return B_BAD_VALUE;

		return fDataReader->ReadData(fOffset + offset, buffer, size);
	}

	virtual status_t ReadDataToOutput(off_t offset, size_t size,
		BDataOutput* output)
	{
		if (size == 0)
			return B_OK;

		if (offset < 0)
			return B_BAD_VALUE;

		if ((uint64)offset > fSize || size > fSize - offset)
			return B_BAD_VALUE;

		// get a temporary buffer
		CachedBuffer* buffer = fBufferCache->GetBuffer(
			kUncompressedReaderBufferSize);
		if (buffer == NULL)
			return B_NO_MEMORY;
		CachedBufferPutter bufferPutter(fBufferCache, &buffer);

		while (size > 0) {
			// read into the buffer
			size_t toRead = std::min(size, buffer->Size());
			status_t error = fDataReader->ReadData(fOffset + offset,
				buffer->Buffer(), toRead);
			if (error != B_OK)
				return error;

			// write to the output
			error = output->WriteData(buffer->Buffer(), toRead);
			if (error != B_OK)
				return error;

			offset += toRead;
			size -= toRead;
		}

		return B_OK;
	}

private:
	BBufferCache*	fBufferCache;
	uint64			fOffset;
	uint64			fSize;
};


// #pragma mark - ZlibPackageDataReader


class ZlibPackageDataReader : public BPackageDataReader {
public:
	ZlibPackageDataReader(BDataReader* dataReader, BBufferCache* bufferCache)
		:
		BPackageDataReader(dataReader),
		fBufferCache(bufferCache),
		fUncompressBuffer(NULL),
		fOffsetTable(NULL)
	{
	}

	~ZlibPackageDataReader()
	{
		delete[] fOffsetTable;

		fBufferCache->PutBuffer(&fUncompressBuffer);
	}

	status_t Init(const BPackageData& data)
	{
		fOffset = data.Offset();
		fCompressedSize = data.CompressedSize();
		fUncompressedSize = data.UncompressedSize();
		fChunkSize = data.ChunkSize();

		// validate chunk size
		if (fChunkSize == 0)
			fChunkSize = B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB;
		if (fChunkSize < kMinSaneZlibChunkSize
			|| fChunkSize > kMaxSaneZlibChunkSize) {
			return B_BAD_DATA;
		}

		fChunkCount = (fUncompressedSize + (fChunkSize - 1)) / fChunkSize;
		fOffsetTableSize = (fChunkCount - 1) * sizeof(uint64);
		if (fOffsetTableSize >= fCompressedSize)
			return B_BAD_DATA;

		// allocate a buffer for the offset table
		if (fChunkCount > 1) {
			fOffsetTableBufferEntryCount = std::min(fChunkCount - 1,
				(uint64)kMaxZlibOffsetTableBufferSize);
			fOffsetTable = new(std::nothrow) uint64[
				fOffsetTableBufferEntryCount];
			if (fOffsetTable == NULL)
				return B_NO_MEMORY;

			fOffsetTableIndex = -1;
				// mark the table content invalid
		} else
			fChunkSize = fUncompressedSize;

		// mark uncompressed content invalid
		fUncompressedChunk = -1;

		return B_OK;
	}

	virtual uint64 Size() const
	{
		return fUncompressedSize;
	}

	virtual size_t BlockSize() const
	{
		return fChunkSize;
	}

	virtual status_t ReadDataToOutput(off_t offset, size_t size,
		BDataOutput* output)
	{
		// check offset and size
		if (size == 0)
			return B_OK;

		if (offset < 0)
			return B_BAD_VALUE;

		if ((uint64)offset > fUncompressedSize
			|| size > fUncompressedSize - offset) {
			return B_BAD_VALUE;
		}

		// get our uncompressed chunk buffer back, if possible
		bool newBuffer;
		if (fBufferCache->GetBuffer(fChunkSize, &fUncompressBuffer, &newBuffer)
				== NULL) {
			return B_NO_MEMORY;
		}
		CachedBufferPutter uncompressBufferPutter(fBufferCache,
			&fUncompressBuffer);

		if (newBuffer)
			fUncompressedChunk = -1;

		// uncompress
		int64 chunkIndex = offset / fChunkSize;
		off_t chunkOffset = chunkIndex * fChunkSize;
		size_t inChunkOffset = offset - chunkOffset;

		while (size > 0) {
			// read and uncompress the chunk
			status_t error = _ReadChunk(chunkIndex);
			if (error != B_OK)
				return error;

			// write data to output
			size_t toCopy = std::min(size, (size_t)fChunkSize - inChunkOffset);
			error = output->WriteData(
				(uint8*)fUncompressBuffer->Buffer() + inChunkOffset, toCopy);
			if (error != B_OK)
				return error;

			size -= toCopy;

			chunkIndex++;
			chunkOffset += fChunkSize;
			inChunkOffset = 0;
		}

		return B_OK;
	}

private:
	status_t _ReadChunk(int64 chunkIndex)
	{
		if (chunkIndex == fUncompressedChunk)
			return B_OK;

		// get the chunk offset and size
		uint64 offset;
		uint32 compressedSize;
		status_t error = _GetCompressedChunkOffsetAndSize(chunkIndex, offset,
			compressedSize);
		if (error != B_OK)
			return error;

		uint32 uncompressedSize = (uint64)chunkIndex + 1 < fChunkCount
			? fChunkSize : fUncompressedSize - chunkIndex * fChunkSize;

		// read the chunk
		if (compressedSize == uncompressedSize) {
			// the chunk is not compressed -- read it directly into the
			// uncompressed buffer
			error = fDataReader->ReadData(offset, fUncompressBuffer->Buffer(),
				compressedSize);
		} else {
			// read to a read buffer and uncompress
			CachedBuffer* readBuffer = fBufferCache->GetBuffer(fChunkSize);
			if (readBuffer == NULL)
				return B_NO_MEMORY;
			CachedBufferPutter readBufferPutter(fBufferCache, readBuffer);

			error = fDataReader->ReadData(offset, readBuffer->Buffer(),
				compressedSize);
			if (error != B_OK)
				return error;

			size_t actuallyUncompressedSize;
			error = ZlibDecompressor::DecompressSingleBuffer(
				readBuffer->Buffer(), compressedSize,
				fUncompressBuffer->Buffer(), uncompressedSize,
				actuallyUncompressedSize);
			if (error == B_OK && actuallyUncompressedSize != uncompressedSize)
				error = B_BAD_DATA;
		}

		if (error != B_OK) {
			// error reading/decompressing data -- mark the cached data invalid
			fUncompressedChunk = -1;
			return error;
		}

		fUncompressedChunk = chunkIndex;
		return B_OK;
	}

	status_t _GetCompressedChunkOffsetAndSize(int64 chunkIndex, uint64& _offset,
		uint32& _size)
	{
		// get the offset
		uint64 offset;
		if (chunkIndex == 0) {
			// first chunk is at 0
			offset = 0;
		} else {
			status_t error = _GetCompressedChunkRelativeOffset(chunkIndex,
				offset);
			if (error != B_OK)
				return error;
		}

		// get the end offset
		uint64 endOffset;
		if ((uint64)chunkIndex + 1 == fChunkCount) {
			// last chunk end with the end of the data
			endOffset = fCompressedSize - fOffsetTableSize;
		} else {
			status_t error = _GetCompressedChunkRelativeOffset(chunkIndex + 1,
				endOffset);
			if (error != B_OK)
				return error;
		}

		// sanity check
		if (endOffset < offset)
			return B_BAD_DATA;

		_offset = fOffset + fOffsetTableSize + offset;
		_size = endOffset - offset;
		return B_OK;
	}

	status_t _GetCompressedChunkRelativeOffset(int64 chunkIndex,
		uint64& _offset)
	{
		if (fOffsetTableIndex < 0 || fOffsetTableIndex > chunkIndex
			|| fOffsetTableIndex + fOffsetTableBufferEntryCount <= chunkIndex) {
			// read the table at the given index, or, if we can, the whole table
			int64 readAtIndex = fChunkCount - 1 > fOffsetTableBufferEntryCount
				? chunkIndex : 1;
			uint32 entriesToRead = std::min(
				(uint64)fOffsetTableBufferEntryCount,
				fChunkCount - readAtIndex);

			status_t error = fDataReader->ReadData(
				fOffset + (readAtIndex - 1) * sizeof(uint64),
				fOffsetTable, entriesToRead * sizeof(uint64));
			if (error != B_OK) {
				fOffsetTableIndex = -1;
				return error;
			}

			fOffsetTableIndex = readAtIndex;
		}

		// get and check the offset
		_offset = fOffsetTable[chunkIndex - fOffsetTableIndex];
		if (_offset > fCompressedSize - fOffsetTableSize)
			return B_BAD_DATA;

		return B_OK;
	}

private:
	BBufferCache*	fBufferCache;
	CachedBuffer*	fUncompressBuffer;
	int64			fUncompressedChunk;

	uint64			fOffset;
	uint64			fUncompressedSize;
	uint64			fCompressedSize;
	uint64			fOffsetTableSize;
	uint64			fChunkCount;
	uint32			fChunkSize;
	uint32			fOffsetTableBufferEntryCount;
	uint64*			fOffsetTable;
	int32			fOffsetTableIndex;
};


// #pragma mark - BPackageDataReaderFactory


BPackageDataReaderFactory::BPackageDataReaderFactory(BBufferCache* bufferCache)
	:
	fBufferCache(bufferCache)
{
}


status_t
BPackageDataReaderFactory::CreatePackageDataReader(BDataReader* dataReader,
	const BPackageData& data, BPackageDataReader*& _reader)
{
	BPackageDataReader* reader;

	switch (data.Compression()) {
		case B_HPKG_COMPRESSION_NONE:
			reader = new(std::nothrow) UncompressedPackageDataReader(
				dataReader, fBufferCache);
			break;
		case B_HPKG_COMPRESSION_ZLIB:
			reader = new(std::nothrow) ZlibPackageDataReader(dataReader,
				fBufferCache);
			break;
		default:
			return B_BAD_VALUE;
	}

	if (reader == NULL)
		return B_NO_MEMORY;

	status_t error = reader->Init(data);
	if (error != B_OK) {
		delete reader;
		return error;
	}

	_reader = reader;
	return B_OK;
}


}	// namespace BHPKG

}	// namespace BPackageKit
