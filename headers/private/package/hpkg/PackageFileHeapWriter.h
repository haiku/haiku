/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_WRITER_H_
#define _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_WRITER_H_


#include <Array.h>
#include <package/hpkg/PackageFileHeapAccessorBase.h>


class BCompressionParameters;

namespace BPrivate {
	template<typename Value> class RangeArray;
}


namespace BPackageKit {

namespace BHPKG {


class BDataReader;
class BErrorOutput;


namespace BPrivate {


class PackageFileHeapReader;


class PackageFileHeapWriter : public PackageFileHeapAccessorBase {
public:
								PackageFileHeapWriter(BErrorOutput* errorOutput,
									BPositionIO* file, off_t heapOffset,
									CompressionAlgorithmOwner*
										compressionAlgorithm,
									DecompressionAlgorithmOwner*
										decompressionAlgorithm);
								~PackageFileHeapWriter();

			void				Init();
			void				Reinit(PackageFileHeapReader* heapReader);

			status_t			AddData(BDataReader& dataReader, off_t size,
									uint64& _offset);
			void				AddDataThrows(const void* buffer, size_t size);
			void				RemoveDataRanges(
									const ::BPrivate::RangeArray<uint64>&
										ranges);
									// doesn't truncate the file
			status_t			Finish();

protected:
	virtual	status_t			ReadAndDecompressChunk(size_t chunkIndex,
									void* compressedDataBuffer,
									void* uncompressedDataBuffer);

private:
			struct Chunk;
			struct ChunkSegment;
			struct ChunkBuffer;

			friend struct ChunkBuffer;

private:
			void				_Uninit();

			status_t			_FlushPendingData();
			status_t			_WriteChunk(const void* data, size_t size,
									bool mayCompress);
			status_t			_WriteDataCompressed(const void* data,
									size_t size);
			status_t			_WriteDataUncompressed(const void* data,
									size_t size);

			void				_PushChunks(ChunkBuffer& chunkBuffer,
									uint64 startOffset, uint64 endOffset);
			void				_UnwriteLastPartialChunk();

private:
			void*				fPendingDataBuffer;
			void*				fCompressedDataBuffer;
			size_t				fPendingDataSize;
			Array<uint64>		fOffsets;
			CompressionAlgorithmOwner* fCompressionAlgorithm;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_WRITER_H_
