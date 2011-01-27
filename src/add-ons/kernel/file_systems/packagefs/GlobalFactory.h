/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef GLOBAL_FACTORY_H
#define GLOBAL_FACTORY_H


#include <package/hpkg/haiku_package.h>
#include <package/hpkg/PackageDataReader.h>

#include "BlockBufferCacheKernel.h"


using namespace BPackageKit::BHaikuPackage::BPrivate;


class GlobalFactory {
private:
								GlobalFactory();
								~GlobalFactory();

public:
	static	status_t			CreateDefault();
	static	void				DeleteDefault();
	static	GlobalFactory*		Default();

			status_t			CreatePackageDataReader(DataReader* dataReader,
									const PackageData& data,
									PackageDataReader*& _reader);

private:
			status_t			_Init();

private:
	static	GlobalFactory*		sDefaultInstance;

			BlockBufferCacheKernel fBufferCache;
			PackageDataReaderFactory fPackageDataReaderFactory;
};

#endif	// GLOBAL_FACTORY_H
