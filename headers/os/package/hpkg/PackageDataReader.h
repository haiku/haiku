/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_DATA_READER_H_
#define _PACKAGE__HPKG__PACKAGE_DATA_READER_H_


#include <package/hpkg/DataReader.h>


namespace BPackageKit {

namespace BHPKG {


class BBufferPool;
class BPackageData;


class BPackageDataReaderFactory {
public:
								BPackageDataReaderFactory();

			status_t			CreatePackageDataReader(
									BAbstractBufferedDataReader* dataReader,
									const BPackageData& data,
									BAbstractBufferedDataReader*& _reader);
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_DATA_READER_H_
