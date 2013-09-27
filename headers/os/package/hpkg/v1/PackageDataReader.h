/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__V1__PACKAGE_DATA_READER_H_
#define _PACKAGE__HPKG__V1__PACKAGE_DATA_READER_H_


#include <package/hpkg/DataReader.h>


namespace BPackageKit {

namespace BHPKG {


class BBufferPool;


namespace V1 {


class BPackageData;


class BPackageDataReaderFactory {
public:
								BPackageDataReaderFactory(
									BBufferPool* bufferPool);

			status_t			CreatePackageDataReader(BDataReader* dataReader,
									const BPackageData& data,
									BAbstractBufferedDataReader*& _reader);

private:
			BBufferPool*		fBufferPool;
};


}	// namespace V1

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__V1__PACKAGE_DATA_READER_H_
