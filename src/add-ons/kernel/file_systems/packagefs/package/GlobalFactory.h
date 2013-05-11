/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef GLOBAL_FACTORY_H
#define GLOBAL_FACTORY_H


#include <package/hpkg/HPKGDefs.h>
#include <package/hpkg/PackageDataReader.h>

#include "BlockBufferCacheKernel.h"


using BPackageKit::BHPKG::BDataReader;
using BPackageKit::BHPKG::BPackageData;
using BPackageKit::BHPKG::BPackageDataReader;
using BPackageKit::BHPKG::BPackageDataReaderFactory;
using BPackageKit::BHPKG::B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB;


class GlobalFactory {
private:
								GlobalFactory();
								~GlobalFactory();

public:
	static	status_t			CreateDefault();
	static	void				DeleteDefault();
	static	GlobalFactory*		Default();

			status_t			CreatePackageDataReader(BDataReader* dataReader,
									const BPackageData& data,
									BPackageDataReader*& _reader);

private:
			status_t			_Init();

private:
	static	GlobalFactory*		sDefaultInstance;

			BlockBufferCacheKernel fBufferCache;
			BPackageDataReaderFactory fPackageDataReaderFactory;
};

#endif	// GLOBAL_FACTORY_H
