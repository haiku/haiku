/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_DATA_READER_H
#define PACKAGE_DATA_READER_H


#include <package/hpkg/DataReader.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


class BufferCache;
class DataOutput;
class PackageData;


class PackageDataReader : public DataReader {
public:
								PackageDataReader(DataReader* dataReader);
	virtual						~PackageDataReader();

	virtual	status_t			Init(const PackageData& data) = 0;

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size);
	virtual	status_t			ReadDataToOutput(off_t offset, size_t size,
									DataOutput* output) = 0;

	virtual	uint64				Size() const = 0;
	virtual	size_t				BlockSize() const = 0;

protected:
			DataReader*			fDataReader;
};


class PackageDataReaderFactory {
public:
								PackageDataReaderFactory(
									BufferCache* bufferCache);

			status_t			CreatePackageDataReader(DataReader* dataReader,
									const PackageData& data,
									PackageDataReader*& _reader);

private:
			BufferCache*		fBufferCache;
};


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit


#endif	// PACKAGE_DATA_READER_H
