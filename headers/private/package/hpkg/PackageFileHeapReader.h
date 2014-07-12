/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_READER_H_
#define _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_READER_H_


#include <Array.h>
#include <package/hpkg/PackageFileHeapAccessorBase.h>


namespace BPackageKit {

namespace BHPKG {


class BDataReader;
class BErrorOutput;


namespace BPrivate {


class PackageFileHeapReader : public PackageFileHeapAccessorBase {
public:
								PackageFileHeapReader(BErrorOutput* errorOutput,
									BPositionIO* file, off_t heapOffset,
									off_t compressedHeapSize,
									uint64 uncompressedHeapSize,
									DecompressionAlgorithmOwner*
										decompressionAlgorithm);
								~PackageFileHeapReader();

			status_t			Init();

			PackageFileHeapReader* Clone() const;

			const OffsetArray&	Offsets() const
									{ return fOffsets; }

protected:
	virtual	status_t			ReadAndDecompressChunk(size_t chunkIndex,
									void* compressedDataBuffer,
									void* uncompressedDataBuffer);

private:
			OffsetArray			fOffsets;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_READER_H_
