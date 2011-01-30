/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_DATA_READER_H_
#define _PACKAGE__HPKG__PACKAGE_DATA_READER_H_


#include <package/hpkg/DataReader.h>


namespace BPackageKit {

namespace BHPKG {


class BBufferCache;
class BDataOutput;
class BPackageData;


class BPackageDataReader : public BDataReader {
public:
								BPackageDataReader(BDataReader* dataReader);
	virtual						~BPackageDataReader();

	virtual	status_t			Init(const BPackageData& data) = 0;

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size);
	virtual	status_t			ReadDataToOutput(off_t offset, size_t size,
									BDataOutput* output) = 0;

	virtual	uint64				Size() const = 0;
	virtual	size_t				BlockSize() const = 0;

protected:
			BDataReader*			fDataReader;
};


class BPackageDataReaderFactory {
public:
								BPackageDataReaderFactory(
									BBufferCache* bufferCache);

			status_t			CreatePackageDataReader(BDataReader* dataReader,
									const BPackageData& data,
									BPackageDataReader*& _reader);

private:
			BBufferCache*		fBufferCache;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_DATA_READER_H_
