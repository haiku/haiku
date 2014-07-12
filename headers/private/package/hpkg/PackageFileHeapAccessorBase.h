/*
 * Copyright 2013-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_ACCESSOR_BASE_H_
#define _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_ACCESSOR_BASE_H_


#include <new>

#include <Referenceable.h>

#include <CompressionAlgorithm.h>
#include <package/hpkg/DataReader.h>


namespace BPackageKit {

namespace BHPKG {


class BErrorOutput;


namespace BPrivate {


template<typename Parameters>
struct GenericCompressionAlgorithmOwner : BReferenceable {
	BCompressionAlgorithm*	algorithm;
	Parameters*				parameters;

	GenericCompressionAlgorithmOwner(BCompressionAlgorithm* algorithm,
		Parameters* parameters)
		:
		algorithm(algorithm),
		parameters(parameters)
	{
	}

	~GenericCompressionAlgorithmOwner()
	{
		delete algorithm;
		delete parameters;
	}

	static GenericCompressionAlgorithmOwner* Create(
		BCompressionAlgorithm* algorithm, Parameters* parameters)
	{
		GenericCompressionAlgorithmOwner* owner
			= new(std::nothrow) GenericCompressionAlgorithmOwner(algorithm,
				parameters);
		if (owner == NULL) {
			delete algorithm;
			delete parameters;
		}

		return owner;
	}
};

typedef GenericCompressionAlgorithmOwner<BCompressionParameters>
	CompressionAlgorithmOwner;
typedef GenericCompressionAlgorithmOwner<BDecompressionParameters>
	DecompressionAlgorithmOwner;


class PackageFileHeapAccessorBase : public BAbstractBufferedDataReader {
public:
			class OffsetArray;

public:
								PackageFileHeapAccessorBase(
									BErrorOutput* errorOutput,
									BPositionIO* file, off_t heapOffset,
									DecompressionAlgorithmOwner*
										decompressionAlgorithm);
	virtual						~PackageFileHeapAccessorBase();

			off_t				HeapOffset() const
									{ return fHeapOffset; }
			off_t				CompressedHeapSize() const
									{ return fCompressedHeapSize; }
			uint64				UncompressedHeapSize() const
									{ return fUncompressedHeapSize; }
			size_t				ChunkSize() const
									{ return kChunkSize; }

			// normally used after cloning a PackageFileHeapReader only
			void				SetErrorOutput(BErrorOutput* errorOutput)
									{ fErrorOutput = errorOutput; }
			void				SetFile(BPositionIO* file)
									{ fFile = file; }

	// BAbstractBufferedDataReader
	virtual	status_t			ReadDataToOutput(off_t offset,
									size_t size, BDataIO* output);

public:
	static	const size_t		kChunkSize = 64 * 1024;

protected:
	virtual	status_t			ReadAndDecompressChunk(size_t chunkIndex,
									void* compressedDataBuffer,
									void* uncompressedDataBuffer) = 0;
			status_t			ReadAndDecompressChunkData(uint64 offset,
									size_t compressedSize,
									size_t uncompressedSize,
									void* compressedDataBuffer,
									void* uncompressedDataBuffer);
			status_t			DecompressChunkData(
									void* compressedDataBuffer,
									size_t compressedSize,
									void* uncompressedDataBuffer,
									size_t uncompressedSize);
			status_t			ReadFileData(uint64 offset, void* buffer,
									size_t size);

protected:
			BErrorOutput*		fErrorOutput;
			BPositionIO*		fFile;
			off_t				fHeapOffset;
			uint64				fCompressedHeapSize;
			uint64				fUncompressedHeapSize;
			DecompressionAlgorithmOwner* fDecompressionAlgorithm;
};


/*!	Stores the chunk offsets in a compact way, while still providing quick
	access.
	- The object doesn't store the number of chunks/offsets it contains. During
	  initialization the chunk count is provided. Later, when getting an offset,
	  the caller is responsible for ensuring a valid index.
	- The first (index 0) chunk offset is omitted, since it is always 0.
	- The chunk offsets that fit in a 32 bit number use only one 32 bit element
	  in the offsets array.
	- The chunk offsets that don't fit in a 32 bit number use two elements in
	  the offsets array.
	Memory use is one pointer, if the chunk count is <= 1 (uncompressed heap size
	<= 64 KiB). Afterwards it's one pointer plus 32 bit per chunk as long as the
	last offset still fits 32 bit (compressed heap size < 4GiB). For any further
	chunks it is 64 bit per chunk. So, for the common case we use sizeof(void*)
	plus 1 KiB per 16 MiB of uncompressed heap, or about 64 KiB per 1 GiB. Which
	seems reasonable for packagefs to keep in memory.
 */
class PackageFileHeapAccessorBase::OffsetArray {
public:
								OffsetArray();
								~OffsetArray();

			bool				InitUncompressedChunksOffsets(
									size_t totalChunkCount);
			bool				InitChunksOffsets(size_t totalChunkCount,
									size_t baseIndex, const uint16* chunkSizes,
									size_t chunkCount);

			bool				Init(size_t totalChunkCount,
									const OffsetArray& other);
									// "copy" init

			uint64				operator[](size_t index) const;

private:
	static	uint32*				_AllocateOffsetArray(size_t totalChunkCount,
									size_t offset32BitChunkCount);

private:
			uint32*				fOffsets;
									// - NULL, if chunkCount <= 1
									// - element 0 contains the number of 32 bit
									//   offsets that follow, or is 0, when all
									//   offsets are 32 bit only
									// - the following offsets use two elements
									//   each (lower followed by upper 32 bit)
									//   to represent the 64 bit value
};


inline uint64
PackageFileHeapAccessorBase::OffsetArray::operator[](size_t index) const
{
	if (index == 0)
		return 0;

	if (fOffsets[0] == 0 || index < fOffsets[0])
		return fOffsets[index];

	index += index - fOffsets[0];
	return fOffsets[index] | ((uint64)fOffsets[index + 1] << 32);
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_ACCESSOR_BASE_H_
